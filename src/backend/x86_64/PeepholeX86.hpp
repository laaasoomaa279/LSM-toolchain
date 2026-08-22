#ifndef LSM_PEEPHOLE_X86_HPP
#define LSM_PEEPHOLE_X86_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

class PeepholeX86 {
public:
    static std::vector<uint8_t> optimize(const std::vector<uint8_t>& code) {
        if (code.size() < 3) return code;

        std::vector<uint8_t> optimized;
        optimized.reserve(code.size());

        size_t i = 0;
        while (i < code.size()) {
            
            if (i + 9 < code.size() &&
                code[i] == 0x48 && code[i + 1] == 0xB8 &&
                code[i + 2] == 0x00 && code[i + 3] == 0x00 &&
                code[i + 4] == 0x00 && code[i + 5] == 0x00 &&
                code[i + 6] == 0x00 && code[i + 7] == 0x00 &&
                code[i + 8] == 0x00 && code[i + 9] == 0x00) {
                
                optimized.push_back(0x31);
                optimized.push_back(0xC0);
                i += 10;
                continue;
            }

            
            if (i + 3 < code.size() &&
                code[i] == 0x48 && code[i + 1] == 0x89 && code[i + 2] == 0xEC &&
                code[i + 3] == 0x90) {
                
                optimized.push_back(0x48);
                optimized.push_back(0x89);
                optimized.push_back(0xEC);
                i += 4; 
                continue;
            }

            
            if (i + 1 < code.size() && code[i] == 0xEB && code[i + 1] == 0x00) {
                i += 2;
                continue;
            }

            optimized.push_back(code[i]);
            i++;
        }

        return optimized;
    }
};

#endif 