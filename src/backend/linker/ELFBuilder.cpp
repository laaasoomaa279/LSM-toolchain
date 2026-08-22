#include "ELFBuilder.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

#pragma pack(push, 1)
struct Elf32_Ehdr {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};
#pragma pack(pop)

void ELFBuilder::buildExecutable(const std::string& outputPath, 
                                const std::vector<ELFSegment>& segments, 
                                uint64_t entryPoint,
                                ArchType arch) {
    if (segments.empty()) {
        throw std::runtime_error("ELFBuilder: Cannot build an empty executable.");
    }

    Elf32_Ehdr ehdr = {};
    ehdr.e_ident[0] = 0x7F; ehdr.e_ident[1] = 'E'; ehdr.e_ident[2] = 'L'; ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 1; 
    ehdr.e_ident[5] = 1; 
    ehdr.e_ident[6] = 1;
    ehdr.e_ident[7] = 0;

    ehdr.e_type = 2; 
    ehdr.e_machine = (arch == ArchType::X86_32) ? 3 : 62; 
    ehdr.e_version = 1;
    ehdr.e_entry = static_cast<uint32_t>(entryPoint);
    ehdr.e_phoff = sizeof(Elf32_Ehdr);
    ehdr.e_shoff = 0; ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr.e_phentsize = sizeof(Elf32_Phdr);
    ehdr.e_phnum = static_cast<uint16_t>(segments.size());
    ehdr.e_shentsize = 0; ehdr.e_shnum = 0; ehdr.e_shstrndx = 0;

    std::vector<Elf32_Phdr> phdrs;
    uint32_t currentOffset = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Phdr) * segments.size());

    for (const auto& seg : segments) {
        Elf32_Phdr phdr = {};
        phdr.p_type = seg.type;
        phdr.p_flags = seg.flags;
        phdr.p_offset = currentOffset;
        phdr.p_vaddr = static_cast<uint32_t>(seg.vaddr);
        phdr.p_paddr = static_cast<uint32_t>(seg.vaddr);
        phdr.p_filesz = static_cast<uint32_t>(seg.data.size());
        phdr.p_memsz = static_cast<uint32_t>(seg.data.size());
        phdr.p_align = 0x1000;

        phdrs.push_back(phdr);
        currentOffset += seg.data.size();
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        throw std::runtime_error("ELFBuilder: Failed to open output file: " + outputPath);
    }

    outFile.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
    for (const auto& phdr : phdrs) {
        outFile.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
    }
    for (const auto& seg : segments) {
        outFile.write(reinterpret_cast<const char*>(seg.data.data()), seg.data.size());
    }
    outFile.close();
}

void ELFBuilder::buildExecutable(const std::string& outputPath, const std::vector<uint8_t>& machineCode, ArchType arch) {
    if (machineCode.empty()) {
        throw std::runtime_error("ELFBuilder: Machine code payload is empty.");
    }

    const uint32_t baseVAddr = 0x100000;
    const uint32_t headerSize = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);

    std::vector<uint8_t> payload;
    if (arch == ArchType::X86_32) {
        
        payload = {
            0x02, 0xB0, 0xAD, 0x1B, 
            0x00, 0x00, 0x00, 0x00, 
            0xFE, 0x4F, 0x52, 0xE4  
        };
    } else {
        
        payload = {
            0x02, 0xB0, 0xAD, 0x1B, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x4F, 0x52, 0xE4,
            0xFA, 0xBF, 0x00, 0x00, 0x20, 0x00, 0xB9, 0x00, 0x0C, 0x00, 0x00, 0x31, 0xC0, 0xF3, 0xAB,
            0xC7, 0x05, 0x00, 0x00, 0x20, 0x00, 0x03, 0x10, 0x20, 0x00,
            0xC7, 0x05, 0x00, 0x10, 0x20, 0x00, 0x03, 0x20, 0x20, 0x00,
            0xC7, 0x05, 0x00, 0x20, 0x20, 0x00, 0x83, 0x00, 0x00, 0x00,
            0xB8, 0x00, 0x00, 0x20, 0x00, 0x0F, 0x22, 0xD8, 0x0F, 0x20, 0xE0, 0x0D, 0x20, 0x00, 0x00, 0x00, 0x0F, 0x22, 0xE0,
            0xB9, 0x80, 0x00, 0x00, 0xC0, 0x0F, 0x32, 0x0D, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x30,
            0x0F, 0x20, 0xC0, 0x0D, 0x00, 0x00, 0x00, 0x80, 0x0F, 0x22, 0xC0,
            0x0F, 0x01, 0x15, 0xF9, 0x00, 0x10, 0x00, 0xEA, 0xC7, 0x00, 0x10, 0x00, 0x08, 0x00,
            0x66, 0xB8, 0x10, 0x00, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xE0, 0x8E, 0xE8, 0x8E, 0xD0,
            0x48, 0xBC, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x1E,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x20, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x17, 0x00, 0xE1, 0x00, 0x10, 0x00
        };
    }

    payload.insert(payload.end(), machineCode.begin(), machineCode.end());
    uint32_t entryPoint = baseVAddr + headerSize + 12;

    std::vector<ELFSegment> segs = { {1, 7, baseVAddr, payload} };
    buildExecutable(outputPath, segs, entryPoint, arch);
}