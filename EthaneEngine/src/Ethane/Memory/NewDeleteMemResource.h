//
//  NewDeleteResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/5.
//

#pragma once
#include "MemoryResource.h"
#include <memory_resource>

namespace Ethane {

    struct NewDeleteMemResource {
        // Point to the standard global new_delete_resource
        std::pmr::memory_resource* m_Upstream = std::pmr::new_delete_resource();
        
        void* Allocate(std::size_t bytes, std::size_t alignment) {
            return m_Upstream->allocate(bytes, alignment);
        }
        
        void Deallocate(void* p, std::size_t bytes, std::size_t alignment) {
            m_Upstream->deallocate(p, bytes, alignment);
        }
        
        void* Reallocate(void* p,
                         std::size_t oldBytes,
                         std::size_t newBytes,
                         std::size_t alignment)
        {
            void* q = Allocate(newBytes, alignment);
            std::memcpy(q, p, std::min(oldBytes, newBytes));
            Deallocate(p, oldBytes, alignment);
            return q;
        }
    };

    inline NewDeleteMemResource* NewDeleteMemResource()
    {
        static struct NewDeleteMemResource instance;
        return &instance;
    }

}
