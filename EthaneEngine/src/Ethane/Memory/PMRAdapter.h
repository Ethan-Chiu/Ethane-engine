//
//  PMRAdapter.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/6.
//

#pragma once
#include "MemoryResource.h"
#include <memory_resource>

template<CMemoryResource T>
class PMRAdapter : public std::pmr::memory_resource {
public:
    explicit PMRAdapter(T& r) noexcept : resource(r) {}

protected:
    void* do_allocate(std::size_t b, std::size_t a) override {
        return resource.Allocate(b, a);
    }
    void  do_deallocate(void* p, std::size_t b, std::size_t a) override {
        resource.Deallocate(p, b, a);
    }
    bool  do_is_equal(const memory_resource& o) const noexcept override {
        auto* other = dynamic_cast<const PMRAdapter*>(&o);
        return other && std::addressof(other->resource) == std::addressof(resource);
    }

private:
    T& resource;
};
