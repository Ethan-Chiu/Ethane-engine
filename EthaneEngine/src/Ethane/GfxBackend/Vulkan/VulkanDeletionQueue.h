#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Ethane {

// Keeps Vulkan objects alive until the GPU can no longer be using them. A resource being destroyed
// retires its handles here, tagged with the frame serial being recorded (or last submitted); they're
// destroyed once that frame is known to have finished, i.e. when frame serial + kMaxFramesInFlight
// starts (its slot's fence was waited on). Thread-safe.
class VulkanDeletionQueue
{
public:
    // Runs a function when retired objects are destroyed, e.g. to free a handle owned elsewhere
    struct Call
    {
        std::function<void()> Fn;
        Call(std::function<void()> fn) : Fn(std::move(fn)) {}
        Call(Call&& other) noexcept : Fn(std::exchange(other.Fn, nullptr)) {}
        Call& operator=(Call&&) = delete;
        ~Call() { if (Fn) Fn(); }
    };

    // Takes the objects (raii handles, vectors of them, Calls...); they're destroyed in argument order
    template<typename... T>
    void Retire(uint64_t serial, T&&... objects)
    {
        static_assert((!std::is_lvalue_reference_v<T> && ...), "retire by std::move (the queue owns them)");
        auto entry = std::make_unique<Objects<std::decay_t<T>...>>(std::forward<T>(objects)...);
        std::lock_guard lock(m_Mutex);
        m_Entries.push_back({ serial, std::move(entry) });
    }

    // Destroys everything retired at or before `completedSerial`, oldest first
    void Collect(uint64_t completedSerial)
    {
        std::deque<Entry> done;
        {
            std::lock_guard lock(m_Mutex);
            while (!m_Entries.empty() && m_Entries.front().Serial <= completedSerial) {
                done.push_back(std::move(m_Entries.front()));
                m_Entries.pop_front();
            }
        }
        // destroyed outside the lock, in retirement order (a destructor may retire more)
        while (!done.empty()) {
            done.pop_front();
        }
    }

    // Destroys everything: only once the GPU is idle
    void Flush() { Collect(UINT64_MAX); }

    size_t Size() const
    {
        std::lock_guard lock(m_Mutex);
        return m_Entries.size();
    }

private:
    struct Base { virtual ~Base() = default; };

    template<typename... T>
    struct Objects final : Base
    {
        explicit Objects(T&&... objects) : Items(std::optional<T>(std::move(objects))...) {}
        ~Objects() override { std::apply([](auto&... item) { (item.reset(), ...); }, Items); } // in order
        std::tuple<std::optional<T>...> Items;
    };

    struct Entry
    {
        uint64_t Serial = 0;
        std::unique_ptr<Base> Payload;
    };

    mutable std::mutex m_Mutex;
    std::deque<Entry> m_Entries; // serials never decrease, so the oldest are in front
};

}
