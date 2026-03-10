# NES Emulator (nesemu)
<img width="1040" height="566" alt="image" src="https://github.com/user-attachments/assets/82a5188b-77c6-4119-b856-e89d2342d8c2" />

A comprehensive Nintendo Entertainment System (NES) emulator written in modern C++20, featuring accurate CPU/PPU/APU emulation, ImGui-based debugging tools, and support for multiple mapper types.

##  Features

### Core Emulation
- **6502 CPU** - Cycle-accurate implementation with all official and unofficial opcodes
- **PPU (Picture Processing Unit)** - Pixel-perfect graphics rendering with proper scanline timing
- **APU (Audio Processing Unit)** - Full audio synthesis with support for all 5 channels
- **Memory Management** - Complete 16-bit address space with cartridge banking
- **Mapper Support** - NROM (0), UxROM (2), CNROM (3), MMC1 (1)

### Graphics & Audio
- **OpenGL-based Rendering** - Hardware-accelerated display with nearest-neighbor filtering
- **ImGui Integration** - Modern debug interface for real-time emulation inspection
- **SDL2 Audio** - Low-latency audio output with dynamic sample generation
- **60 FPS Synchronized Emulation** - VSync-locked frame timing

### Debugging
- **Real-time CPU State Inspector** - View registers, flags, and cycle count
- **PPU Debug Window** - Monitor frame-ready status and NMI events
- **Performance Metrics** - FPS counter and per-frame timing
- **Controller State Display** - Visual feedback for input states

##  Quick Start

### Prerequisites
- **C++20 Compatible Compiler** (MSVC 2022, GCC 10+, Clang 10+)
- **CMake 3.8+**
- **Git**
- **Ninja** (recommended) or MSBuild

### Building

```bash
# Clone the repository
git clone https://github.com/mouradism/nesemu.git
cd nesemu

# Configure the build
cmake -B out/build/x64-debug -G Ninja

# Build the emulator
cmake --build out/build/x64-debug --target nesemu

# Run
./out/build/x64-debug/Debug/nesemu.exe [path/to/rom.nes]
```

### Usage

```bash
# Run with a specific ROM
nesemu.exe "Super_Mario_Bros.nes"

# Run without arguments (demo mode with test pattern)
nesemu.exe
```

### Controls

| Key | Action |
|-----|--------|
| **Arrow Keys** | D-Pad |
| **Z** | A Button |
| **X** | B Button |
| **Enter** | Start |
| **Right Shift** | Select |
| **F1** | Toggle Debug Window |
| **ESC** | Quit |

## ?? Project Structure

```
nesemu/
 src/                          # Source code
    cpu.cpp/hpp              # 6502 CPU implementation
    ppu.cpp/hpp              # PPU with scanline rendering
    apu.cpp/hpp              # Audio synthesis engine
    memory.cpp/hpp           # Memory bus and address decoding
    cartridge.cpp/hpp        # ROM loader and cartridge handling
    mapper*.cpp/hpp          # Mapper implementations (0,1,2,3)
    audio.cpp/hpp            # SDL2 audio interface
    video.cpp/hpp            # OpenGL rendering and ImGui integration
    input.cpp/hpp            # Keyboard input mapping
    controller.cpp/hpp       # NES controller emulation
    imgui_manager.cpp/hpp    # ImGui debug interface
    main.cpp                 # Entry point
 tests/                        # Unit tests (GTest)
    cpu_test.cpp
    ppu_test.cpp
    apu_test.cpp
    memory_test.cpp
    cartridge_test.cpp
    mapper_test.cpp
 roms/                         # Game ROMs (not included)
 CMakeLists.txt              # Build configuration
 README.md                    # This file
```

## ??? Architecture

### CPU (src/cpu.cpp/hpp)
- **Instruction Set**: All 256 6502 opcodes including unofficial ones
- **Addressing Modes**: IMP, ACC, IMM, ZPG, ZPX, ZPY, REL, ABS, ABX, ABY, IND, IZX, IZY
- **Cycle Accuracy**: Proper page boundary crossing penalties
- **Interrupts**: NMI and IRQ support

### PPU (src/ppu.cpp/hpp)
- **Rendering**: 256x240 resolution with proper scanline timing
- **Sprite Handling**: 8 sprites per scanline with collision detection
- **Scrolling**: Horizontal and vertical scrolling with fine X adjustment
- **Attribute Tables**: Proper palette indexing from nametable attributes
- **Mirroring**: Support for Horizontal, Vertical, Single-Screen, and 4-Screen modes

### APU (src/apu.cpp/hpp)
- **Pulse Channels** (2): Square wave generation with envelope and sweep
- **Triangle Channel**: Triangle wave synthesis
- **Noise Channel**: Pseudo-random noise generation
- **DMC Channel**: Delta modulation (basic support)
- **Master Volume**: 44.1 kHz output with dynamic mixing

### Memory (src/memory.cpp/hpp)
- **Address Space**: 64KB addressable memory
- **RAM**: 2KB internal RAM with mirroring
- **PPU Registers**: $2000-$2007 with proper port semantics
- **APU Registers**: $4000-$4017 for audio control
- **Cartridge Space**: $8000-$FFFF for ROM and mapper I/O

## ?? Mappers

### Mapper 0 (NROM)
- No banking
- Games: Super Mario Bros, Donkey Kong, Pac-Man
- Status: ? Fully Supported

### Mapper 1 (MMC1)
- 16KB programmable PRG ROM banks
- 4KB programmable CHR ROM/RAM banks
- Games: The Legend of Zelda, Metroid
- Status: ? Fully Supported

### Mapper 2 (UxROM)
- 16KB programmable PRG ROM banks
- Fixed CHR ROM
- Games: Mega Man, Castlevania
- Status: ? Fully Supported

### Mapper 3 (CNROM)
- Fixed PRG ROM
- 8KB programmable CHR ROM banks
- Games: Arkanoid, Paperboy
- Status: ? Fully Supported

## ?? Testing

Run the comprehensive test suite:

```bash
# Build and run all tests
cmake --build out/build/x64-debug

# Or run specific test suites
./out/build/x64-debug/Debug/cpu_test.exe
./out/build/x64-debug/Debug/ppu_test.exe
./out/build/x64-debug/Debug/apu_test.exe
```

## ?? Debug Interface

Press **F1** in-game to toggle the debug window. The interface provides:

### Performance Tab
- Real-time FPS counter
- Per-frame timing in milliseconds

### CPU Tab
- Accumulator (A), X, Y registers
- Stack Pointer (SP)
- Program Counter (PC)
- Status register breakdown (C, Z, I, D, V, N flags)
- Cycle counter

### PPU Tab
- Frame-ready status
- NMI interrupt status
- Framebuffer information

### APU Tab
- Audio output status
- Channel information

### Controllers Tab
- Real-time button state display
- Support for both Player 1 and Player 2

## ?? Building from Source

### Windows (Visual Studio 2022)

```bash
# With Ninja
cmake -B build -G Ninja
cmake --build build

# With Visual Studio IDE
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build
```

### Linux (GCC/Clang)

```bash
cmake -B build -G Ninja
cmake --build build
```

### macOS

```bash
cmake -B build -G Ninja
cmake --build build
```

##  Dependencies

- **SDL2** - Cross-platform window and audio management
- **OpenGL 2.1+** - Hardware-accelerated rendering
- **ImGui** - Immediate-mode GUI framework
- **GTest** - Unit testing framework (optional, for tests)
- **CMake** - Build system

All dependencies are automatically fetched via CMake's FetchContent.

##  Configuration

### Emulation Settings (src/main.cpp)
- `SCREEN_WIDTH`: 256 pixels
- `SCREEN_HEIGHT`: 240 pixels
- `WINDOW_SCALE`: 2x by default
- `CPU_CYCLES_PER_FRAME`: 29780 cycles
- `TARGET_FPS`: 60 Hz
- `SAMPLE_RATE`: 44100 Hz

### Audio Latency
- Buffer size: 512 samples (~12ms)
- Min buffer: 512 samples for underrun protection
- Max buffer: 2048 samples to minimize latency

##  Performance

Typical performance on modern hardware:

| Metric | Value |
|--------|-------|
| CPU Emulation | ~3 million cycles/second |
| PPU Rendering | 60 FPS (vsync-locked) |
| Audio Output | 44.1 kHz, 16-bit stereo |
| Memory Usage | ~50 MB baseline |

## ?? Known Limitations

- **CHR RAM Games**: Full support for writable CHR RAM
- **Save Games**: No save state functionality yet
- **Advanced Mappers**: Only NROM, MMC1, UxROM, CNROM supported
- **Palettes**: Uses standard NES palette (no custom palettes)
- **Video Filters**: Nearest-neighbor only (no scaling filters)

##  Git Branches

- **`master`** - Stable release branch (public)
- **`imgui`** - ImGui debug UI development
- **`dev`** - General development work
- **`cpuDev`** - CPU-specific optimizations

##  License

This project is provided as-is for educational purposes. NES is a trademark of Nintendo.

## ?? Contributing

For private contributions, please contact the repository owner.

##  References

- **NES Architecture**: https://wiki.nesdev.org/
- **6502 CPU**: http://6502.org/
- **Mapper Database**: https://wiki.nesdev.org/w/index.php/Mapper
- **PPU Documentation**: https://wiki.nesdev.org/w/index.php/PPU

##  Future Enhancements

- [ ] Additional mapper support (MMC2, MMC3, MMC4, etc.)
- [ ] Save state functionality
- [ ] Rewind capability
- [ ] Cheat code support
- [ ] Network multiplayer
- [ ] Video filters and scaling
- [ ] Input recording and playback
- [ ] Profiling and optimization tools

##  Support

For issues, questions, or suggestions, please visit:
https://github.com/mouradism/nesemu

---

**Status**:  Actively Maintained  
**Last Updated**: January 2025  
**Version**: 1.0.0
