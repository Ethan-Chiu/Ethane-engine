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
                                   void* p,
                                   std::size_t oldBytes,
                                   std::size_t newBytes)
{
    // Raw allocate/deallocate
    { mr.Allocate(bytes, alignment) } -> std::same_as<void*>;
    { mr.Deallocate(p, bytes, alignment) } -> std::same_as<void>;
    // Optional reallocate (fall-back ok)
    { mr.Reallocate(p, oldBytes, newBytes, alignment) }
        -> std::same_as<void*>;
    // Optional reset
    { mr.Reset() } -> std::same_as<void>;
};


// Alternative style
// concept CMemoryResource = requires(R r) {
//    { r.Allocate(std::declval<uint32_t>(), std::declval<uint32_t>()) } -> std::same_as<typename T::TextureHandle>;
// }

// Alternative style
// template<typename T>
// concept CMemoryResource =
//    requires(T r) {
// requires std::invocable<decltype(&T::Allocate), T&, size_t, size_t>;
