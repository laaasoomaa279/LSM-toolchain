# Oshi OS

A bare-metal, 64-bit operating system written in **LSM** (a custom systems programming language) with a handcrafted x86 assembly bootloader. Oshi OS boots straight into high-resolution graphics ($1920 \times 1080 \times 32\text{bpp}$) via the Bochs VBE Display Adapter (BGA) and features an integrated desktop environment, interactive terminal shell, persistent file management, text editing, animations, and built-in applications.

---

## Key Features

* **Custom 64-Bit Boot Architecture:**
* Custom 16-bit Master Boot Record (MBR) enabling 32-bit Protected Mode and transitioning to 64-bit Long Mode.
* 4-Level identity paging configured to map the full 4GB physical memory address space.
* BIOS 8x16 bitmap font extracted directly from interrupt routines to system RAM.


* **High-Resolution Graphics Engine:**
* Direct frame manipulation targeting $1920 \times 1080$ TrueColor ($32\text{bpp}$).
* Hardware double-buffering using dedicated back buffers and linear framebuffers to eliminate artifacts.
* Scaled $2\times2$ typography rendering with fast dirty-rectangle region invalidation.


* **Modular Desktop Launcher (`desktop.lsm`):**
* Top status bar displaying real-time system details.
* Unified launcher allowing instant context switching between system tools and applications.


* **Interactive Terminal Shell (`shell.lsm`):**
* Integrated prompt and command processor.
* Commands for filesystem traversal, file inspection, memory diagnostics, and application management (`help`, `clear`, `list`, `info`, `create`, `edit`, `read`, `delete`, `exit`).


* **Integrated Apps Suite:**
* **Editor (`apps/editor.lsm`):** In-place text editor with disk flush persistence.
* **Calculator (`apps/calc.lsm`):** Fixed-point expression evaluator supporting chained arithmetic operations.
* **Snake (`apps/snake.lsm`):** Real-time keyboard-driven arcade classic.
* **Pac-Man (`apps/pacman.lsm`):** Maze navigation, scoring, dynamic collision checks, and chase AI.
* **Oshi Demo (`apps/oshi.lsm`):** Multi-stage animated geometric visualizer.



---

## Directory Structure

```text
├── boot/
│   └── boot.s           # MBR, BGA configuration, Long Mode switch, Paging setup
├── kernel/
│   ├── kernel.lsm       # Kernel entry point and initialization
│   ├── fonts.lsm        # Font metrics and glyph bitmap retrieval
│   ├── fs.lsm           # In-memory storage, allocation tables, and file descriptors
│   ├── keyboard.lsm     # PS/2 keyboard scancode decoder and non-blocking IO
│   ├── system.lsm       # Hardware status bar drawing and uptime tracking
│   └── vga.lsm          # BGA framebuffers, double-buffering, blitting routines
└── apps/
    ├── desktop.lsm      # Main launcher environment
    ├── shell.lsm        # Interactive command-line interface
    ├── editor.lsm       # File editor
    ├── calc.lsm         # Fixed-point calculator
    ├── snake.lsm        # Snake game
    ├── pacman.lsm       # Pacman game
    └── oshi.lsm         # Graphics demo visualizer

```

---

## System Requirements & Toolchain

* **LSM Compiler:** The native `LSM.exe` compiler toolchain for bare-metal builds.
* **NASM:** Netwide Assembler for `boot.s` compilation.
* **QEMU:** `qemu-system-x86_64` with VGA / Bochs VBE device emulation support.
* **PowerShell:** For Windows automated build pipeline execution.

---

## Build & Emulation (PowerShell Pipeline)

### 1. Assemble the Bootloader

```powershell
nasm -f bin boot/boot.s -o boot.bin

```

### 2. Compile the Kernel with Full Optimizations

```powershell
.\LSM.exe build kernel/kernel.lsm --baremetal --modern --entry=_start -o kernel.bin -O3

```

### 3. Generate the 10 MB Bootable Disk Image

```powershell
[System.IO.File]::WriteAllBytes("temp_sys.bin", [System.IO.File]::ReadAllBytes("boot.bin") + [System.IO.File]::ReadAllBytes("kernel.bin"))
$diskSize = 10 * 1024 * 1024
$imgBytes = New-Object byte[] $diskSize
$sysBytes = [System.IO.File]::ReadAllBytes("temp_sys.bin")
[Array]::Copy($sysBytes, 0, $imgBytes, 0, $sysBytes.Length)
[System.IO.File]::WriteAllBytes("oshi_os.img", $imgBytes)

```

### 4. Run via QEMU

```powershell
& "(Your Path):\qemu\qemu-system-x86_64.exe" -drive file=oshi_os.img,format=raw,index=0,media=disk -vga std -m 256M

```

---

## Shell Commands Reference

| Command | Arguments | Description |
| --- | --- | --- |
| `help` | None | Lists available shell commands |
| `clear` | None | Clears terminal canvas and resets buffer cursor |
| `list` | None | Displays files with metadata and byte sizes |
| `info` | None | Displays hardware, resolution, and memory parameters |
| `create` | `<filename>` | Allocates a new empty file entry on storage |
| `edit` | `<filename>` | Opens interactive editor for text modification |
| `read` | `<filename>` | Dumps file contents directly to standard terminal output |
| `delete` | `<filename>` | Removes the target file entry from filesystem records |
| `exit` | None | Terminates shell session and returns to Desktop environment |
