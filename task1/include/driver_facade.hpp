#pragma once

#include <cstdint>

struct i_chip_gpio_driver {
  virtual void setHigh(int pin) = 0;
  virtual void setLow(int pin) = 0;
  virtual bool read(int pin) = 0;
  virtual ~i_chip_gpio_driver() = default;
};

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
  virtual uint8_t transfer(uint8_t data) = 0;
  ~i_chip_spi_api() = default;
};

class chip_spi_api : public i_chip_spi_api {
public:
  chip_spi_api(i_chip_gpio_driver &gpio, int pinCS, int pinSCK, int pinMOSI,
               int pinMISO);

  void select() override;
  void deselect() override;

  uint8_t transfer(uint8_t data) override;

private:
  /*
   * SPI primitives
   * */

  void clockPulse();
  void writeBit(bool bit);
  bool readBit();

private:
  i_chip_gpio_driver &m_gpio;

  int pinCS_;
  int pinSCK_;
  int pinMOSI_;
  int pinMISO_;
};

class i_eeprom_api {
public:
  virtual void writeByte(uint16_t address, uint8_t data) = 0;
  virtual uint8_t readByte(uint16_t address) = 0;

  virtual void writeBuffer(uint16_t address, const uint8_t *data,
                           std::size_t length) = 0;
  virtual void readBuffer(uint16_t address, uint8_t *buffer,
                          std::size_t length) = 0;

  virtual void writeBit(uint16_t address, uint8_t bitPosition, bool value) = 0;
  virtual bool readBit(uint16_t address, uint8_t bitPosition) = 0;
  ~i_eeprom_api() = default;
};

class eeprom_api : public i_eeprom_api {
public:
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
  void writeEnable();
  uint8_t readStatus();
  void waitUntilReady();

  void sendCommand(uint8_t command);
  void sendAddress(uint16_t address);

private:
  i_chip_spi_api &spi_api_;

  /*
   * eeprom commands
   * */
  static constexpr uint8_t CMD_READ = 0x03;
  static constexpr uint8_t CMD_WRITE = 0x02;
  static constexpr uint8_t CMD_WREN = 0x06;
  static constexpr uint8_t CMD_RDSR = 0x05;
  static constexpr uint8_t CMD_WRSR = 0x05;
};
