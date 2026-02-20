#pragma once

#include <cstdint>

/**
 * @brief Interface for GPIO driver used for bit-banging SPI.
 */
struct i_chip_gpio_driver {
  /**
   * @brief Set the specified GPIO pin high.
   * @param pin Pin number
   */
  virtual void setHigh(int pin) = 0;

  /**
   * @brief Set the specified GPIO pin low.
   * @param pin Pin number
   */
  virtual void setLow(int pin) = 0;

  /**
   * @brief Read the current state of a GPIO pin.
   * @param pin Pin number
   * @return true if pin is high, false if low
   */
  virtual bool read(int pin) = 0;

  virtual ~i_chip_gpio_driver() = default;
};

/**
 * @brief Abstract SPI interface for memory devices.
 *
 * SPI operations are performed at byte level,
 * even if the underlying hardware transfers bits.
 */
class i_chip_spi_api {
public:
  /*
   * To begin communication
   * master have to
   * set CS to LOW on the slave device
   *
   * on contrary, deselect ends
   * communication and sets
   * slave device's CS to HIGH
   * */
  virtual void select() = 0;
  virtual void deselect() = 0;

  /*
   * SPI assume data transfers by bits,
   * but we need byte transfer interface
   * inplace since EEPROM device is
   * capable only of understand
   * by byte and not by bit communication scenario
   * */
  /**
   * @brief Transfer a byte over SPI.
   * @param data Byte to send
   * @return uint8_t Received byte
   */
  virtual uint8_t transfer(uint8_t data) = 0;

  virtual ~i_chip_spi_api() = default;
};

/**
 * @brief Concrete SPI implementation using GPIO bit-banging.
 */
class chip_spi_api final : public i_chip_spi_api {
public:
  /**
   * @brief Construct a chip_spi_api instance.
   * @param gpio Reference to GPIO driver
   * @param pinCS Chip select pin
   * @param pinSCK SPI clock pin
   * @param pinMOSI SPI MOSI pin
   * @param pinMISO SPI MISO pin
   */
  chip_spi_api(i_chip_gpio_driver &gpio, int pinCS, int pinSCK, int pinMOSI,
               int pinMISO);

  void select() override;
  void deselect() override;
  uint8_t transfer(uint8_t data) override;

private:
  /*
   * SPI primitives
   * */

  /**
   * @brief Generate a clock pulse on SCK.
   */
  void clockPulse();

  /**
   * @brief Write a single bit over SPI.
   * @param bit Bit value to write (true=1, false=0)
   */
  void writeBit(bool bit);

  /**
   * @brief Read a single bit from SPI.
   * @return true if bit is 1, false if 0
   */
  bool readBit();

private:
  i_chip_gpio_driver &m_gpio; ///< Reference to GPIO driver

  int pinCS_;   ///< Chip select pin
  int pinSCK_;  ///< Clock pin
  int pinMOSI_; ///< MOSI pin
  int pinMISO_; ///< MISO pin
};

/**
 * @brief Generic memory device interface (EEPROM/NOR etc.).
 *
 * Defines the standard read/write API for memory devices.
 */
class i_memory_device_api {
public:
  virtual void writeByte(uint16_t address, uint8_t data) = 0;
  virtual uint8_t readByte(uint16_t address) = 0;

  virtual void writeBuffer(uint16_t address, const uint8_t *data,
                           std::size_t length) = 0;
  virtual void readBuffer(uint16_t address, uint8_t *buffer,
                          std::size_t length) = 0;

  virtual void writeBit(uint16_t address, uint8_t bitPosition, bool value) = 0;
  virtual bool readBit(uint16_t address, uint8_t bitPosition) = 0;

  virtual ~i_memory_device_api() = default;
};

/**
 * @brief EEPROM implementation of i_memory_device_api using SPI.
 */
class eeprom_api final : public i_memory_device_api {
public:
  /**
   * @brief Construct EEPROM API with SPI interface.
   * @param spi SPI interface reference
   */
  explicit eeprom_api(i_chip_spi_api &spi);

  /*
   * eeprom api we want to have
   * */

  void writeByte(uint16_t address, uint8_t data) override;
  uint8_t readByte(uint16_t address) override;

  void writeBuffer(uint16_t address, const uint8_t *data,
                   std::size_t length) override;
  void readBuffer(uint16_t address, uint8_t *buffer,
                  std::size_t length) override;

  void writeBit(uint16_t address, uint8_t bitPosition, bool value) override;
  bool readBit(uint16_t address, uint8_t bitPosition) override;

private:
  /*
   * eeprom primitives
   * */

  /**
   * @brief Enable EEPROM write operations.
   */
  void writeEnable();

  /**
   * @brief Read EEPROM status register.
   * @return Status register value
   */
  uint8_t readStatus();

  /**
   * @brief Wait until EEPROM is ready for the next operation.
   */
  void waitUntilReady();

  /**
   * @brief Send a raw command byte to EEPROM.
   * @param command Command to send
   */
  void sendCommand(uint8_t command);

  /**
   * @brief Send a 9-bit EEPROM memory address over SPI.
   * @param address Memory address
   */
  void sendAddress(uint16_t address);

private:
  i_chip_spi_api &spi_api_; ///< SPI interface reference

  /*
   * eeprom commands
   * */
  static constexpr uint8_t CMD_READ = 0x03;
  static constexpr uint8_t CMD_WRITE = 0x02;
  static constexpr uint8_t CMD_WREN = 0x06;
  static constexpr uint8_t CMD_RDSR = 0x05;
  static constexpr uint8_t CMD_WRSR = 0x05;
};
