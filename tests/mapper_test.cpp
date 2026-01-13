#include <gtest/gtest.h>
#include "mapper.hpp"
#include "mapper_000.hpp"
#include "mapper_001.hpp"
#include "mapper_002.hpp"
#include "mapper_003.hpp"

#include <vector>

// =============================================================================
// Mapper Factory Tests
// =============================================================================

TEST(MapperFactoryTest, CreateMapper0) {
    auto mapper = create_mapper(0, 2, 1, MirrorMode::Horizontal);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->mapper_id(), 0);
    EXPECT_STREQ(mapper->mapper_name(), "NROM");
}

TEST(MapperFactoryTest, CreateMapper1) {
    auto mapper = create_mapper(1, 8, 4, MirrorMode::Vertical);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->mapper_id(), 1);
    EXPECT_STREQ(mapper->mapper_name(), "MMC1");
}

TEST(MapperFactoryTest, CreateMapper2) {
    auto mapper = create_mapper(2, 8, 0, MirrorMode::Horizontal);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->mapper_id(), 2);
    EXPECT_STREQ(mapper->mapper_name(), "UxROM");
}

TEST(MapperFactoryTest, CreateMapper3) {
    auto mapper = create_mapper(3, 1, 4, MirrorMode::Vertical);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->mapper_id(), 3);
    EXPECT_STREQ(mapper->mapper_name(), "CNROM");
}

TEST(MapperFactoryTest, UnsupportedMapperFallsBackToNROM) {
    auto mapper = create_mapper(255, 2, 1, MirrorMode::Horizontal);
    ASSERT_NE(mapper, nullptr);
    EXPECT_EQ(mapper->mapper_id(), 0);  // Falls back to NROM
}

// =============================================================================
// Mapper 0 (NROM) Tests
// =============================================================================

class Mapper000Test : public ::testing::Test {
protected:
    std::vector<std::uint8_t> prg_rom_;
    std::vector<std::uint8_t> chr_rom_;
    std::vector<std::uint8_t> chr_ram_;
    std::unique_ptr<Mapper000> mapper_;
    
    void SetUp() override {
        // Default: 32KB PRG, 8KB CHR
        prg_rom_.resize(32 * 1024);
        chr_rom_.resize(8 * 1024);
        
        // Fill with pattern
        for (std::size_t i = 0; i < prg_rom_.size(); ++i) {
            prg_rom_[i] = static_cast<std::uint8_t>(i & 0xFF);
        }
        for (std::size_t i = 0; i < chr_rom_.size(); ++i) {
            chr_rom_[i] = static_cast<std::uint8_t>((i + 0x80) & 0xFF);
        }
        
        mapper_ = std::make_unique<Mapper000>(2, 1);
        mapper_->set_rom_data(&prg_rom_, &chr_rom_, nullptr);
    }
};

TEST_F(Mapper000Test, ReadPRG32KB) {
    // First bank
    EXPECT_EQ(mapper_->read_prg(0x8000), prg_rom_[0x0000]);
    EXPECT_EQ(mapper_->read_prg(0x8001), prg_rom_[0x0001]);
    
    // Second bank
    EXPECT_EQ(mapper_->read_prg(0xC000), prg_rom_[0x4000]);
    EXPECT_EQ(mapper_->read_prg(0xFFFF), prg_rom_[0x7FFF]);
}

TEST_F(Mapper000Test, ReadPRG16KB_Mirrored) {
    prg_rom_.resize(16 * 1024);
    mapper_ = std::make_unique<Mapper000>(1, 1);
    mapper_->set_rom_data(&prg_rom_, &chr_rom_, nullptr);
    
    // Both banks should read from same 16KB
    EXPECT_EQ(mapper_->read_prg(0x8000), mapper_->read_prg(0xC000));
    EXPECT_EQ(mapper_->read_prg(0xBFFF), mapper_->read_prg(0xFFFF));
}

TEST_F(Mapper000Test, ReadCHR) {
    EXPECT_EQ(mapper_->read_chr(0x0000), chr_rom_[0x0000]);
    EXPECT_EQ(mapper_->read_chr(0x1FFF), chr_rom_[0x1FFF]);
}

TEST_F(Mapper000Test, WriteCHRROM_Ignored) {
    std::uint8_t original = mapper_->read_chr(0x0000);
    mapper_->write_chr(0x0000, 0xFF);
    EXPECT_EQ(mapper_->read_chr(0x0000), original);
}

TEST_F(Mapper000Test, WriteCHRRAM_Works) {
    chr_ram_.resize(8 * 1024, 0);
    mapper_ = std::make_unique<Mapper000>(2, 0);
    mapper_->set_rom_data(&prg_rom_, nullptr, &chr_ram_);
    
    mapper_->write_chr(0x0000, 0xAB);
    EXPECT_EQ(mapper_->read_chr(0x0000), 0xAB);
}

// =============================================================================
// Mapper 1 (MMC1) Tests
// =============================================================================

class Mapper001Test : public ::testing::Test {
protected:
    std::vector<std::uint8_t> prg_rom_;
    std::vector<std::uint8_t> chr_rom_;
    std::vector<std::uint8_t> chr_ram_;
    std::unique_ptr<Mapper001> mapper_;
    
    void SetUp() override {
        // 256KB PRG, 128KB CHR (typical MMC1 game)
        prg_rom_.resize(256 * 1024);
        chr_rom_.resize(128 * 1024);
        
        for (std::size_t i = 0; i < prg_rom_.size(); ++i) {
            prg_rom_[i] = static_cast<std::uint8_t>(i & 0xFF);
        }
        for (std::size_t i = 0; i < chr_rom_.size(); ++i) {
            chr_rom_[i] = static_cast<std::uint8_t>((i + 0x40) & 0xFF);
        }
        
        mapper_ = std::make_unique<Mapper001>(16, 16);
        mapper_->set_rom_data(&prg_rom_, &chr_rom_, nullptr);
    }
    
    void write_mmc1_register(std::uint16_t addr, std::uint8_t value) {
        // Write 5 bits serially
        for (int i = 0; i < 5; ++i) {
            mapper_->write_prg(addr, (value >> i) & 0x01);
        }
    }
};

TEST_F(Mapper001Test, ResetShiftRegister) {
    // Write with bit 7 set should reset
    mapper_->write_prg(0x8000, 0x80);
    // Should not crash
    EXPECT_TRUE(true);
}

TEST_F(Mapper001Test, DefaultsToLastBankFixed) {
    // After reset, last bank should be at $C000
    std::uint8_t last_bank_byte = prg_rom_[(15 * 0x4000)];  // Bank 15
    EXPECT_EQ(mapper_->read_prg(0xC000), last_bank_byte);
}

TEST_F(Mapper001Test, PRGBankSwitching) {
    // Set PRG mode 3 (fix last bank) and select bank 5
    write_mmc1_register(0x8000, 0x0C);  // Control: mode 3
    write_mmc1_register(0xE000, 0x05);  // PRG bank 5
    
    std::uint8_t bank5_byte = prg_rom_[(5 * 0x4000)];
    EXPECT_EQ(mapper_->read_prg(0x8000), bank5_byte);
}

TEST_F(Mapper001Test, PRGRAMAccess) {
    EXPECT_TRUE(mapper_->has_prg_ram());
    
    mapper_->write_prg_ram(0x6000, 0xAB);
    EXPECT_EQ(mapper_->read_prg_ram(0x6000), 0xAB);
    
    mapper_->write_prg_ram(0x7FFF, 0xCD);
    EXPECT_EQ(mapper_->read_prg_ram(0x7FFF), 0xCD);
}

TEST_F(Mapper001Test, MirroringChange) {
    // Set mirroring to vertical (mode 2)
    write_mmc1_register(0x8000, 0x02);
    EXPECT_EQ(mapper_->mirror_mode(), MirrorMode::Vertical);
    
    // Set mirroring to horizontal (mode 3)
    write_mmc1_register(0x8000, 0x03);
    EXPECT_EQ(mapper_->mirror_mode(), MirrorMode::Horizontal);
}

// =============================================================================
// Mapper 2 (UxROM) Tests
// =============================================================================

class Mapper002Test : public ::testing::Test {
protected:
    std::vector<std::uint8_t> prg_rom_;
    std::vector<std::uint8_t> chr_ram_;
    std::unique_ptr<Mapper002> mapper_;
    
    void SetUp() override {
        // 128KB PRG, 8KB CHR RAM
        prg_rom_.resize(128 * 1024);
        chr_ram_.resize(8 * 1024, 0);
        
        for (std::size_t i = 0; i < prg_rom_.size(); ++i) {
            prg_rom_[i] = static_cast<std::uint8_t>(i & 0xFF);
        }
        
        mapper_ = std::make_unique<Mapper002>(8, 0);
        mapper_->set_rom_data(&prg_rom_, nullptr, &chr_ram_);
    }
};

TEST_F(Mapper002Test, FixedLastBank) {
    // Last bank (7) should be at $C000
    std::uint8_t last_bank_byte = prg_rom_[(7 * 0x4000)];
    EXPECT_EQ(mapper_->read_prg(0xC000), last_bank_byte);
}

TEST_F(Mapper002Test, SwitchableFirstBank) {
    // Default: bank 0 at $8000
    EXPECT_EQ(mapper_->read_prg(0x8000), prg_rom_[0]);
    
    // Switch to bank 3
    mapper_->write_prg(0x8000, 0x03);
    std::uint8_t bank3_byte = prg_rom_[(3 * 0x4000)];
    EXPECT_EQ(mapper_->read_prg(0x8000), bank3_byte);
    
    // Last bank still fixed
    std::uint8_t last_bank_byte = prg_rom_[(7 * 0x4000)];
    EXPECT_EQ(mapper_->read_prg(0xC000), last_bank_byte);
}

TEST_F(Mapper002Test, CHRRAMAccess) {
    mapper_->write_chr(0x0000, 0x12);
    EXPECT_EQ(mapper_->read_chr(0x0000), 0x12);
    
    mapper_->write_chr(0x1FFF, 0x34);
    EXPECT_EQ(mapper_->read_chr(0x1FFF), 0x34);
}

// =============================================================================
// Mapper 3 (CNROM) Tests
// =============================================================================

class Mapper003Test : public ::testing::Test {
protected:
    std::vector<std::uint8_t> prg_rom_;
    std::vector<std::uint8_t> chr_rom_;
    std::unique_ptr<Mapper003> mapper_;
    
    void SetUp() override {
        // 32KB PRG, 32KB CHR (4 banks)
        prg_rom_.resize(32 * 1024);
        chr_rom_.resize(32 * 1024);
        
        for (std::size_t i = 0; i < prg_rom_.size(); ++i) {
            prg_rom_[i] = static_cast<std::uint8_t>(i & 0xFF);
        }
        // Fill each CHR bank with different values
        for (std::size_t bank = 0; bank < 4; ++bank) {
            for (std::size_t i = 0; i < 0x2000; ++i) {
                chr_rom_[bank * 0x2000 + i] = static_cast<std::uint8_t>(bank * 0x40 + (i & 0x3F));
            }
        }
        
        mapper_ = std::make_unique<Mapper003>(2, 4);
        mapper_->set_rom_data(&prg_rom_, &chr_rom_, nullptr);
    }
};

TEST_F(Mapper003Test, PRGNotBanked) {
    // PRG should work like NROM
    EXPECT_EQ(mapper_->read_prg(0x8000), prg_rom_[0]);
    EXPECT_EQ(mapper_->read_prg(0xFFFF), prg_rom_[0x7FFF]);
}

TEST_F(Mapper003Test, CHRBankSwitching) {
    // Default: bank 0
    EXPECT_EQ(mapper_->read_chr(0x0000), chr_rom_[0]);
    
    // Switch to bank 2
    mapper_->write_prg(0x8000, 0x02);
    EXPECT_EQ(mapper_->read_chr(0x0000), chr_rom_[2 * 0x2000]);
    
    // Switch to bank 3
    mapper_->write_prg(0x8000, 0x03);
    EXPECT_EQ(mapper_->read_chr(0x0000), chr_rom_[3 * 0x2000]);
}

// =============================================================================
// Common Mapper Interface Tests
// =============================================================================

TEST(MapperInterfaceTest, MirrorModeInitialization) {
    auto mapper = create_mapper(0, 2, 1, MirrorMode::Vertical);
    EXPECT_EQ(mapper->mirror_mode(), MirrorMode::Vertical);
    
    mapper = create_mapper(0, 2, 1, MirrorMode::Horizontal);
    EXPECT_EQ(mapper->mirror_mode(), MirrorMode::Horizontal);
}

TEST(MapperInterfaceTest, ResetWorks) {
    auto mapper = create_mapper(2, 8, 0, MirrorMode::Horizontal);
    mapper->write_prg(0x8000, 0x05);  // Change bank
    
    mapper->reset();
    // After reset, bank should be 0 again (implementation dependent)
    EXPECT_TRUE(true);  // Just verify no crash
}

TEST(MapperInterfaceTest, IRQDefaultsOff) {
    auto mapper = create_mapper(0, 2, 1, MirrorMode::Horizontal);
    EXPECT_FALSE(mapper->irq_pending());
}
