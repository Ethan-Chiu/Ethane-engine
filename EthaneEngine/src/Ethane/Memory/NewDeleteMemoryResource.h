//
//  NewDeleteResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/5.
//

#pragma once
#include <memory_resource>

struct NewDeleteMemoryResource {
    // Point to the standard global new_delete_resource
    std::pmr::memory_resource* upstream = std::pmr::new_delete_resource();

    void* Allocate(std::size_t bytes, std::size_t alignment) {
        return upstream->allocate(bytes, alignment);
    }

    void Deallocate(void* p, std::size_t bytes, std::size_t alignment) {
        upstream->deallocate(p, bytes, alignment);
    }

    // Optional: just forward through to the upstream resource
    void* Reallocate(void* p,
                     std::size_t oldBytes,
                     std::size_t newBytes,
                     std::size_t alignment)
    {
        // new_delete_resource doesn’t provide reallocate itself,
        // but std::pmr::memory_resource defines a default do_reallocate
        // so we can allocate+copy+free:
        void* q = Allocate(newBytes, alignment);
        std::memcpy(q, p, std::min(oldBytes, newBytes));
        Deallocate(p, oldBytes, alignment);
        return q;
    }

    // Optional (noop for new/delete)
    void Reset() noexcept {}
};

static_assert(CMemoryResource<NewDeleteMemoryResource>, "NewDeleteResource should implement the CMemoryResource concept.");
