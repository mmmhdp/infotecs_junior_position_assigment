#include "driver_facade.hpp"

int main() {
  mock_chip_gpio_driver gpio;

  chip_spi_api spi(gpio, 0, 1, 2, 3);
  eeprom_api eeprom(spi);

  uint8_t value = 0xAB;
  eeprom.writeByte(0x10, value);
  uint8_t read = eeprom.readByte(0x10);

  bool bitValue = eeprom.readBit(0x10, 3);
  eeprom.writeBit(0x10, 3, !bitValue);

  return 0;
}
