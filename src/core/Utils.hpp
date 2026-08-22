#ifndef LSM_CORE_UTILS_HPP
#define LSM_CORE_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <string>
#include <unordered_map>

namespace LSM::Core::Utils {

    
    constexpr uint64_t hashString(std::string_view str) {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : str) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    constexpr bool isPowerOfTwo(size_t x) {
        return x && ((x & (x - 1)) == 0);
    }

    constexpr size_t alignUp(size_t value, size_t alignment) {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    inline void fastCopy(void* dest, const void* src, size_t bytes) {
        std::memcpy(dest, src, bytes);
    }

    
    class StringInternPool {
    private:
        std::unordered_map<uint64_t, std::string> pool;

    public:
        const char* intern(std::string_view str) {
            uint64_t hash = hashString(str);
            auto it = pool.find(hash);
            if (it != pool.end()) {
                return it->second.c_str();
            }
            auto res = pool.emplace(hash, std::string(str));
            return res.first->second.c_str();
        }

        void clear() { pool.clear(); }
    };
}

#endif 