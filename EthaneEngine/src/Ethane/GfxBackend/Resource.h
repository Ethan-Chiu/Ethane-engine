//
//  Resource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/27.
//
#pragma once

#include <atomic>
#include <cstddef>
#include <iostream>

namespace Ethane {

template <typename T>
concept CResource = requires(T* p)
{
    { p->AddRef() } noexcept;
    { p->Release() } noexcept;
};


template <bool ThreadSafe>
struct CounterPolicy
{
    using CounterType = std::conditional_t<ThreadSafe, std::atomic<std::size_t>, std::size_t>;
    
    static void Increment(CounterType& c) noexcept {
        if constexpr (ThreadSafe) c.fetch_add(1, std::memory_order_relaxed);
        else ++c;
    }
    static std::size_t Decrement(CounterType& c) noexcept {
        if constexpr (ThreadSafe) return c.fetch_sub(1, std::memory_order_acq_rel);
        else return --c;
    }
};


template <typename T>
struct DefaultDeleter
{
    void operator()(T* ptr) const noexcept { delete ptr; }
};


template <
    typename Derived,
    bool ThreadSafe = false,
    bool Debug = false,
    typename Deleter = DefaultDeleter<Derived>
>
class RefCounted
{
    using CounterType = typename CounterPolicy<ThreadSafe>::CounterType;
    CounterType m_RefCount{0};
    
    // Debug: count how many objects are alive.
    static inline std::size_t s_DebugLiveCount = 0;
    
protected:
    // Protected destructor: only called via deleter.
    ~RefCounted() noexcept {
        if constexpr (Debug) {
            --s_DebugLiveCount;
            if (s_DebugLiveCount == 0) {
                // TODO: change this to use my logger
                std::cerr << "[Debug] All resources freed (no leaks).\n";
            }
        }
    }
    
public:
    RefCounted() noexcept {
        if constexpr (Debug) {
            ++s_DebugLiveCount;
        }
    }
    
    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;
    
    void AddRef() noexcept {
        CounterPolicy<ThreadSafe>::Increment(m_RefCount);
    }
    
    void Release() noexcept {
        std::size_t prev = CounterPolicy<ThreadSafe>::Decrement(m_RefCount);
        // If non-atomic, prev==new count; if atomic, prev==old count.
        if ((ThreadSafe ? (prev == 1) : (prev == 0))) {
            // Last reference: invoke custom deleter.
            Deleter deleter;
            deleter(static_cast<Derived*>(this));
        }
    }
    
    // (Optional) inspect current count.
    std::size_t UseCount() const noexcept { return m_RefCount; }
};


template <CResource T>
class RefCountPtr
{
    template <CResource> friend class RefCountPtr;
    
private:
    T* ptr;
    
public:
    RefCountPtr() noexcept : ptr(nullptr) {}
    
    // Explicit when taking ownership
    explicit RefCountPtr(T* p, bool add_ref = true) : ptr(p) {
        if(ptr && add_ref) ptr->AddRef();
    }
    
    // Copy
    RefCountPtr(const RefCountPtr& other) noexcept : ptr(other.ptr) {
        if (ptr) ptr->AddRef();
    }
    
    // Templated copy (Derived->Base)
    template<typename U>
    RefCountPtr(const RefCountPtr<U>& other) noexcept
      requires(std::convertible_to<U*, T*>)
      : ptr(other.ptr) {
        if(ptr) ptr->AddRef();
    }

    // Move
    RefCountPtr(RefCountPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    // Templated move (Derived->Base)
    template<typename U>
    RefCountPtr(RefCountPtr<U>&& other) noexcept
      requires(std::convertible_to<U*, T*>)
      : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    // Copy assignment
    RefCountPtr& operator=(const RefCountPtr& other) noexcept {
        if (this != &other) {
            if (other.ptr) other.ptr->AddRef();
            if (ptr) ptr->Release();
            ptr = other.ptr;
        }
        return *this;
    }
    
    // Templated assignment (Derived->Base)
    template<typename U>
    RefCountPtr& operator=(const RefCountPtr<U>& other) noexcept
      requires(std::convertible_to<U*, T*>) {
        if(ptr != other.ptr) {
            if(other.ptr) other.ptr->AddRef();
            if(ptr) ptr->Release();
            ptr = other.ptr;
        }
        return *this;
    }

    // Move assignment
    RefCountPtr& operator=(RefCountPtr&& other) noexcept {
        if (this != &other) {
            if (ptr) ptr->Release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    // Templated move assignment
    template<typename U>
    RefCountPtr& operator=(RefCountPtr<U>&& other) noexcept
      requires(std::convertible_to<U*, T*>) {
        if(ptr != other.ptr) {
            if(ptr) ptr->Release();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ~RefCountPtr() noexcept {
        if (ptr) ptr->Release();
    }
    
    T* get() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }
    explicit operator bool() const noexcept { return ptr != nullptr; }
    
    void reset(T* p = nullptr, bool add_ref = true) noexcept {
        if(p == ptr) return;
        if(p && add_ref) p->AddRef();
        if(ptr) ptr->Release();
        ptr = p;
    }
    
    void swap(RefCountPtr& other) noexcept {
        std::swap(ptr, other.ptr);
    }
};


// Free functions for pointer casts:
// Static cast
template<typename T, typename U>
RefCountPtr<T> static_pointer_cast(const RefCountPtr<U>& r) noexcept {
    return RefCountPtr<T>(static_cast<T*>(r.get()));
}

// Const cast (remove constness)
template<typename T, typename U>
RefCountPtr<T> const_pointer_cast(const RefCountPtr<U>& r) noexcept {
    return RefCountPtr<T>(const_cast<T*>(r.get()));
}

// Dynamic cast (runtime-checked, returns null on failure)
template<typename T, typename U>
RefCountPtr<T> dynamic_pointer_cast(const RefCountPtr<U>& r) noexcept {
    return RefCountPtr<T>(dynamic_cast<T*>(r.get()));
}


template<typename T, typename... Args>
RefCountPtr<T> MakeRefCountPtr(Args&&... args) {
    T* raw = new T(std::forward<Args>(args)...);
    RefCountPtr<T> ptr(raw);
    return ptr;
}
}
