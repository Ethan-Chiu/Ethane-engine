//
//  LinearMemResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/6.
//

#pragma once
#include "Ethane/Core/Log.h"
#include "MemoryResource.h"
#include "NewDeleteMemResource.h"

namespace Ethane {

template<CMemoryResource T>
class LinearMemResource
{
    constexpr const static std::size_t s_DefaultInitialSize = 64;

public:
    explicit LinearMemResource(T* upstream) noexcept
      : m_Upstream(upstream)
    {
        ETH_CORE_DEBUG_ASSERT(m_Upstream != nullptr, "Constructing LinearMemResource with upstream being nullptr.");
    }
    
    LinearMemResource()
    : LinearMemResource(NewDeleteMemResource()) {};

    LinearMemResource(std::size_t initialSize, T* upstream) noexcept
      : m_Upstream(upstream), m_RemainSize(initialSize)
    {
        ETH_CORE_DEBUG_ASSERT(m_Upstream != nullptr, "Constructing LinearMemResource with upstream being nullptr.");
        ETH_CORE_DEBUG_ASSERT(m_RemainSize > 0, "Size needs to be greater than zero.");
    }
    
    explicit LinearMemResource(std::size_t initialSize) noexcept
      : LinearMemResource(initialSize, NewDeleteMemResource())
    {
    }


    LinearMemResource(void* buffer, std::size_t bufferSize, T* upstream)
      : m_Upstream(upstream)
      , m_Top(buffer)
      , m_RemainSize(bufferSize)
      , m_OrigBuf(buffer)
      , m_OrigSize(bufferSize)
    {
    }
    
    LinearMemResource(void* buffer, std::size_t bufferSize)
      : m_Upstream(NewDeleteMemResource())
      , m_Top(buffer)
      , m_RemainSize(bufferSize)
      , m_OrigBuf(buffer)
      , m_OrigSize(bufferSize)
    {
    }

    // No copy
    LinearMemResource(const LinearMemResource&) = delete;
    LinearMemResource& operator=(const LinearMemResource&) = delete;
    ~LinearMemResource() { Reset(); }

private:
	class _Chunk
    {
    public:
        // Allocate a new chunk from upstream and push it to the front of the list.
        // Returns { buffer_start, usable_size }.
        static std::pair<void*, std::size_t>
        Allocate(
            T* upstream,
            std::size_t size,
            std::size_t alignment,
            _Chunk*& head
        )
        {
            ETH_CORE_DEBUG_ASSERT(upstream != nullptr, "Upstream is null.");

            const std::size_t originalSize = size;

            // Add space for _Chunk metadata
            size = AlignUp(size + sizeof(_Chunk), s_ChunkAlignment);

            // Detect unsigned overflow
            if (size < originalSize) [[unlikely]]
            {
                // Linear allocator is not allowed to throw
                size = static_cast<std::size_t>(-1);
                alignment = ~(static_cast<std::size_t>(-1) >> 1);
            }

            void* buffer = upstream->Allocate(size, alignment);
            ETH_CORE_DEBUG_ASSERT(buffer != nullptr, "Chunk allocation failed.");

            // Place _Chunk metadata at the end of the buffer
            void* const chunkAddr =
                static_cast<uint8_t*>(buffer) + size - sizeof(_Chunk);

            head = ::new (chunkAddr) _Chunk(size, alignment, head);

            return { buffer, size - sizeof(_Chunk) };
        }

        // Release all chunks in the linked list
        static void
        Release(_Chunk*& head, T* upstream) noexcept
        {
            ETH_CORE_DEBUG_ASSERT(upstream != nullptr, "Upstream is null.");

            _Chunk* current = head;
            while (current)
            {
                _Chunk* next = current->m_Next;

                const std::size_t size  = current->m_Size;
                const std::size_t align = current->m_Alignment;

                // Compute original allocation start
                void* buffer =
                    reinterpret_cast<uint8_t*>(current + 1) - size;

                upstream->Deallocate(buffer, size, align);
                current = next;
            }
            head = nullptr;
        }

    private:
        _Chunk(
            std::size_t size,
            std::size_t alignment,
            _Chunk* next
        ) noexcept
            : m_Size(size)
            , m_Alignment(alignment)
            , m_Next(next)
        {}

    private:
        static constexpr std::size_t s_ChunkAlignment = 64;

        std::size_t m_Size;
        std::size_t m_Alignment;
        _Chunk*     m_Next;
    };



public:
	void* Allocate(std::size_t bytes, std::size_t alignment) {
		ETH_CORE_DEBUG_ASSERT(bytes > 0, "Cannot allocate zero bytes.");

		alignment = std::max(alignment, alignof(std::max_align_t));

		uintptr_t raw = reinterpret_cast<uintptr_t>(m_Top);
		uintptr_t aligned = AlignUp(raw, alignment);
		std::size_t padding = aligned - raw;

		if (padding + bytes <= m_RemainSize)
		{
			void* result = reinterpret_cast<void*>(aligned);
			m_Top = reinterpret_cast<void*>(aligned + bytes);
			m_RemainSize -= padding + bytes;
			
			// Update stats
			m_StatTotalRequested += bytes;
			m_StatTotalAllocated += padding + bytes;
			m_StatHighWater = std::max(m_StatHighWater, m_StatTotalAllocated);
			
			return result;
		}

		// Slow path: need a new chunk
		const std::size_t requiredSize = AlignUp(bytes, alignment);
		const std::size_t newChunkSize = std::max(m_NextBufSize, requiredSize);
		
		ETH_CORE_TRACE("Allocating new memory chunk with size {}", newChunkSize);
        m_NextBufSize = static_cast<std::size_t>(m_NextBufSize * s_GrowthFactor);

		auto [buffer, usableSize] =
			_Chunk::Allocate(m_Upstream, newChunkSize, alignment, m_ChunkHead);

		void* result = buffer;
        m_Top = static_cast<uint8_t*>(buffer) + bytes;
        m_RemainSize = usableSize - bytes;
        
        // Update stats
        m_StatTotalRequested += bytes;
        m_StatTotalAllocated += bytes;
		
        m_StatHighWater = std::max(m_StatHighWater, m_StatTotalAllocated);

        return result;
	}
    void Deallocate(void* p, std::size_t bytes, std::size_t alignment) {};
    void Reset() noexcept {
		if (m_StatTotalAllocated > 0) {
			float utilization = (m_StatTotalRequested * 100.0f) / m_StatTotalAllocated;
			ETH_CORE_TRACE(
			  "[Linear Memory Resource] Peak={}, Requested={}, Allocated={}, Utilization={:.1f}%",
			  m_StatHighWater, m_StatTotalRequested, m_StatTotalAllocated, utilization);
		}

        _Chunk::Release(m_ChunkHead, m_Upstream);

        if (m_OrigBuf)
        {
            m_Top = m_OrigBuf;
            m_RemainSize = m_OrigSize;
        }
        else
        {
            m_Top = nullptr;
            m_RemainSize = 0;
        }

        m_StatHighWater = 0;
        m_StatTotalRequested = 0;
        m_StatTotalAllocated = 0;
	}

private:
    static constexpr size_t s_InitBufsize = 128 * sizeof(void*);
    static constexpr float s_GrowthFactor = 1.5;
	size_t m_NextBufSize = s_InitBufsize;

    T* m_Upstream;
    void* const m_OrigBuf = nullptr;
    size_t const m_OrigSize = s_InitBufsize;

    void* m_Top = nullptr;
    std::size_t m_RemainSize = 0;

    // Stats
    std::size_t m_StatHighWater = 0;
    std::size_t m_StatTotalRequested = 0;  // Total bytes requested by user
    std::size_t m_StatTotalAllocated = 0;  // Total bytes actually allocated (with padding)

private:
    _Chunk* m_ChunkHead = nullptr;
};

} // namespace Ethane
