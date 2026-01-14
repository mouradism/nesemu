#include <gtest/gtest.h>
#include "controller.hpp"

// =============================================================================
// Button State Tests
// =============================================================================

TEST(ControllerTest, InitialState) {
    Controller ctrl;
    EXPECT_EQ(ctrl.get_state(), 0x00);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_FALSE(ctrl.is_pressed(static_cast<Button>(i)));
    }
}

TEST(ControllerTest, SetButtonA) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::A));
    EXPECT_EQ(ctrl.get_state(), 0x01);
    
    ctrl.set_button(Button::A, false);
    EXPECT_FALSE(ctrl.is_pressed(Button::A));
    EXPECT_EQ(ctrl.get_state(), 0x00);
}

TEST(ControllerTest, SetButtonB) {
    Controller ctrl;
    
    ctrl.set_button(Button::B, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::B));
    EXPECT_EQ(ctrl.get_state(), 0x02);
}

TEST(ControllerTest, SetButtonSelect) {
    Controller ctrl;
    
    ctrl.set_button(Button::Select, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Select));
    EXPECT_EQ(ctrl.get_state(), 0x04);
}

TEST(ControllerTest, SetButtonStart) {
    Controller ctrl;
    
    ctrl.set_button(Button::Start, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Start));
    EXPECT_EQ(ctrl.get_state(), 0x08);
}

TEST(ControllerTest, SetButtonUp) {
    Controller ctrl;
    
    ctrl.set_button(Button::Up, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Up));
    EXPECT_EQ(ctrl.get_state(), 0x10);
}

TEST(ControllerTest, SetButtonDown) {
    Controller ctrl;
    
    ctrl.set_button(Button::Down, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Down));
    EXPECT_EQ(ctrl.get_state(), 0x20);
}

TEST(ControllerTest, SetButtonLeft) {
    Controller ctrl;
    
    ctrl.set_button(Button::Left, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Left));
    EXPECT_EQ(ctrl.get_state(), 0x40);
}

TEST(ControllerTest, SetButtonRight) {
    Controller ctrl;
    
    ctrl.set_button(Button::Right, true);
    EXPECT_TRUE(ctrl.is_pressed(Button::Right));
    EXPECT_EQ(ctrl.get_state(), 0x80);
}

TEST(ControllerTest, MultipleButtons) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    ctrl.set_button(Button::B, true);
    ctrl.set_button(Button::Start, true);
    
    EXPECT_TRUE(ctrl.is_pressed(Button::A));
    EXPECT_TRUE(ctrl.is_pressed(Button::B));
    EXPECT_TRUE(ctrl.is_pressed(Button::Start));
    EXPECT_FALSE(ctrl.is_pressed(Button::Select));
    
    EXPECT_EQ(ctrl.get_state(), 0x0B);  // A=1, B=2, Start=8
}

TEST(ControllerTest, SetState) {
    Controller ctrl;
    
    ctrl.set_state(0xFF);
    EXPECT_EQ(ctrl.get_state(), 0xFF);
    
    for (int i = 0; i < 8; ++i) {
        EXPECT_TRUE(ctrl.is_pressed(static_cast<Button>(i)));
    }
    
    ctrl.set_state(0x00);
    EXPECT_EQ(ctrl.get_state(), 0x00);
}

TEST(ControllerTest, Reset) {
    Controller ctrl;
    
    ctrl.set_state(0xFF);
    ctrl.reset();
    
    EXPECT_EQ(ctrl.get_state(), 0x00);
}

// =============================================================================
// Shift Register Tests
// =============================================================================

TEST(ControllerTest, StrobeLatchesState) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    ctrl.set_button(Button::Start, true);  // 0x09
    
    // Strobe high then low
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // First read: A (bit 0)
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    
    // Second read: B (bit 1) - not pressed
    EXPECT_EQ(ctrl.read() & 0x01, 0);
    
    // Third read: Select (bit 2) - not pressed
    EXPECT_EQ(ctrl.read() & 0x01, 0);
    
    // Fourth read: Start (bit 3) - pressed
    EXPECT_EQ(ctrl.read() & 0x01, 1);
}

TEST(ControllerTest, FullButtonRead) {
    Controller ctrl;
    
    // Set specific pattern: A, Select, Up, Right (0x55)
    ctrl.set_button(Button::A, true);      // bit 0
    ctrl.set_button(Button::Select, true); // bit 2
    ctrl.set_button(Button::Up, true);     // bit 4
    ctrl.set_button(Button::Right, true);  // bit 7
    
    EXPECT_EQ(ctrl.get_state(), 0x95);  // A=1, Select=4, Up=16, Right=128 = 0x95
    
    // Strobe
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // Read all 8 buttons
    std::uint8_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= (ctrl.read() & 0x01) << i;
    }
    
    EXPECT_EQ(result, 0x95);
}

TEST(ControllerTest, StrobeHighReturnsA) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    ctrl.set_button(Button::B, true);
    
    // Strobe high - should always return A button
    ctrl.write(0x01);
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(ctrl.read() & 0x01, 1);
    }
}

TEST(ControllerTest, StrobeHighANotPressed) {
    Controller ctrl;
    
    ctrl.set_button(Button::B, true);  // B pressed, not A
    
    ctrl.write(0x01);  // Strobe high
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(ctrl.read() & 0x01, 0);  // A not pressed
    }
}

TEST(ControllerTest, AfterEightReadsReturns1) {
    Controller ctrl;
    
    ctrl.set_state(0x00);  // No buttons
    
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // First 8 reads return button states
    for (int i = 0; i < 8; ++i) {
        ctrl.read();
    }
    
    // After 8 reads, should return 1 (open bus behavior)
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    EXPECT_EQ(ctrl.read() & 0x01, 1);
}

TEST(ControllerTest, ReadReturnsOpenBusBits) {
    Controller ctrl;
    
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // Read should have bit 6 set (open bus)
    std::uint8_t result = ctrl.read();
    EXPECT_TRUE(result & 0x40);
}

TEST(ControllerTest, MultipleStrobes) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    
    // First strobe
    ctrl.write(0x01);
    ctrl.write(0x00);
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    EXPECT_EQ(ctrl.read() & 0x01, 0);  // B
    
    // Second strobe resets
    ctrl.write(0x01);
    ctrl.write(0x00);
    EXPECT_EQ(ctrl.read() & 0x01, 1);  // A again
}

TEST(ControllerTest, StateChangeDuringRead) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    
    ctrl.write(0x01);
    ctrl.write(0x00);
    
    // Read A
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    
    // Change state (should not affect current read sequence)
    ctrl.set_button(Button::A, false);
    ctrl.set_button(Button::B, true);
    
    // Still reading from latched state
    EXPECT_EQ(ctrl.read() & 0x01, 0);  // B was not pressed when latched
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(ControllerTest, RapidStrobes) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    
    for (int i = 0; i < 100; ++i) {
        ctrl.write(0x01);
        ctrl.write(0x00);
        EXPECT_EQ(ctrl.read() & 0x01, 1);
    }
}

TEST(ControllerTest, NoStrobeBeforeRead) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    
    // Read without strobing - shift register is 0
    EXPECT_EQ(ctrl.read() & 0x01, 0);
}

TEST(ControllerTest, StrobeOnly) {
    Controller ctrl;
    
    ctrl.set_button(Button::A, true);
    
    // Strobe high only, no low
    ctrl.write(0x01);
    
    // Should continuously return A
    EXPECT_EQ(ctrl.read() & 0x01, 1);
    EXPECT_EQ(ctrl.read() & 0x01, 1);
}

// =============================================================================
// Button Enum Tests
// =============================================================================

TEST(ButtonEnumTest, ButtonValues) {
    EXPECT_EQ(static_cast<std::uint8_t>(Button::A), 0);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::B), 1);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Select), 2);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Start), 3);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Up), 4);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Down), 5);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Left), 6);
    EXPECT_EQ(static_cast<std::uint8_t>(Button::Right), 7);
}
