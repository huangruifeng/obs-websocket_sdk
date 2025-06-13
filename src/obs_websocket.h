#pragma once
#include <functional>
#include <memory>
#include <string>
#ifdef _WIN32
#define OBS_WEBSOCKET_SDK_EXPORT __declspec(dllexport)
#else
#define OBS_WEBSOCKET_SDK_EXPORT __attribute__((visibility("default")))
#endif


class OBS_WEBSOCKET_SDK_EXPORT OBSWebSocketClientObserver {
public:
    struct Status {
        bool StreamStarted;
        bool StreamReconnecting;
    };
    virtual ~OBSWebSocketClientObserver() = default;
    virtual void OnConnected() {}
    virtual void OnDisconnected() {}
    virtual void OnStreamStarting() {}
    virtual void OnStreamStarted() {}
    virtual void OnStreamStopping() {}
    virtual void OnstreamStopped() {}
    virtual void OnstreamStatus(const Status& status){}


};


class OBS_WEBSOCKET_SDK_EXPORT OBSWebSocket {
public:
    ~OBSWebSocket() = default;
    
    virtual void connect(const std::function<void(bool)>& callback) = 0;
    virtual void disconnect(const std::function<void(bool)>& callback) = 0;
    virtual void startStreaming(const std::function<void(int, bool)>& callback) = 0;
    virtual void stopStreaming(const std::function<void(int, bool)>& callback) = 0;
    virtual void requestStreamStatus() = 0;
    virtual void add_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs) = 0;
    virtual void remove_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs) = 0;
};


OBS_WEBSOCKET_SDK_EXPORT std::shared_ptr<OBSWebSocket> CreateOBSWebSocketClient(const std::string& serverUri);