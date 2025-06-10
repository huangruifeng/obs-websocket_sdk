#include "obs_websocket_client.h"
#include <libwebsockets.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>

using json = nlohmann::json;

OBSWebSocketClient::OBSWebSocketClient(const std::string& serverUri) :_state(State::Disconnected),_running(false), _serverUri(serverUri), _context(nullptr), _wsi(nullptr) {
    _running = true;
    _thread = std::thread(&OBSWebSocketClient::run,this);
}

OBSWebSocketClient::~OBSWebSocketClient() {
    disconnect([](bool) {});
    _running = false;
    if (_thread.joinable()) {
        _thread.join();
    }
}

long long generateTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}


void handle_hello(OBSWebSocketClient* client, json& data) {
    try {
        //todo support authentication
        if (data.contains("authentication")) {
            //not support auth.
            std::cout << "not support authentication." << std::endl;
        }
        client->sendHello(1,"");
    }
    catch (std::exception& e) {
        //LOG_WARNING(e.what);
    }
}

void handle_connected(OBSWebSocketClient* client, json& data) {
    std::cout << "Successfully identified with obs-websocket: " << data.dump() << std::endl;
    client->_state = OBSWebSocketClient::State::Connected;
    client->for_each([](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
        ptr->OnConnected();
    });
}

void handle_operator(OBSWebSocketClient* client, json& data) {
    try {
        auto eventData = data["eventData"];
        if (eventData["eventType"] == "StreamStateChanged")
        {
            auto state = eventData["outputState"];
            if (state == "OBS_WEBSOCKET_OUTPUT_STARTED") {
                client->for_each([&](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
                    ptr->OnStreamStarted();
                });
            }
            else if (state == "OBS_WEBSOCKET_OUTPUT_STOPPED") {
                client->for_each([&](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
                    ptr->OnstreamStopped();
                });
            }
            else if (state == "OBS_WEBSOCKET_OUTPUT_STARTING") {
                client->for_each([&](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
                    ptr->OnStreamStarting();
                });
            }
            else if (state == "OBS_WEBSOCKET_OUTPUT_STOPPING") {
                client->for_each([&](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
                    ptr->OnStreamStopping();
                });
            }
        }
    }
    catch (std::exception& e) {
        //LOG_WARNING(e.what);
    }
}

void handle_response(OBSWebSocketClient* client, json& data) {
    try {
        client->callCacheCallback(data["requestId"],data["requestStatus"]["code"], data["requestStatus"]["result"]);
    }
    catch (std::exception& e) {
        //LOG_WARNING(e.what);
    }
}

static int callback(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len) {
    OBSWebSocketClient* client = static_cast<OBSWebSocketClient*>(user);
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        std::cout << "WebSocket connection established." << std::endl;
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE: {
        std::string message(static_cast<char*>(in), len);
        json response = json::parse(message);
        int op = response["op"];
        std::cout << response.dump() << std::endl;;
        switch (op) {
        case 0:
            handle_hello(client,response["d"]);
            break;
        case 2:
            handle_connected(client, response["d"]);
            break;
        case 5:
            handle_operator(client, response["d"]);
        case 7:
            handle_response(client, response["d"]);
            break;
        };
        break;
    }
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        // 保持连接活跃
        break;
    case LWS_CALLBACK_WSI_DESTROY:
        std::cout << "WebSocket disconnect." << std::endl;
        client->for_each([](std::shared_ptr<OBSWebSocketClientObserver>& ptr) {
            ptr->OnDisconnected();
        });
        client->_state = OBSWebSocketClient::State::Disconnected;
        break;
    default:
        break;
    }
    return 0;
}

void OBSWebSocketClient::connect(const std::function<void(bool)>& callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _taskQueue.push_back([this, callback]() {

        if (_state != State::Disconnected) {
            callback(false);
            return;
        }
        _state = State::Connecting;

        // Parse serverUri (e.g., "ws://localhost:4455")
        size_t protocolEnd = _serverUri.find("://");
        if (protocolEnd == std::string::npos) {
            std::cerr << "Invalid server URI format." << std::endl;
            callback(false);
            _state = State::Disconnected;
            return;
        }

        std::string address = _serverUri.substr(protocolEnd + 3);
        size_t portSeparator = address.find(':');
        std::string host = (portSeparator == std::string::npos) ? address : address.substr(0, portSeparator);
        int port = (portSeparator == std::string::npos) ? 4455 : std::stoi(address.substr(portSeparator + 1));

        struct lws_client_connect_info connectInfo = { 0 };
        connectInfo.context = (lws_context*)_context;
        connectInfo.address = host.c_str();
        connectInfo.port = port;
        connectInfo.path = "/";
        connectInfo.host = connectInfo.address;
        connectInfo.origin = connectInfo.address;
        connectInfo.protocol = "obs";
        connectInfo.userdata = this;

        _wsi = lws_client_connect_via_info(&connectInfo);
        if (!_wsi) {
            std::cerr << "Failed to connect to OBS WebSocket." << std::endl;
            callback(false);
            _state = State::Disconnected;
            return;
        }
        callback(true);
    });
}

void OBSWebSocketClient::disconnect(const std::function<void(bool)>& callback) {
    std::lock_guard<std::mutex> lock(_mutex);
    _taskQueue.push_back([this,callback]() {
        if (_wsi) {
            lws_callback_on_writable((lws*)_wsi);
            lws_set_timeout((lws*)_wsi, PENDING_TIMEOUT_CLOSE_SEND, LWS_TO_KILL_SYNC);
            callback(true);
            return;
        }
        callback(false);
    });
}


void OBSWebSocketClient::startStreaming(const std::function<void(int, bool)>& callback) {
    json request = {
        {"op", 6},
        {"d", {
            {"requestType","StartStream"},
            {"requestId",""}
        }}
    };
    cacheApiCallback(&request, callback);
    sendRequest(&request);
}

void OBSWebSocketClient::stopStreaming(const std::function<void(int, bool)>& callback) {
    json request = {
        {"op", 6},
        {"d", {
            {"requestType","StopStream"},
            {"requestId",""}
        }}
    };
    cacheApiCallback(&request, callback);
    sendRequest(&request);
}


void OBSWebSocketClient::callCacheCallback(const std::string& requestId,int errorCode,bool successed)
{
    std::function<void(int, bool)> fun = nullptr;
    {
        std::lock_guard<std::mutex> lock(_apiMutex);
        auto it = _apiCache.find(requestId);
        if (it != _apiCache.end()) {
            fun = it->second;
            _apiCache.erase(it);
        }
    }
    if (fun) {
        fun(errorCode, successed);
    }
}

void OBSWebSocketClient::sendHello(int version,const std::string& auth)
{
    std::string requestStr;
    if (!auth.empty()) {
        json msg = {
            {"op", 1},
            {"d", {
                {"rpcVersion", version},
                {"authentication",auth}
                }
            }
        };
        requestStr = msg.dump();
    }
    else {
        json msg = {
            {"op", 1},
            {
                "d", {
                {"rpcVersion", version},
                }
            }
        };
        requestStr = msg.dump();
    }
    _taskQueue.push_back([this, requestStr]() {
        unsigned char* buf = new unsigned char[LWS_PRE + requestStr.size()];
        memcpy(buf + LWS_PRE, requestStr.c_str(), requestStr.size());
        lws_write((lws*)_wsi, buf + LWS_PRE, requestStr.size(), LWS_WRITE_TEXT);
        delete[] buf;
    });
}

void OBSWebSocketClient::clearAllCallback()
{
    std::lock_guard<std::mutex> lock(_apiMutex);
    for (auto& it : _apiCache) {
        it.second(-5, false);
    }
    _apiCache.clear();
}

void OBSWebSocketClient::cacheApiCallback(const void* request,const std::function<void(int, bool)>& callback)
{
    auto id = std::to_string(generateTimestampMs());
    auto& r = *(json*)request;
    r["d"]["requestId"] = id;
    std::lock_guard<std::mutex> lock(_apiMutex);
    _apiCache[id] = callback;
}

void OBSWebSocketClient::run()
{
    struct lws_protocols protocols[] = {
            {"obs", callback, 0, 0},
            {nullptr, nullptr, 0, 0}
    };

    struct lws_context_creation_info info = { 0 };
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.user = this;

    _context = lws_create_context(&info);
    if (!_context) {
        std::cerr << "Failed to create WebSocket context." << std::endl;
        return;
    }

    while (_running) {
        if (_taskQueue.size() > 0) {
            _mutex.lock();
            auto fun = _taskQueue.front();
            _taskQueue.pop_front();
            _mutex.unlock();
            try {
                if (fun != nullptr) {
                    fun();
                };
            }
            catch (std::exception& e) {
                std::cerr << e.what();
            }
        }
        if (_running) {
            lws_service((lws_context*)_context, 5);
        }
    }

    lws_context_destroy((lws_context*)_context);
    _context = nullptr;
}

void OBSWebSocketClient::sendRequest(const void* request) {
    std::string requestStr = ((json*)request)->dump();
    std::lock_guard<std::mutex> lock(_mutex);
    _taskQueue.push_back([this, requestStr]() {

        if (_state != State::Connected) {
            clearAllCallback();
            return;
        }

        unsigned char* buf = new unsigned char[LWS_PRE + requestStr.size()];
        memcpy(buf + LWS_PRE, requestStr.c_str(), requestStr.size());
        lws_write((lws*)_wsi, buf + LWS_PRE, requestStr.size(), LWS_WRITE_TEXT);
        delete[] buf;
    });
}