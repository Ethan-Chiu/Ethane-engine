//
//  ReportMemResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/6.
//

#pragma once
#include "Ethane/Core/Log.h"
#include "MemoryResource.h"
#include "Ethane/Core/Assert.h"

namespace Ethane {

    template <CMemoryResource T>
    class ReportMemResource
    {
    public:
        ReportMemResource(std::string name, T* upstream, bool verbose = false)
        : m_Name(std::move(name)), m_Upstream(upstream), m_TotalAllocated(0), m_Verbose(verbose) {
            ETH_CORE_ASSERT(m_Upstream);
        }
        
        ~ReportMemResource() {
            if (m_TotalAllocated != 0) {
                ETH_CORE_WARN("{} Memory Resource destroyed with {} bytes still allocated",
                              m_Name, m_TotalAllocated);
            } else {
                ETH_CORE_INFO("{} Memory Resource destroyed with all memory freed", m_Name);
            }
        }

        // Rule of 6
        ReportMemResource(const ReportMemResource&) = delete;
        ReportMemResource& operator=(const ReportMemResource&) = delete;
        ReportMemResource(ReportMemResource&&) = delete;
        ReportMemResource& operator=(ReportMemResource&&) = delete;

    private:
        std::string m_Name;
        T* m_Upstream;

        std::unordered_map<void*, size_t> m_Allocations;
        size_t m_TotalAllocated;
        std::mutex m_Mutex;
        bool m_Verbose;

        std::string FormatDestroyedBytes(std::byte* p, const std::size_t size) {
            std::string result = "";
            bool inString = false;
            
            auto formatChar = [](bool& inString, const char c, const char next) {
                auto formatByte = [](const char byte) {
                    return fmt::format(" {:02x}", static_cast<unsigned char>(byte));
                };
                
                if (std::isprint(static_cast<int>(c))) {
                    if (!inString) {
                        if (std::isprint(static_cast<int>(next))) {
                            inString = true;
                            return fmt::format(" \"{}", c);
                        } else {
                            return formatByte(c);
                        }
                    } else {
                        return std::string(1, c);
                    }
                } else {
                    if (inString) {
                        inString = false;
                        return '"' + formatByte(c);
                    }
                    return formatByte(c);
                }
            };
            
            std::size_t pos = 0;
            for (; pos < std::min(size - 1, static_cast<std::size_t>(32)); ++pos) {
                result += formatChar(inString, static_cast<char>(p[pos]),
                                     static_cast<char>(p[pos + 1]));
            }
            result += formatChar(inString, static_cast<char>(p[pos]), 0);
            if (inString) {
                result += '"';
            }
            if (pos < (size - 1)) {
                result += " <truncated...>";
            }
            return result;
        }
        
    public:
        void* Allocate(std::size_t bytes, std::size_t alignment) {
            if (m_Verbose)
                ETH_CORE_TRACE("{} Allocates Size={}, Alignment={}", m_Name, bytes, alignment);
            else
                ETH_CORE_TRACE("{} A: {}", m_Name, bytes);

            auto result = m_Upstream->Allocate(bytes, alignment);

            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Allocations[result] = bytes;
                m_TotalAllocated += bytes;
            }

            if (m_Verbose)
                ETH_CORE_TRACE("{} Allocates at Address={}", m_Name, result);

            return result;
        }
        
        void Deallocate(void* p, std::size_t bytes, std::size_t alignment) {
            if (m_Verbose)
                ETH_CORE_TRACE(
                           "{} Deallocates Address={} Size={} Alignment={} Data={}",
                           m_Name, p, bytes, alignment,
                           FormatDestroyedBytes(static_cast<std::byte*>(p), bytes));
            else
                ETH_CORE_TRACE("{} D: {}", m_Name, bytes);

            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                auto it = m_Allocations.find(p);
                if (it != m_Allocations.end()) {
                    m_TotalAllocated -= it->second;
                    m_Allocations.erase(it);
                } else {
                    ETH_CORE_WARN("Deallocating unknown pointer {}", p);
                }
            }

            m_Upstream->Deallocate(p, bytes, alignment);
        }
        
    };

}
