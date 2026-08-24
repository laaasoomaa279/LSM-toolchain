#ifndef LSM_PEEPHOLE_X86_HPP
#define LSM_PEEPHOLE_X86_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

class PeepholeX86 {
public:
    static std::vector<uint8_t> optimize(const std::vector<uint8_t>& code) {

        if (code.size() < 10) return code;

        std::vector<uint8_t> optimized = code;
        for (size_t i = 0; i + 9 < optimized.size(); ++i) {
            if (optimized[i] == 0x48 && optimized[i + 1] == 0xB8 &&
                optimized[i + 2] == 0x00 && optimized[i + 3] == 0x00 &&
                optimized[i + 4] == 0x00 && optimized[i + 5] == 0x00 &&
                optimized[i + 6] == 0x00 && optimized[i + 7] == 0x00 &&
                optimized[i + 8] == 0x00 && optimized[i + 9] == 0x00) {
                optimized[i]     = 0x31; optimized[i + 1] = 0xC0; // xor eax, eax
                optimized[i + 2] = 0x90; optimized[i + 3] = 0x90; // nop
                optimized[i + 4] = 0x90; optimized[i + 5] = 0x90; // nop
                optimized[i + 6] = 0x90; optimized[i + 7] = 0x90; // nop
                optimized[i + 8] = 0x90; optimized[i + 9] = 0x90; // nop
            }
        }
        return optimized;
    }
};

#endif
