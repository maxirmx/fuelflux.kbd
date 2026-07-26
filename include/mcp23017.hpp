#pragma once

#include <cstdint>
#include <string>

class MCP23017 {
public:
  enum class Port : uint8_t {
    A,
    B
  };

  MCP23017(std::string i2c_dev, uint8_t i2c_addr);
  ~MCP23017();

  MCP23017(const MCP23017&) = delete;
  MCP23017& operator=(const MCP23017&) = delete;

  void open_bus();
  void close_bus();

  // GPIO port helpers (BANK=0 addressing)
  void configure_port(
    Port port,
    uint8_t iodir,
    uint8_t gppu,
    uint8_t ipol = 0x00
  );
  uint8_t read_gpio(Port port);
  void write_olat(Port port, uint8_t value);

private:
  std::string dev_;
  uint8_t addr_;
  int fd_{-1};

  void ensure_open() const;
  uint8_t read_reg(uint8_t reg);
  void write_reg(uint8_t reg, uint8_t value);
};
