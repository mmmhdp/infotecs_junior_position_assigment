#include "driver_facade.hpp"
#include <chrono>
#include <thread>

/**
 * @brief Construct a chip_spi_api object for bit-banging SPI.
 *
 * @param gpio Reference to GPIO driver
 * @param pinCS Chip select pin
 * @param pinSCK SPI clock pin
 * @param pinMOSI SPI MOSI pin
 * @param pinMISO SPI MISO pin
 */
chip_spi_api::chip_spi_api(i_chip_gpio_driver &gpio, int pinCS, int pinSCK,
                           int pinMOSI, int pinMISO)
    : m_gpio(gpio), pinCS_(pinCS), pinSCK_(pinSCK), pinMOSI_(pinMOSI),
      pinMISO_(pinMISO) {
  deselect();
  m_gpio.setLow(pinSCK_);
}

/**
 * @brief Select the SPI device by pulling CS low.
 */
void chip_spi_api::select() { m_gpio.setLow(pinCS_); }

/**
 * @brief Deselect the SPI device by pulling CS high.
 */
void chip_spi_api::deselect() { m_gpio.setHigh(pinCS_); }

/**
 * @brief Generate a single clock pulse on the SCK line.
 *
 * This function is used by writeBit and readBit.
 */
void chip_spi_api::clockPulse() {
  m_gpio.setHigh(pinSCK_);

  std::this_thread::sleep_for(std::chrono::microseconds(1));

  m_gpio.setLow(pinSCK_);

  std::this_thread::sleep_for(std::chrono::microseconds(1));
}

/**
 * @brief Write a single bit over SPI.
 *
 * @param bit Value of the bit to write (true = 1, false = 0)
 */
void chip_spi_api::writeBit(bool bit) {
  if (bit)
    m_gpio.setHigh(pinMOSI_);
  else
    m_gpio.setLow(pinMOSI_);
  clockPulse();
}

/**
 * @brief Read a single bit over SPI.
 *
 * @return true if the bit read is 1, false if 0
 */
bool chip_spi_api::readBit() {
  clockPulse();
  return m_gpio.read(pinMISO_);
}

/**
 * @brief Transfer a byte over SPI.
 *
 * Sends 8 bits MSB-first and returns the byte received simultaneously.
 *
 * @param data Byte to send
 * @return uint8_t Received byte
 */
uint8_t chip_spi_api::transfer(uint8_t data) {
  uint8_t result = 0;
  for (int i = 7; i >= 0; --i) {
    bool bitToSend = (data >> i) & 0x1;
    writeBit(bitToSend);
    bool bitReceived = m_gpio.read(pinMISO_);
    result = (result << 1) | (bitReceived ? 1 : 0);
  }
  return result;
}

/**
 * @brief Construct an eeprom_api object for EEPROM operations.
 *
 * @param spi Reference to SPI interface
 */
eeprom_api::eeprom_api(i_chip_spi_api &spi) : spi_api_(spi) {}

/**
 * @brief Enable writing to EEPROM.
 */
void eeprom_api::writeEnable() {
  spi_api_.select();
  spi_api_.transfer(CMD_WREN);
  spi_api_.deselect();
}

/**
 * @brief Read the status register from EEPROM.
 *
 * @return uint8_t Status register value
 */
uint8_t eeprom_api::readStatus() {
  spi_api_.select();
  spi_api_.transfer(CMD_RDSR);
  /*
   * it's seems weird but we have to
   * make a second call with zeroed
   * data, because bytes in responce
   * to previous call are garbage in reality
   * since eeprom is not really get a
   * read command yet.
   * so we call again and now we get a
   * real response to prevoiusly
   * send command
   * */
  uint8_t status = spi_api_.transfer(0x00);
  spi_api_.deselect();
  return status;
}

/**
 * @brief Wait until EEPROM is ready for the next operation.
 *
 * Polls the Write-In-Process (WIP) bit in the status register.
 */
void eeprom_api::waitUntilReady() {
  /*
   * it's recommended to organize
   * pooling cycle in docs itself with
   * pooling wip (write-in-process) bit status in the first place
   * to be able to have consistent memory write
   * */
  uint8_t wip_bit = 0x01;
  while (readStatus() & wip_bit) {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }
}

/**
 * @brief Send a raw command byte to EEPROM.
 *
 * @param command Command to send
 */
void eeprom_api::sendCommand(uint8_t command) { spi_api_.transfer(command); }

/**
 * @brief Send a 9-bit EEPROM memory address over SPI.
 *
 * @param address Memory address (0..511)
 */
void eeprom_api::sendAddress(uint16_t address) {
  /*
   * from the docs:
   * 9-bit address
   * 512 byte memory in total
   * */
  uint8_t addrHigh = (address >> 8) & 0x01;
  uint8_t addrLow = address & 0xFF;
  spi_api_.transfer(addrHigh);
  spi_api_.transfer(addrLow);
}

/**
 * @brief Write a single byte to EEPROM.
 *
 * @param address Memory address to write to
 * @param data Byte to write
 */
void eeprom_api::writeByte(uint16_t address, uint8_t data) {
  writeEnable();

  spi_api_.select();
  sendCommand(CMD_WRITE);
  sendAddress(address);
  spi_api_.transfer(data);
  spi_api_.deselect();

  waitUntilReady();
}

/**
 * @brief Read a single byte from EEPROM.
 *
 * @param address Memory address to read
 * @return uint8_t Value read from memory
 */
uint8_t eeprom_api::readByte(uint16_t address) {
  spi_api_.select();
  sendCommand(CMD_READ);
  sendAddress(address);
  uint8_t data = spi_api_.transfer(0x00);
  spi_api_.deselect();
  return data;
}

/**
 * @brief Write multiple bytes to EEPROM.
 *
 * Writes each byte individually using writeByte().
 *
 * @param address Starting memory address
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 */
void eeprom_api::writeBuffer(uint16_t address, const uint8_t *data,
                             std::size_t length) {
  for (std::size_t i = 0; i < length; ++i) {
    writeByte(address + i, data[i]);
  }
}

/**
 * @brief Read multiple bytes from EEPROM.
 *
 * Reads each byte individually using readByte().
 *
 * @param address Starting memory address
 * @param buffer Pointer to buffer to store data
 * @param length Number of bytes to read
 */
void eeprom_api::readBuffer(uint16_t address, uint8_t *buffer,
                            std::size_t length) {
  for (std::size_t i = 0; i < length; ++i) {
    buffer[i] = readByte(address + i);
  }
}

/**
 * @brief Write a single bit to EEPROM.
 *
 * Uses read-modify-write on the containing byte.
 *
 * @param address Memory address
 * @param bitPosition Bit index (0-7)
 * @param value Boolean value to write
 */
void eeprom_api::writeBit(uint16_t address, uint8_t bitPosition, bool value) {
  uint8_t byte = readByte(address);
  if (value)
    byte |= (1 << bitPosition);
  else
    byte &= ~(1 << bitPosition);
  writeByte(address, byte);
}

/**
 * @brief Read a single bit from EEPROM.
 *
 * @param address Memory address
 * @param bitPosition Bit index (0-7)
 * @return true if bit is 1, false if 0
 */
bool eeprom_api::readBit(uint16_t address, uint8_t bitPosition) {
  uint8_t byte = readByte(address);
  return (byte >> bitPosition) & 0x01;
}
