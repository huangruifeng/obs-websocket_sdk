#pragma once
#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace hrtc {
class SingleThreaded {
public:
    virtual void lock(){};
    virtual void unlock(){};
};

using MultiThreaded = std::mutex;

template <class T>
class RawPointer {
public:
    RawPointer(T* data) : m_data(data) {}
    T* get()const { return m_data; }
    T* operator->() const { return m_data; }
    T& operator*() const { return *m_data; }

private:
    T* m_data;
};

template <class T,
          template <typename> class Ptr = std::shared_ptr,
          typename MutexPolicy = SingleThreaded,
          template <typename ELE, typename Alloc = std::allocator<ELE> >
          class Container = std::vector>
class Collections {
public:
    typedef T Type;
    typedef Ptr<T> PtrType;

    int32_t AddElement(PtrType observer) {
        if (observer.get() == nullptr)
            return -1;
        std::lock_guard<MutexPolicy> lock(m_mutex);
        for (const auto& ob : m_elements) {
            if (observer.get() == ob.get()) {
                return -4;
            }
        }
        m_elements.push_back(observer);
        return 0;
    }
    int32_t RemoveElement(PtrType observer) {
        if (observer.get() == nullptr)
            return -1;
        std::lock_guard<MutexPolicy> lock(m_mutex);
        const auto it = std::find_if(m_elements.begin(), m_elements.end(),
            [observer](const PtrType ob) {
                return observer.get() == ob.get();
            });

        if (it != m_elements.end()) {
            m_elements.erase(it);
            return 0;
        }
        return -3;
    }

    Container<PtrType> Copy() {
        std::lock_guard<MutexPolicy> lock(m_mutex);
        return m_elements;
    }

    int32_t Size() {
        std::lock_guard<MutexPolicy> lock(m_mutex);
        return m_elements.size();
    }

    Container<PtrType> CopyObservers() {
        std::lock_guard<MutexPolicy> lock(m_mutex);
        return m_elements;
    }

    bool IsExist(Type* t){
        std::lock_guard<MutexPolicy> lock(m_mutex);
        for (const auto& ob : m_elements) {
            if(ob.get() == t)
                return true;
        }
        return false;
    }

    bool IsExist(PtrType ptr){
        return IsExist(ptr.get());
    }

    int32_t Foreach(std::function<void(PtrType)> fun) {
        std::lock_guard<MutexPolicy> lock(m_mutex);
        for (const auto& ob : m_elements) {
            fun(ob);
        }
        return 0;
    }

    ~Collections() = default;


protected:
    Container<PtrType> m_elements;
    MutexPolicy m_mutex;
};
} 