#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

struct i_chip_gpio_driver {
  virtual void setHigh(int pin) = 0;
  virtual void setLow(int pin) = 0;
  virtual bool read(int pin) = 0;
  virtual ~i_chip_gpio_driver() = default;
};

class mock_chip_gpio_driver : public i_chip_gpio_driver {
public:
  void setHigh(int pin) override { std::cout << "Pin " << pin << " HIGH\n"; }

  void setLow(int pin) override { std::cout << "Pin " << pin << " LOW\n"; }

  bool read(int pin) override { return false; }
};

class chip_spi_api {
public:
  chip_spi_api(i_chip_gpio_driver &gpio, int pinCS, int pinSCK, int pinMOSI,
               int pinMISO);
  /*
   * To begin communication
   * master have to
   * set CS to LOW on the slave device
   *
   * on contrary, deselect ends
   * communication and sets
   * slave device's CS to HIGH
   * */

  void select();
  void deselect();

  /*
   * SPI assume data transfers by bits,
   * but we need byte transfer interface
   * inplace since EEPROM device is
   * capable only of understand
   * by byte and not by bit communication scenario
   * */
  uint8_t transfer(uint8_t data);

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

chip_spi_api::chip_spi_api(i_chip_gpio_driver &gpio, int pinCS, int pinSCK,
                           int pinMOSI, int pinMISO)
    : m_gpio(gpio), pinCS_(pinCS), pinSCK_(pinSCK), pinMOSI_(pinMOSI),
      pinMISO_(pinMISO) {
  deselect();
  m_gpio.setLow(pinSCK_);
}

void chip_spi_api::select() { m_gpio.setLow(pinCS_); }
void chip_spi_api::deselect() { m_gpio.setHigh(pinCS_); }

void chip_spi_api::clockPulse() {
  m_gpio.setHigh(pinSCK_);

  std::this_thread::sleep_for(std::chrono::microseconds(1));

  m_gpio.setLow(pinSCK_);

  std::this_thread::sleep_for(std::chrono::microseconds(1));
}

void chip_spi_api::writeBit(bool bit) {
  if (bit)
    m_gpio.setHigh(pinMOSI_);
  else
    m_gpio.setLow(pinMOSI_);
  clockPulse();
}

bool chip_spi_api::readBit() {
  clockPulse();
  return m_gpio.read(pinMISO_);
}

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

class eeprom_api {
public:
  explicit eeprom_api(chip_spi_api &spi);

  /*
   * eeprom api we want to have
   * */

  void writeByte(uint16_t address, uint8_t data);
  uint8_t readByte(uint16_t address);

  void writeBuffer(uint16_t address, const uint8_t *data, std::size_t length);
  void readBuffer(uint16_t address, uint8_t *buffer, std::size_t length);

  void writeBit(uint16_t address, uint8_t bitPosition, bool value);
  bool readBit(uint16_t address, uint8_t bitPosition);

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
  chip_spi_api &spi_api_;

  /*
   * eeprom commands
   * */
  static constexpr uint8_t CMD_READ = 0x03;
  static constexpr uint8_t CMD_WRITE = 0x02;
  static constexpr uint8_t CMD_WREN = 0x06;
  static constexpr uint8_t CMD_RDSR = 0x05;
  static constexpr uint8_t CMD_WRSR = 0x05;
};

eeprom_api::eeprom_api(chip_spi_api &spi) : spi_api_(spi) {}

void eeprom_api::writeEnable() {
  spi_api_.select();
  spi_api_.transfer(CMD_WREN);
  spi_api_.deselect();
}

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

void eeprom_api::sendCommand(uint8_t command) { spi_api_.transfer(command); }

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
void eeprom_api::writeByte(uint16_t address, uint8_t data) {
  writeEnable();

  spi_api_.select();
  sendCommand(CMD_WRITE);
  sendAddress(address);
  spi_api_.transfer(data);
  spi_api_.deselect();

  waitUntilReady();
}

uint8_t eeprom_api::readByte(uint16_t address) {
  spi_api_.select();
  sendCommand(CMD_READ);
  sendAddress(address);
  uint8_t data = spi_api_.transfer(0x00);
  spi_api_.deselect();
  return data;
}

void eeprom_api::writeBuffer(uint16_t address, const uint8_t *data,
                             std::size_t length) {
  for (std::size_t i = 0; i < length; ++i) {
    writeByte(address + i, data[i]);
  }
}

void eeprom_api::readBuffer(uint16_t address, uint8_t *buffer,
                            std::size_t length) {
  for (std::size_t i = 0; i < length; ++i) {
    buffer[i] = readByte(address + i);
  }
}

void eeprom_api::writeBit(uint16_t address, uint8_t bitPosition, bool value) {
  uint8_t byte = readByte(address);
  if (value)
    byte |= (1 << bitPosition);
  else
    byte &= ~(1 << bitPosition);
  writeByte(address, byte);
}

bool eeprom_api::readBit(uint16_t address, uint8_t bitPosition) {
  uint8_t byte = readByte(address);
  return (byte >> bitPosition) & 0x01;
}

int main(void) { return 0; }
