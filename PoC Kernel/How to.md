# Freestanding Kernel Proof-of-Concept (PoC)

This PoC demonstrates unmediated hardware sovereignty by deploying a minimal freestanding kernel compiled directly via the **LSM** toolchain. It bootstraps directly into x86_64 long mode, configures the Mode 13h VGA linear framebuffer ($320 \times 200 \times 8\text{bpp}$) at physical address `0xA0000`, processes three-byte raw PS/2 mouse packet streams via legacy I/O ports (`0x64`/`0x60`), and renders a real-time interactive graphical interface.

---

## Prerequisites & Workspace Setup

Ensure all source files (`boot.s`, `kernel.lsm`) and the compiler binary (`lsm.exe` or `lsm`) reside within the same working directory:

* **LSM Toolchain:** Native `lsm` executable.
* **NASM:** Netwide Assembler for the Stage-0/Stage-1 bootloader (`boot.s`).


* **QEMU:** `qemu-system-x86_64` for bare-metal architectural emulation.
* **PowerShell:** For binary artifact assembly and image resizing.

---

## Build & Deployment Pipeline

**Step 1: Assemble the Custom Bootloader**
Assemble the 16-bit to 64-bit transition bootloader into a raw 512-byte MBR sector:

```powershell
nasm -f bin boot.s -o boot.bin

```

**Step 2: Compile the Freestanding Kernel**
Lower and compile the LSM source directly into a freestanding binary without runtime dependencies:

```powershell
./lsm build --baremetal kernel.lsm -o kernel.bin

```

**Step 3: Concatenate Binary Artifacts**
Merge the Master Boot Record and the compiled kernel into a contiguous raw binary image:

```powershell
cmd /c "copy /b boot.bin + kernel.bin os.img"

```

**Step 4: Standardize Floppy Disk Image Geometry**
Expand the generated image to a standard 1.44 MB ($1{,}474{,}560\text{ bytes}$) floppy layout:

```powershell
$fs = [System.IO.File]::OpenWrite("os.img")
$fs.SetLength(1474560)
$fs.Close()

```

**Step 5: Emulate via QEMU**
Launch the virtual machine and mount the image as the primary boot floppy drive:

```powershell
& "<Path-To-Qemu>\qemu-system-x86_64.exe" -fda os.img

```
