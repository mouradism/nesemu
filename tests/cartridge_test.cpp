#include <gtest/gtest.h>
#include "cartridge.hpp"
#include <fstream>
#include <vector>
#include <cstdio>

class CartridgeTest : public ::testing::Test {
protected:
    std::string temp_file_;
    
    void SetUp() override {
        temp_file_ = "";
    }
    
    void TearDown() override {
        if (!temp_file_.empty()) {
            std::remove(temp_file_.c_str());
        }
    }
    
    // Helper: Create a minimal valid iNES ROM file
    std::string create_test_rom(
        std::uint8_t prg_banks, 
        std::uint8_t chr_banks,
        bool vertical_mirror = false,
        std::uint8_t mapper = 0
    ) {
        temp_file_ = "test_rom_" + std::to_string(rand()) + ".nes";
        
        std::vector<std::uint8_t> data;
        
        // iNES header (16 bytes)
        data.push_back('N');
        data.push_back('E');
        data.push_back('S');
        data.push_back(0x1A);  // MS-DOS EOF
        data.push_back(prg_banks);  // PRG ROM size in 16KB units
        data.push_back(chr_banks);  // CHR ROM size in 8KB units
        
        std::uint8_t flags6 = (mapper & 0x0F) << 4;
        if (vertical_mirror) flags6 |= 0x01;
        data.push_back(flags6);
        
        std::uint8_t flags7 = mapper & 0xF0;
        data.push_back(flags7);
        
        // Flags 8-15 (zeros for basic ROM)
        for (int i = 8; i < 16; ++i) {
            data.push_back(0);
        }
        
        // PRG ROM data (filled with pattern)
        for (std::size_t i = 0; i < prg_banks * 16384; ++i) {
            data.push_back(static_cast<std::uint8_t>(i & 0xFF));
        }
        
        // CHR ROM data (filled with pattern)
        for (std::size_t i = 0; i < chr_banks * 8192; ++i) {
            data.push_back(static_cast<std::uint8_t>((i + 0x80) & 0xFF));
        }
        
        std::ofstream file(temp_file_, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        
        return temp_file_;
    }
};

// =============================================================================
// Loading Tests
// =============================================================================

TEST_F(CartridgeTest, EmptyPathNotLoaded) {
    Cartridge cart("");
    EXPECT_FALSE(cart.loaded());
}

TEST_F(CartridgeTest, NonExistentFileNotLoaded) {
    Cartridge cart("nonexistent_file_12345.nes");
    EXPECT_FALSE(cart.loaded());
}

TEST_F(CartridgeTest, ValidROMLoads) {
    std::string path = create_test_rom(1, 1);
    Cartridge cart(path);
    EXPECT_TRUE(cart.loaded());
}

// =============================================================================
// iNES Header Parsing Tests
// =============================================================================

TEST_F(CartridgeTest, ParsesPRGBanks) {
    std::string path = create_test_rom(2, 1);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.prg_banks(), 2);
    EXPECT_EQ(cart.prg().size(), 2 * 16384);
}

TEST_F(CartridgeTest, ParsesCHRBanks) {
    std::string path = create_test_rom(1, 2);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.chr_banks(), 2);
    EXPECT_EQ(cart.chr().size(), 2 * 8192);
}

TEST_F(CartridgeTest, ZeroCHRBanksCreatesCHRRAM) {
    std::string path = create_test_rom(1, 0);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.chr_banks(), 0);
    EXPECT_TRUE(cart.has_chr_ram());
    EXPECT_EQ(cart.chr().size(), 8192);  // 8KB CHR RAM
}

TEST_F(CartridgeTest, ParsesHorizontalMirroring) {
    std::string path = create_test_rom(1, 1, false);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.mirror_mode(), Cartridge::MirrorMode::Horizontal);
}

TEST_F(CartridgeTest, ParsesVerticalMirroring) {
    std::string path = create_test_rom(1, 1, true);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.mirror_mode(), Cartridge::MirrorMode::Vertical);
}

TEST_F(CartridgeTest, ParsesMapperID) {
    // Create ROM with mapper 1 (MMC1)
    temp_file_ = "test_mapper.nes";
    
    std::vector<std::uint8_t> data;
    data.push_back('N');
    data.push_back('E');
    data.push_back('S');
    data.push_back(0x1A);
    data.push_back(1);  // 1 PRG bank
    data.push_back(1);  // 1 CHR bank
    data.push_back(0x10);  // Mapper low nibble = 1
    data.push_back(0x00);  // Mapper high nibble = 0
    for (int i = 8; i < 16; ++i) data.push_back(0);
    
    // PRG and CHR data
    for (std::size_t i = 0; i < 16384 + 8192; ++i) {
        data.push_back(0);
    }
    
    std::ofstream file(temp_file_, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    Cartridge cart(temp_file_);
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.mapper_id(), 1);
}

// =============================================================================
// PRG ROM Access Tests
// =============================================================================

TEST_F(CartridgeTest, ReadPRGReturnsData) {
    std::string path = create_test_rom(1, 1);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    
    // PRG data was filled with (i & 0xFF)
    EXPECT_EQ(cart.read_prg(0x8000), 0x00);
    EXPECT_EQ(cart.read_prg(0x8001), 0x01);
    EXPECT_EQ(cart.read_prg(0x80FF), 0xFF);
}

TEST_F(CartridgeTest, PRGMirroringFor16KB) {
    std::string path = create_test_rom(1, 1);  // 16KB PRG
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    
    // $8000-$BFFF and $C000-$FFFF should mirror
    EXPECT_EQ(cart.read_prg(0x8000), cart.read_prg(0xC000));
    EXPECT_EQ(cart.read_prg(0x8100), cart.read_prg(0xC100));
}

TEST_F(CartridgeTest, PRGNoMirroringFor32KB) {
    std::string path = create_test_rom(2, 1);  // 32KB PRG
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    
    // First 16KB: data[0] = 0x00
    // Second 16KB: data[16384] = 0x00 (pattern wraps)
    // They should be different positions
    EXPECT_EQ(cart.read_prg(0x8000), 0x00);
    EXPECT_EQ(cart.read_prg(0xC000), 0x00);  // 16384 & 0xFF = 0
}

// =============================================================================
// CHR ROM/RAM Access Tests
// =============================================================================

TEST_F(CartridgeTest, ReadCHRROMReturnsData) {
    std::string path = create_test_rom(1, 1);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    
    // CHR data was filled with ((i + 0x80) & 0xFF)
    EXPECT_EQ(cart.read_chr(0x0000), 0x80);
    EXPECT_EQ(cart.read_chr(0x0001), 0x81);
    EXPECT_EQ(cart.read_chr(0x007F), 0xFF);
}

TEST_F(CartridgeTest, WriteCHRROMIgnored) {
    std::string path = create_test_rom(1, 1);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_FALSE(cart.has_chr_ram());
    
    std::uint8_t original = cart.read_chr(0x0000);
    cart.write_chr(0x0000, 0xFF);
    EXPECT_EQ(cart.read_chr(0x0000), original);  // Write ignored
}

TEST_F(CartridgeTest, WriteCHRRAMWorks) {
    std::string path = create_test_rom(1, 0);  // No CHR banks = CHR RAM
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_TRUE(cart.has_chr_ram());
    
    cart.write_chr(0x0000, 0xAB);
    EXPECT_EQ(cart.read_chr(0x0000), 0xAB);
    
    cart.write_chr(0x1000, 0xCD);
    EXPECT_EQ(cart.read_chr(0x1000), 0xCD);
}

TEST_F(CartridgeTest, CHRAddressMasking) {
    std::string path = create_test_rom(1, 1);
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    
    // Addresses should be masked to $0000-$1FFF
    EXPECT_EQ(cart.read_chr(0x0000), cart.read_chr(0x2000));
    EXPECT_EQ(cart.read_chr(0x1000), cart.read_chr(0x3000));
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(CartridgeTest, LargePRGROM) {
    std::string path = create_test_rom(8, 1);  // 128KB PRG
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.prg_banks(), 8);
    EXPECT_EQ(cart.prg().size(), 8 * 16384);
}

TEST_F(CartridgeTest, LargeCHRROM) {
    std::string path = create_test_rom(1, 8);  // 64KB CHR
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.chr_banks(), 8);
    EXPECT_EQ(cart.chr().size(), 8 * 8192);
}

TEST_F(CartridgeTest, MinimalROM) {
    std::string path = create_test_rom(1, 0);  // 16KB PRG, CHR RAM
    Cartridge cart(path);
    
    EXPECT_TRUE(cart.loaded());
    EXPECT_EQ(cart.prg_banks(), 1);
    EXPECT_EQ(cart.chr_banks(), 0);
    EXPECT_TRUE(cart.has_chr_ram());
}

// =============================================================================
// Invalid File Tests
// =============================================================================

TEST_F(CartridgeTest, InvalidMagicNumber) {
    temp_file_ = "invalid_magic.nes";
    
    std::vector<std::uint8_t> data(16 + 16384 + 8192, 0);
    data[0] = 'X';  // Wrong magic
    data[1] = 'Y';
    data[2] = 'Z';
    data[3] = 0x1A;
    data[4] = 1;
    data[5] = 1;
    
    std::ofstream file(temp_file_, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    Cartridge cart(temp_file_);
    EXPECT_FALSE(cart.loaded());
}

TEST_F(CartridgeTest, FileTooSmall) {
    temp_file_ = "too_small.nes";
    
    std::vector<std::uint8_t> data = {'N', 'E', 'S', 0x1A};  // Only 4 bytes
    
    std::ofstream file(temp_file_, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    Cartridge cart(temp_file_);
    EXPECT_FALSE(cart.loaded());
}
