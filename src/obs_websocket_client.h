#pragma once
#include <libwebsockets.h>
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include<mutex>
#endif
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <deque>
#include <map>
#include <condition_variable>
#include "collections.h"
#include "obs_websocket.h"


#if _WIN32
class WindowsMutex {
public:
    WindowsMutex() {
        InitializeCriticalSection(&criticalSection);
    }

    ~WindowsMutex() {
        DeleteCriticalSection(&criticalSection);
    }

    void lock() {
        EnterCriticalSection(&criticalSection);
    }

    void unlock() {
        LeaveCriticalSection(&criticalSection);
    }

private:
    CRITICAL_SECTION criticalSection;
};
#endif

class OBSWebSocketClient : public  OBSWebSocket {
public:
#if _WIN32
    using Mutex = WindowsMutex;
#else
    using Mutex = std::mutex;
#endif
    OBSWebSocketClient(const std::string& serverUri);
    ~OBSWebSocketClient();

    enum class State {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2
    };
    State _state;
    std::string _pwd;

    void set_password(const std::string& pwd) override { _pwd = pwd; };
    void connect(const std::function<void(bool)>& callback) override;
    void disconnect(const std::function<void(bool)>& callback) override;
    void startStreaming(const std::function<void(int,bool)>& callback)override;
    void stopStreaming(const std::function<void(int, bool)>& callback)override;
    void requestStreamStatus() override;


    int32_t for_each(const std::function<void(std::shared_ptr<OBSWebSocketClientObserver>& t)> fun) {
        return _observes.Foreach(fun);
    }

    void add_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs)override {
        _observes.AddElement(obs);
    }

    void remove_observer(const std::shared_ptr<OBSWebSocketClientObserver>& obs)override {
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
    Mutex _apiMutex;
    std::map <std::string, std::function<void(int, bool)>> _apiCache;
    std::atomic<bool> _running;
    std::string _serverUri;
    Mutex _mutex;
    std::deque<std::function<void()>> _taskQueue;
    std::thread _thread;
    lws_context* _context;
    void* _wsi;
    hrtc::Collections<OBSWebSocketClientObserver, std::shared_ptr, Mutex> _observes;
};
