# Oshi OS

A bare-metal, 64-bit operating system written in **LSM** (a custom systems programming language) with a handcrafted x86 assembly bootloader. Oshi OS boots straight into high-resolution graphics (1920x1080x32bpp) via the Bochs VBE Display Adapter (BGA) and features an integrated desktop environment, interactive terminal shell, persistent file management, text editing, animations, and built-in applications[cite: 8, 12, 13].

---

## Key Features

* **Custom 64-Bit Boot Architecture:**
  * Custom 16-bit Master Boot Record (MBR) enabling 32-bit Protected Mode and transitioning to 64-bit Long Mode[cite: 13].
  * 4-Level identity paging configured to map full 4GB physical memory address space[cite: 13].
  * BIOS 8x16 bitmap font extracted directly from interrupt routines to system RAM[cite: 13].
* **High-Resolution Graphics Engine:**
  * Direct frame manipulation targeting 1920x1080 TrueColor (32bpp)[cite: 11, 13].
  * Hardware double-buffering using dedicated back buffers and linear framebuffers to eliminate artifacts[cite: 11, 13].
  * Scaled 2x2 typography rendering with fast dirty-rectangle region invalidation[cite: 11].
* **Modular Desktop Launcher (`desktop.lsm`):**
  * Top status bar displaying real-time system details[cite: 5, 8].
  * Unified launcher allowing instant context switching between system tools and applications[cite: 5].
* **Interactive Terminal Shell (`shell.lsm`):**
  * Integrated prompt and command processor.
  * Commands for filesystem traversal, file inspection, memory diagnostics, and application management (`help`, `clear`, `list`, `info`, `create`, `edit`, `read`, `delete`, `exit`).
* **Integrated Apps Suite:**
  * **Editor (`apps/editor.lsm`):** In-place text editor with disk flush persistence[cite: 6].
  * **Calculator (`apps/calc.lsm`):** Fixed-point expression evaluator supporting chained arithmetic operations[cite: 4].
  * **Snake (`apps/snake.lsm`):** Real-time keyboard-driven arcade classic[cite: 3].
  * **Pac-Man (`apps/pacman.lsm`):** Maze navigation, scoring, dynamic collision checks, and chase AI[cite: 1].
  * **Oshi Demo (`apps/oshi.lsm`):** Multi-stage animated geometric visualizer[cite: 7].

---

## Directory Structure

├── boot/
│   └── boot.s           # MBR, BGA configuration, Long Mode switch, Paging setup
├── kernel/
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



## System Requirements & Toolchain

* **LSM Compiler:** The native `lsm` compiler toolchain for compilation and link stages.
* **NASM:** Netwide Assembler for `boot.s` compilation.
* **QEMU:** `qemu-system-x86_64` with VGA / Bochs VBE device emulation support.

---

## Build & Emulation

### 1. Assemble the Bootloader

```bash
nasm -f bin boot.s -o boot.bin

```

### 2. Compile Kernel & Application Suite

```bash
lsm compile kernel.lsm -o kernel.bin

```

### 3. Build Bootable Disk Image

```bash
dd if=/dev/zero of=oshi_os.img bs=512 count=2880
dd if=boot.bin of=oshi_os.img conv=notrunc bs=512 seek=0
dd if=kernel.bin of=oshi_os.img conv=notrunc bs=512 seek=1

```

### 4. Run via QEMU

```bash
qemu-system-x86_64 -drive format=raw,file=oshi_os.img -m 128M -vga std

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

```

```

