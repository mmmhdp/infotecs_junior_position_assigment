#include "driver_facade.hpp"
#include <iostream>

/**
 * @brief Mock GPIO driver for testing SPI EEPROM.
 *
 * Prints pin operations to the console instead of interacting with real
 * hardware.
 */
class mock_chip_gpio_driver : public i_chip_gpio_driver {
public:
  /**
   * @brief Set the specified pin HIGH.
   * @param pin Pin number
   */
  void setHigh(int pin) override {
    std::cout << "Pin: " << pin << " HIGH" << std::endl;
  }

  /**
   * @brief Set the specified pin LOW.
   * @param pin Pin number
   */
  void setLow(int pin) override {
    std::cout << "Pin: " << pin << " LOW" << std::endl;
  }

  /**
   * @brief Read the state of a pin.
   * @param pin Pin number
   * @return false Always returns false in mock
   */
  bool read(int pin) override { return false; }
};

/**
 * @brief Example usage of EEPROM API with mock SPI.
 *
 * Demonstrates writing a byte, reading it back, and modifying a bit.
 */
int main() {
  mock_chip_gpio_driver gpio;

  chip_spi_api spi(gpio, 0, 1, 2, 3); ///< SPI interface using GPIO bit-banging
  eeprom_api eeprom(spi);             ///< EEPROM API using SPI

  uint8_t value = 0xAB;
  eeprom.writeByte(0x10, value); ///< Write a byte to address 0x10

  uint8_t read = eeprom.readByte(0x10); ///< Read back the byte

  bool bitValue = eeprom.readBit(0x10, 3); ///< Read bit 3
  eeprom.writeBit(0x10, 3, !bitValue);     ///< Toggle bit 3

  return 0;
}
