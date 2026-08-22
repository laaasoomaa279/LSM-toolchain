#ifndef LSM_BAREMETAL_UART_DRIVER_HPP
#define LSM_BAREMETAL_UART_DRIVER_HPP

#include <cstdint>
#include <cstddef>

namespace LSM::BareMetal {

    class UART {
    public:
#if defined(__x86_64__) || defined(_M_X64)
        
        static constexpr uint16_t COM1_PORT = 0x3F8;

        static inline void outb(uint16_t port, uint8_t val) {
            __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
        }

        static inline uint8_t inb(uint16_t port) {
            uint8_t ret;
            __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
            return ret;
        }

        static void init() {
            outb(COM1_PORT + 1, 0x00); 
            outb(COM1_PORT + 3, 0x80); 
            outb(COM1_PORT + 0, 0x03); 
            outb(COM1_PORT + 1, 0x00);
            outb(COM1_PORT + 3, 0x03); 
            outb(COM1_PORT + 2, 0xC7); 
            outb(COM1_PORT + 4, 0x0B);
        }

        static void putc(char c) {
            while ((inb(COM1_PORT + 5) & 0x20) == 0); 
            outb(COM1_PORT, static_cast<uint8_t>(c));
        }

#else
        
        static constexpr uintptr_t MMIO_BASE = 0x3F000000;
        static constexpr uintptr_t AUX_ENABLES = MMIO_BASE + 0x00215004;
        static constexpr uintptr_t AUX_MU_IO_REG = MMIO_BASE + 0x00215040;
        static constexpr uintptr_t AUX_MU_LSR_REG = MMIO_BASE + 0x00215054;

        static inline void mmio_write(uintptr_t reg, uint32_t data) {
            *reinterpret_cast<volatile uint32_t*>(reg) = data;
        }

        static inline uint32_t mmio_read(uintptr_t reg) {
            return *reinterpret_cast<volatile uint32_t*>(reg);
        }

        static void init() {
            mmio_write(AUX_ENABLES, mmio_read(AUX_ENABLES) | 1); 
        }

        static void putc(char c) {
            while (!(mmio_read(AUX_MU_LSR_REG) & 0x20)); 
            mmio_write(AUX_MU_IO_REG, static_cast<uint32_t>(c));
        }
#endif

        static void puts(const char* str) {
            if (!str) return;
            while (*str) {
                if (*str == '\n') putc('\r');
                putc(*str++);
            }
        }

        static void printHex(uint64_t val) {
            puts("0x");
            const char* hexChars = "0123456789ABCDEF";
            for (int i = 60; i >= 0; i -= 4) {
                putc(hexChars[(val >> i) & 0xF]);
            }
        }
    };

} 

#endif