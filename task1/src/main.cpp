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

int main(void) { return 0; }
