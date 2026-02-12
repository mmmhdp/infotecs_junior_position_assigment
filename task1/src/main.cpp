#include <cstdint>

struct i_chip_gpio_driver {
  virtual void setHigh(int pin) = 0;
  virtual void setLow(int pin) = 0;
  virtual bool read(int pin) = 0;
  virtual ~i_chip_gpio_driver() = default;
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

int main (void) {
  return 0;
}
