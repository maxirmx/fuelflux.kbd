\
#include "mcp23017.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

static volatile std::sig_atomic_t g_stop = 0;

static void on_sigint(int) { g_stop = 1; }

static void usage(const char* argv0) {
  std::cerr
    << "Usage: " << argv0 << " [--dev /dev/i2c-X] [--addr 0x20] [--poll-ms 5]\n"
    << "\nDefaults:\n"
    << "  --dev     /dev/i2c-3n"
    << "  --addr    0x20\n"
    << "  --poll-ms 5\n";
}

// Parse hex like 0x20 or decimal
static uint8_t parse_u8(const std::string& s) {
  char* end = nullptr;
  long v = std::strtol(s.c_str(), &end, 0);
  if (!end || *end != '\0' || v < 0 || v > 0x7F) {
    throw std::runtime_error("Invalid address value: " + s);
  }
  return static_cast<uint8_t>(v);
}

int main(int argc, char** argv) {
  std::signal(SIGINT, on_sigint);

  std::string dev = "/dev/i2c-3";
  uint8_t addr = 0x20;
  int poll_ms = 5;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--help" || a == "-h") {
      usage(argv[0]);
      return 0;
    } else if (a == "--dev" && i + 1 < argc) {
      dev = argv[++i];
    } else if (a == "--addr" && i + 1 < argc) {
      addr = parse_u8(argv[++i]);
    } else if (a == "--poll-ms" && i + 1 < argc) {
      poll_ms = std::stoi(argv[++i]);
      if (poll_ms < 1) poll_ms = 1;
    } else {
      std::cerr << "Unknown arg: " << a << "\n";
      usage(argv[0]);
      return 2;
    }
  }

  // Key layout:
  // R1: 1 2 3 A
  // R2: 4 5 6 B
  // R3: 7 8 9 C
  // R4: * 0 # D
  static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };

  // Port A mapping (your requirement):
  // PA0..PA3 rows (outputs)
  // PA4..PA7 cols (inputs with pull-ups)
  constexpr uint8_t ROW_MASK = 0b0000'1111;  // PA0..PA3
  constexpr uint8_t COL_MASK = 0b1111'0000;  // PA4..PA7

  try {
    MCP23017 mcp(dev, addr);
    mcp.open_bus();

    // Rows = outputs (0), Cols = inputs (1)
    // IODIR bit: 1=input, 0=output
    uint8_t iodirA = (COL_MASK) | 0x00;              // PA4..PA7 inputs, PA0..PA3 outputs
    uint8_t gppuA  = (COL_MASK);                     // pull-ups on PA4..PA7
    mcp.configure_portA(iodirA, gppuA);

    // Set all rows HIGH initially (inactive)
    // For outputs, OLATA bit=1 -> drive high.
    // Keep upper bits don't-care for inputs.
    uint8_t rows_idle = ROW_MASK; // PA0..PA3 high
    mcp.write_olata(rows_idle);

    std::cout << "MCP23017 keypad demo started\n"
              << "  I2C dev : " << dev << "\n"
              << "  Address : 0x" << std::hex << int(addr) << std::dec << "\n"
              << "Press Ctrl+C to stop.\n";

    char last_key = '\0';
    bool waiting_release = false;

    while (!g_stop) {
      char found = '\0';

      // Scan each row: drive one row LOW, other rows HIGH, read columns.
      for (int r = 0; r < 4 && !found; r++) {
        uint8_t out = rows_idle;

        // Drive current row LOW => clear that bit
        out &= static_cast<uint8_t>(~(1u << r));
        mcp.write_olata(out);

        // Small settle time (MCP is fast, but keypad + wiring benefits)
        std::this_thread::sleep_for(std::chrono::microseconds(300));

        uint8_t gpioA = mcp.read_gpioA();
        uint8_t cols = static_cast<uint8_t>(gpioA & COL_MASK);

        // With pull-ups, idle cols are HIGH (1). Press makes LOW (0) on the active row.
        if (cols != COL_MASK) {
          for (int c = 0; c < 4; c++) {
            uint8_t bit = static_cast<uint8_t>(1u << (4 + c));
            if ((cols & bit) == 0) {
              found = keymap[r][c];
              break;
            }
          }
        }
      }

      // Restore idle state each scan loop
      mcp.write_olata(rows_idle);

      if (!waiting_release) {
        if (found) {
          // Debounce: confirm after a short delay
          std::this_thread::sleep_for(std::chrono::milliseconds(20));

          // Re-check quickly (one more scan)
          char confirm = '\0';
          for (int r = 0; r < 4 && !confirm; r++) {
            uint8_t out = rows_idle & static_cast<uint8_t>(~(1u << r));
            mcp.write_olata(out);
            std::this_thread::sleep_for(std::chrono::microseconds(300));
            uint8_t cols = static_cast<uint8_t>(mcp.read_gpioA() & COL_MASK);
            if (cols != COL_MASK) {
              for (int c = 0; c < 4; c++) {
                uint8_t bit = static_cast<uint8_t>(1u << (4 + c));
                if ((cols & bit) == 0) {
                  confirm = keymap[r][c];
                  break;
                }
              }
            }
          }
          mcp.write_olata(rows_idle);

          if (confirm == found) {
            std::cout << "Pressed: " << found << "\n";
            last_key = found;
            waiting_release = true;
          }
        }
      } else {
        // Wait for release: no key must be detected for some time
        if (!found) {
          std::this_thread::sleep_for(std::chrono::milliseconds(30));
          // Still no key?
          char again = '\0';
          for (int r = 0; r < 4 && !again; r++) {
            uint8_t out = rows_idle & static_cast<uint8_t>(~(1u << r));
            mcp.write_olata(out);
            std::this_thread::sleep_for(std::chrono::microseconds(300));
            uint8_t cols = static_cast<uint8_t>(mcp.read_gpioA() & COL_MASK);
            if (cols != COL_MASK) {
              for (int c = 0; c < 4; c++) {
                uint8_t bit = static_cast<uint8_t>(1u << (4 + c));
                if ((cols & bit) == 0) {
                  again = keymap[r][c];
                  break;
                }
              }
            }
          }
          mcp.write_olata(rows_idle);

          if (!again) {
            waiting_release = false;
            last_key = '\0';
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }

    std::cout << "Stopped.\n";
    return 0;

  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << "\n";
    return 1;
  }
}
