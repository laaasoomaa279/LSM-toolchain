#ifndef LSM_ELF_BUILDER_HPP
#define LSM_ELF_BUILDER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include "../regalloc/PhysicalRegister.hpp"

struct ELFSegment {
    uint32_t type;     
    uint32_t flags;    
    uint64_t vaddr;
    std::vector<uint8_t> data;
};

class ELFBuilder {
public:
    static void buildExecutable(const std::string& outputPath, 
                                const std::vector<ELFSegment>& segments, 
                                uint64_t entryPoint,
                                ArchType arch = ArchType::X86_64);

    static void buildExecutable(const std::string& outputPath, 
                                const std::vector<uint8_t>& machineCode,
                                ArchType arch = ArchType::X86_64);
};

#endif 