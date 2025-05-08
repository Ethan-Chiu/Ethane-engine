//
//  ReportMemResource.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/5/6.
//

#pragma once
#include "MemoryResource.h"
#include "Ethane/Core/Assert.h"
#include <memory_resource>

namespace Ethane {

    template <CMemoryResource T>
    class ReportMemResource
    {
    public:
        ReportMemResource(std::string name, T* upstream)
        : m_Name(std::move(name)), m_Upstream(upstream) {
            ETH_CORE_ASSERT(m_Upstream);
        }
        
    private:
        std::string m_Name;
        T* m_Upstream;
        
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
            ETH_CORE_TRACE("{} Allocates Size={}, Alignment={}", m_Name, bytes, alignment);
            auto result = m_Upstream->Allocate(bytes, alignment);
            ETH_CORE_TRACE("{} Allocates at Address={}", m_Name, result);
            return result;
        }
        
        void Deallocate(void* p, std::size_t bytes, std::size_t alignment) {
            ETH_CORE_TRACE(
                           "{} Deallocates Address={} Size={} Alignment={} Data={}",
                           m_Name, p, bytes, alignment,
                           FormatDestroyedBytes(static_cast<std::byte*>(p), bytes));
            m_Upstream->Deallocate(p, bytes, alignment);
        }
        
    };

}
