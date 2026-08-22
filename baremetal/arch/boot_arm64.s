.global _start
.extern main
.extern __bss_start
.extern __bss_end

.section .text.boot
_start:
    # 1. فحص معرّف النواة (Core ID) عبر MPIDR_EL1
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, .primary_core

.hang_secondary_cores:
    wfe
    b       .hang_secondary_cores

.primary_core:
    # 2. ضبط مؤشر المكدس عند عنوان البداية الفيزيائي
    ldr     x0, =_start
    mov     sp, x0

    # 3. مسح قسم .bss
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
.clear_bss_loop:
    cmp     x1, x2
    b.ge    .call_lsm_main
    str     xzr, [x1], #8
    b       .clear_bss_loop

.call_lsm_main:
    # 4. الانتقال إلى دالة main
    bl      main

.halt_arm:
    wfe
    b       .halt_arm