#pragma once
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <deque>
#include <mutex>
#include <map>
#include <condition_variable>
#include "collections.h"

class OBSWebSocketClientObserver {
public:
    virtual ~OBSWebSocketClientObserver() = default;
    virtual void OnConnected(){}
    virtual void OnDisconnected(){}
    virtual void OnStreamStarting(){}
    virtual void OnStreamStarted(){}
    virtual void OnStreamStopping(){}
    virtual void OnstreamStopped(){}
};

class OBSWebSocketClient {
public:
    OBSWebSocketClient(const std::string& serverUri);
    ~OBSWebSocketClient();

    enum class State {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2
    };
    State _state;
    std::string _pwd;

    void setPassword(const std::string& pd) {
        _pwd = pd;
    }

    void connect(const std::function<void(bool)>& callback);
    void disconnect(const std::function<void(bool)>& callback);
    void startStreaming(const std::function<void(int,bool)>& callback);
    void stopStreaming(const std::function<void(int, bool)>& callback);


    int32_t for_each(const std::function<void(std::shared_ptr<OBSWebSocketClientObserver>& t)> fun) {
        _observes.Foreach(fun);
    }

    void add_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs) {
        _observes.AddElement(obs);
    }

    void remove_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs) {
        _observes.RemoveElement(obs);
    }

    void callCacheCallback(const std::string& requestId,int errorCode, bool successed);
    void sendHello(int version,const std::string& auth);
protected:
    void clearAllCallback();
    void cacheApiCallback(const void* request,const std::function<void(int, bool)>& callback);
    void sendRequest(const void* request);
    void run();
private:
    std::mutex _apiMutex;
    std::map <std::string, std::function<void(int, bool)>> _apiCache;
    std::atomic<bool> _running;
    std::string _serverUri;
    std::mutex _mutex;
    std::deque<std::function<void()>> _taskQueue;
    std::thread _thread;
    void* _context;
    void* _wsi;
    hrtc::Collections<OBSWebSocketClientObserver, std::shared_ptr, hrtc::MultiThreaded> _observes;
};