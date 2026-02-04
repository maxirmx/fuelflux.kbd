\
#include "mcp23017.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifndef I2C_SLAVE
#include <linux/i2c-dev.h>
#endif

// MCP23017 register map, BANK=0 (default)
static constexpr uint8_t REG_IODIRA = 0x00;
static constexpr uint8_t REG_IPOLA  = 0x02;
static constexpr uint8_t REG_GPPUA  = 0x0C;
static constexpr uint8_t REG_GPIOA  = 0x12;
static constexpr uint8_t REG_OLATA  = 0x14;

static std::runtime_error sys_err(const char* what) {
  return std::runtime_error(std::string(what) + ": " + std::strerror(errno));
}

MCP23017::MCP23017(std::string i2c_dev, uint8_t i2c_addr)
  : dev_(std::move(i2c_dev)), addr_(i2c_addr) {}

MCP23017::~MCP23017() { close_bus(); }

void MCP23017::open_bus() {
  if (fd_ >= 0) return;

  fd_ = ::open(dev_.c_str(), O_RDWR | O_CLOEXEC);
  if (fd_ < 0) throw sys_err("open(i2c)");

  if (::ioctl(fd_, I2C_SLAVE, addr_) < 0) {
    int saved = errno;
    ::close(fd_);
    fd_ = -1;
    errno = saved;
    throw sys_err("ioctl(I2C_SLAVE)");
  }
}

void MCP23017::close_bus() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

void MCP23017::ensure_open() const {
  if (fd_ < 0) throw std::runtime_error("MCP23017: bus is not open");
}

uint8_t MCP23017::read_reg(uint8_t reg) {
  ensure_open();
  // Write register address
  uint8_t wbuf[1] = { reg };
  if (::write(fd_, wbuf, 1) != 1) throw sys_err("i2c write(reg)");

  // Read back one byte
  uint8_t rbuf[1] = { 0 };
  if (::read(fd_, rbuf, 1) != 1) throw sys_err("i2c read(data)");
  return rbuf[0];
}

void MCP23017::write_reg(uint8_t reg, uint8_t value) {
  ensure_open();
  uint8_t buf[2] = { reg, value };
  if (::write(fd_, buf, 2) != 2) throw sys_err("i2c write(reg,value)");
}

void MCP23017::configure_portA(uint8_t iodir, uint8_t gppu, uint8_t ipol) {
  write_reg(REG_IODIRA, iodir);
  write_reg(REG_GPPUA,  gppu);
  write_reg(REG_IPOLA,  ipol);
}

uint8_t MCP23017::read_gpioA() { return read_reg(REG_GPIOA); }

void MCP23017::write_olata(uint8_t value) { write_reg(REG_OLATA, value); }
