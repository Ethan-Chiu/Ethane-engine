//
//  MemoryResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/5.
//

#pragma once

#include <cstddef>
#include <type_traits>

template<typename T>
concept CMemoryResource = requires(T& mr,
                                   std::size_t bytes,
                                   std::size_t alignment,
                                   void* p)
{
    { mr.Allocate(bytes, alignment) } -> std::same_as<void*>;
    { mr.Deallocate(p, bytes, alignment) } -> std::same_as<void>;
    // Optional reallocate (fall-back ok)
    // { mr.Reallocate(p, oldBytes, newBytes, alignment) }
    //     -> std::same_as<void*>;
    // Optional reset
    // { mr.Reset() } -> std::same_as<void>;
};
