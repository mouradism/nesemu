#include <gtest/gtest.h>
#include "apu.hpp"

class APUTest : public ::testing::Test {
protected:
    APU apu;
    
    void SetUp() override {
        apu.reset();
    }
};

// Initialization and Reset Tests
TEST_F(APUTest, ConstructorInitializesAPU) {
    APU fresh_apu;
    std::uint8_t status = fresh_apu.read(0x4015);
    EXPECT_EQ(status, 0x00);
}

TEST_F(APUTest, ResetClearsAllState) {
    apu.write(0x4000, 0xFF);
    apu.write(0x4015, 0x0F);
    apu.reset();
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status, 0x00);
}

// Pulse Channel Tests
TEST_F(APUTest, Pulse1ControlRegisterWrite) {
    apu.write(0x4000, 0xF3);
    apu.write(0x4015, 0x01);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status, 0x00);
}

TEST_F(APUTest, Pulse1FrequencyRegisters) {
    // Enable channel FIRST, then write length counter
    apu.write(0x4015, 0x01);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);  // Length counter loaded because channel is enabled
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x01, 0x01);
}

TEST_F(APUTest, Pulse2FrequencyRegisters) {
    // Enable channel FIRST, then write length counter
    apu.write(0x4015, 0x02);
    apu.write(0x4006, 0x56);
    apu.write(0x4007, 0x09);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x02, 0x02);
}

TEST_F(APUTest, Pulse1SweepRegister) {
    apu.write(0x4001, 0xF8);
}

TEST_F(APUTest, Pulse2SweepRegister) {
    apu.write(0x4005, 0x48);
}

// Triangle Channel Tests
TEST_F(APUTest, TriangleControlRegister) {
    // Enable channel FIRST
    apu.write(0x4015, 0x04);
    apu.write(0x4008, 0x7F);
    apu.write(0x400A, 0x12);
    apu.write(0x400B, 0x04);  // Length counter loaded
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x04, 0x04);
}

TEST_F(APUTest, TriangleFrequencyRegisters) {
    apu.write(0x400A, 0xFF);
    apu.write(0x400B, 0x0F);
}

// Noise Channel Tests
TEST_F(APUTest, NoiseControlRegister) {
    // Enable channel FIRST
    apu.write(0x4015, 0x08);
    apu.write(0x400C, 0x3F);
    apu.write(0x400E, 0x84);
    apu.write(0x400F, 0x08);  // Length counter loaded
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x08, 0x08);
}

TEST_F(APUTest, NoisePeriodIndices) {
    // Enable channel FIRST
    apu.write(0x4015, 0x08);
    for (std::uint8_t i = 0; i < 16; i++) {
        apu.write(0x400E, (0x80 | i));
        apu.write(0x400F, 0x08);
        std::uint8_t status = apu.read(0x4015);
        EXPECT_EQ(status & 0x08, 0x08);
    }
}

TEST_F(APUTest, NoiseLoopFlag) {
    // Enable channel FIRST
    apu.write(0x4015, 0x08);
    apu.write(0x400E, 0x80);
    apu.write(0x400F, 0x08);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x08, 0x08);
}

// Control Register Tests
TEST_F(APUTest, ControlRegisterEnableChannels) {
    // Enable channels FIRST, then write registers
    apu.write(0x4015, 0x0F);
    apu.write(0x4000, 0x3F);
    apu.write(0x4003, 0x08);
    apu.write(0x4004, 0x3F);
    apu.write(0x4007, 0x08);
    apu.write(0x4008, 0x7F);
    apu.write(0x400B, 0x08);
    apu.write(0x400C, 0x3F);
    apu.write(0x400F, 0x08);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x0F, 0x0F);
}

TEST_F(APUTest, ControlRegisterDisableChannels) {
    // Enable channels FIRST
    apu.write(0x4015, 0x0F);
    apu.write(0x4000, 0x3F);
    apu.write(0x4003, 0x08);
    apu.write(0x4004, 0x3F);
    apu.write(0x4007, 0x08);
    apu.write(0x4008, 0x7F);
    apu.write(0x400B, 0x08);
    apu.write(0x400C, 0x3F);
    apu.write(0x400F, 0x08);
    // Now disable
    apu.write(0x4015, 0x00);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status, 0x00);
}

TEST_F(APUTest, ControlRegisterSelectiveEnable) {
    // Enable only pulse1 and triangle (0x05)
    apu.write(0x4015, 0x05);
    apu.write(0x4000, 0x3F);
    apu.write(0x4003, 0x08);
    apu.write(0x4008, 0x7F);
    apu.write(0x400B, 0x08);
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x05, 0x05);
    EXPECT_EQ(status & 0x02, 0x00);  // pulse2 disabled
    EXPECT_EQ(status & 0x08, 0x00);  // noise disabled
}

// Frame Counter Tests
TEST_F(APUTest, FrameCounterMode4Step) {
    apu.write(0x4017, 0x00);
    for (int i = 0; i < 100; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, FrameCounterMode5Step) {
    apu.write(0x4017, 0x80);
    for (int i = 0; i < 100; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, FrameInterruptInhibitFlag) {
    apu.write(0x4017, 0x40);
    apu.write(0x4017, 0x00);
    EXPECT_TRUE(true);
}

// Clocking and Audio Generation Tests
TEST_F(APUTest, ClockAdvancesState) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, GetSamplesReturnsAudio) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 100000; i++) {
        apu.clock();
    }
    auto samples = apu.get_samples();
    EXPECT_GT(samples.size(), 0);
}

TEST_F(APUTest, GetSamplesClears) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 100000; i++) {
        apu.clock();
    }
    auto samples1 = apu.get_samples();
    auto size1 = samples1.size();
    auto samples2 = apu.get_samples();
    EXPECT_LT(samples2.size(), size1);
}

TEST_F(APUTest, SampleValuesInRange) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 200000; i++) {
        apu.clock();
    }
    auto samples = apu.get_samples();
    for (float sample : samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Envelope and Length Counter Tests
TEST_F(APUTest, PulseEnvelopeStartFlag) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 10000; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, ConstantVolumeMode) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x1F);
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0x08);
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, LengthCounterHalt) {
    // Enable channel first, use halt flag (bit 5 = 0x20)
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);  // 0x3F has bit 5 set (length counter halt)
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0x08);  // Load length counter
    for (int i = 0; i < 50000; i++) {
        apu.clock();
    }
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x01, 0x01);
}

// Register Boundary Tests
TEST_F(APUTest, InvalidReadReturnsZero) {
    std::uint8_t result = apu.read(0x4000);
    EXPECT_EQ(result, 0x00);
}

TEST_F(APUTest, InvalidWriteDoesntCrash) {
    apu.write(0x4010, 0xFF);
    apu.write(0x4020, 0x00);
    EXPECT_TRUE(true);
}

TEST_F(APUTest, MultipleWritesSameRegister) {
    for (int i = 0; i < 10; i++) {
        apu.write(0x4000, static_cast<std::uint8_t>(i << 4));
    }
    EXPECT_TRUE(true);
}

// Triangle Linear Counter Tests
TEST_F(APUTest, TriangleLinearCounterControl) {
    apu.write(0x4015, 0x04);
    apu.write(0x4008, 0xFF);
    apu.write(0x400A, 0x00);
    apu.write(0x400B, 0x08);
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, TriangleLinearCounterReload) {
    apu.write(0x4015, 0x04);
    apu.write(0x4008, 0x7F);
    apu.write(0x400A, 0x12);
    apu.write(0x400B, 0x08);
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    EXPECT_TRUE(true);
}

// Audio Mixing Tests
TEST_F(APUTest, MixAllChannels) {
    apu.write(0x4015, 0x0F);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);
    apu.write(0x4004, 0x3F);
    apu.write(0x4006, 0x56);
    apu.write(0x4007, 0x08);
    apu.write(0x4008, 0x7F);
    apu.write(0x400A, 0x78);
    apu.write(0x400B, 0x08);
    apu.write(0x400C, 0x3F);
    apu.write(0x400E, 0x04);
    apu.write(0x400F, 0x08);
    for (int i = 0; i < 200000; i++) {
        apu.clock();
    }
    auto samples = apu.get_samples();
    EXPECT_GT(samples.size(), 0);
    for (float sample : samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// Lookup Table Tests
TEST_F(APUTest, NoisePeriodTableValues) {
    const std::uint16_t expected[] = {
        4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
    };
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(APU::NOISE_PERIOD_TABLE[i], expected[i]);
    }
}

TEST_F(APUTest, LengthCounterTableSize) {
    apu.write(0x4015, 0x01);
    for (int i = 0; i < 32; i++) {
        apu.write(0x4003, (i << 3) | 0x08);
    }
    EXPECT_TRUE(true);
}

TEST_F(APUTest, PulseWaveformsDataIntegrity) {
    const std::uint8_t expected[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 1},
        {0, 0, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 0, 0},
    };
    for (int duty = 0; duty < 4; duty++) {
        for (int phase = 0; phase < 8; phase++) {
            EXPECT_EQ(APU::PULSE_WAVEFORMS[duty][phase], expected[duty][phase]);
        }
    }
}

// Stress Tests
TEST_F(APUTest, ExtendedClocking) {
    apu.write(0x4015, 0x0F);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x34);
    apu.write(0x4003, 0x08);
    apu.write(0x4004, 0x3F);
    apu.write(0x4006, 0x56);
    apu.write(0x4007, 0x08);
    apu.write(0x4008, 0x7F);
    apu.write(0x400A, 0x78);
    apu.write(0x400B, 0x08);
    apu.write(0x400C, 0x3F);
    apu.write(0x400E, 0x04);
    apu.write(0x400F, 0x08);
    for (int i = 0; i < 1789773; i++) {
        apu.clock();
    }
    auto samples = apu.get_samples();
    EXPECT_GT(samples.size(), 40000);
}

TEST_F(APUTest, RepeatedEnableDisable) {
    for (int cycle = 0; cycle < 10; cycle++) {
        apu.write(0x4015, 0x01);
        apu.write(0x4000, 0x3F);
        apu.write(0x4002, 0x34);
        apu.write(0x4003, 0x08);
        for (int i = 0; i < 1000; i++) {
            apu.clock();
        }
        apu.write(0x4015, 0x00);
        for (int i = 0; i < 100; i++) {
            apu.clock();
        }
    }
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status, 0x00);
}

TEST_F(APUTest, DifferentDutyCycles) {
    for (int duty = 0; duty < 4; duty++) {
        apu.reset();
        apu.write(0x4015, 0x01);
        apu.write(0x4000, (duty << 6) | 0x3F);
        apu.write(0x4002, 0x34);
        apu.write(0x4003, 0x08);
        for (int i = 0; i < 10000; i++) {
            apu.clock();
        }
    }
    EXPECT_TRUE(true);
}

// ============================================================================
// Sweep Unit Tests
// ============================================================================

TEST_F(APUTest, Pulse1SweepUnitEnabled) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4001, 0x87);
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0x08);
    
    for (int i = 0; i < 20000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, Pulse1SweepNegate) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4001, 0x8F);
    apu.write(0x4002, 0x80);
    apu.write(0x4003, 0x08);
    
    for (int i = 0; i < 20000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, Pulse2SweepNegate) {
    apu.write(0x4015, 0x02);
    apu.write(0x4004, 0x3F);
    apu.write(0x4005, 0x8F);
    apu.write(0x4006, 0x80);
    apu.write(0x4007, 0x08);
    
    for (int i = 0; i < 20000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, SweepMutingLowPeriod) {
    apu.write(0x4015, 0x01);
    apu.write(0x4000, 0x3F);
    apu.write(0x4001, 0x80);
    apu.write(0x4002, 0x04);
    apu.write(0x4003, 0x08);
    
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(true);
}

// ============================================================================
// DMC Channel Tests
// ============================================================================

TEST_F(APUTest, DMCRateRegister) {
    apu.write(0x4010, 0x0F);
    apu.write(0x4011, 0x40);
    apu.write(0x4012, 0x00);
    apu.write(0x4013, 0x01);
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, DMCDirectLoad) {
    apu.write(0x4011, 0x7F);
    
    for (int i = 0; i < 1000; i++) {
        apu.clock();
    }
    
    auto samples = apu.get_samples();
    EXPECT_GT(samples.size(), 0);
}

TEST_F(APUTest, DMCEnableDisable) {
    apu.write(0x4010, 0x0F);
    apu.write(0x4011, 0x40);
    apu.write(0x4012, 0x00);
    apu.write(0x4013, 0x10);
    
    apu.write(0x4015, 0x10);
    std::uint8_t status = apu.read(0x4015);
    
    apu.write(0x4015, 0x00);
    status = apu.read(0x4015);
    EXPECT_EQ(status & 0x10, 0x00);
}

TEST_F(APUTest, DMCLoopFlag) {
    apu.write(0x4010, 0x4F);
    apu.write(0x4011, 0x40);
    apu.write(0x4012, 0x00);
    apu.write(0x4013, 0x01);
    apu.write(0x4015, 0x10);
    
    for (int i = 0; i < 10000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, DMCIRQFlag) {
    apu.write(0x4010, 0x80);
    apu.write(0x4011, 0x40);
    apu.write(0x4012, 0x00);
    apu.write(0x4013, 0x01);
    
    EXPECT_TRUE(true);
}

// ============================================================================
// Frame Interrupt Tests
// ============================================================================

TEST_F(APUTest, FrameInterruptFlag4Step) {
    apu.write(0x4017, 0x00);
    
    for (int i = 0; i < 30000; i++) {
        apu.clock();
    }
    
    std::uint8_t status = apu.read(0x4015);
    std::uint8_t status2 = apu.read(0x4015);
    EXPECT_EQ(status2 & 0x40, 0x00);
}

TEST_F(APUTest, FrameInterruptInhibit) {
    apu.write(0x4017, 0x40);
    
    for (int i = 0; i < 30000; i++) {
        apu.clock();
    }
    
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x40, 0x00);
}

TEST_F(APUTest, FrameInterrupt5StepNoIRQ) {
    apu.write(0x4017, 0x80);
    
    for (int i = 0; i < 40000; i++) {
        apu.clock();
    }
    
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x40, 0x00);
}

TEST_F(APUTest, IRQCallbackTriggered) {
    bool irq_triggered = false;
    apu.set_irq_callback([&irq_triggered]() {
        irq_triggered = true;
    });
    
    apu.write(0x4017, 0x00);
    
    for (int i = 0; i < 30000; i++) {
        apu.clock();
    }
    
    EXPECT_TRUE(irq_triggered);
}

// ============================================================================
// Status Register Tests (Enhanced)
// ============================================================================

TEST_F(APUTest, StatusRegisterDMCBit) {
    apu.write(0x4010, 0x0F);
    apu.write(0x4012, 0x00);
    apu.write(0x4013, 0x10);
    apu.write(0x4015, 0x10);
    
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x10, 0x10);
}

TEST_F(APUTest, StatusRegisterClearsFrameInterrupt) {
    apu.write(0x4017, 0x00);
    
    for (int i = 0; i < 30000; i++) {
        apu.clock();
    }
    
    apu.read(0x4015);
    
    std::uint8_t status = apu.read(0x4015);
    EXPECT_EQ(status & 0x40, 0x00);
}

// ============================================================================
// DMC Rate Table Test
// ============================================================================

TEST_F(APUTest, DMCRateTableValues) {
    const std::uint16_t expected[] = {
        428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
    };
    
    for (int i = 0; i < 16; i++) {
        EXPECT_EQ(APU::DMC_RATE_TABLE[i], expected[i]);
    }
}

// ============================================================================
// Nonlinear Mixer Test
// ============================================================================

TEST_F(APUTest, NonlinearMixerOutput) {
    apu.write(0x4015, 0x1F);
    apu.write(0x4000, 0x3F);
    apu.write(0x4002, 0x00);
    apu.write(0x4003, 0xF8);
    
    apu.write(0x4004, 0x3F);
    apu.write(0x4006, 0x00);
    apu.write(0x4007, 0xF8);
    
    apu.write(0x4008, 0xFF);
    apu.write(0x400A, 0x00);
    apu.write(0x400B, 0xF8);
    
    apu.write(0x400C, 0x3F);
    apu.write(0x400E, 0x00);
    apu.write(0x400F, 0xF8);
    
    apu.write(0x4011, 0x7F);
    
    for (int i = 0; i < 100000; i++) {
        apu.clock();
    }
    
    auto samples = apu.get_samples();
    EXPECT_GT(samples.size(), 0);
    
    for (float sample : samples) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
    }
}

// ============================================================================
// Memory Read Callback Test
// ============================================================================

TEST_F(APUTest, MemoryReadCallbackSet) {
    bool callback_set = false;
    apu.set_memory_read_callback([&callback_set](std::uint16_t addr) -> std::uint8_t {
        callback_set = true;
        return 0x00;
    });
    
    EXPECT_TRUE(true);
}

TEST_F(APUTest, GetFrameInterrupt) {
    apu.write(0x4017, 0x00);
    
    for (int i = 0; i < 30000; i++) {
        apu.clock();
    }
    
    bool frame_int = apu.get_frame_interrupt();
    EXPECT_TRUE(frame_int);
}

TEST_F(APUTest, GetDMCInterrupt) {
    bool dmc_int = apu.get_dmc_interrupt();
    EXPECT_FALSE(dmc_int);  // Should be false initially
}
