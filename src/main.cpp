\
#include "mcp23017.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#ifndef KEYPAD_LAYOUT_VID
#define KEYPAD_LAYOUT_VID 1
#endif

#ifndef KEYPAD_LONG_PRESS_MS
#define KEYPAD_LONG_PRESS_MS 1000
#endif

namespace {

constexpr int MATRIX_SIZE = 4;
constexpr std::chrono::milliseconds LONG_PRESS_THRESHOLD{
  KEYPAD_LONG_PRESS_MS
};

static_assert(KEYPAD_LONG_PRESS_MS > 0,
              "long press threshold must be positive");

struct KeypadLayout {
  const char* name;
  uint8_t row_mask;
  uint8_t col_mask;
  uint8_t row_bits[MATRIX_SIZE];
  uint8_t col_bits[MATRIX_SIZE];
  std::string_view keymap[MATRIX_SIZE][MATRIX_SIZE];
};

#if KEYPAD_LAYOUT_VID
constexpr KeypadLayout KEYPAD_LAYOUT = {
  "VID 14-key",
  0b1010'1010,  // PA1, PA3, PA5, PA7 are row outputs
  0b0101'0101,  // PA0, PA2, PA4, PA6 are column inputs
  {7, 5, 3, 1},
  {0, 2, 4, 6},
  {
    {"1",       "2", "3",         "START/ENTER"},
    {"4",       "5", "6",         "STOP/CANCEL"},
    {"7",       "8", "9",         ""},
    {"RUS/ENG", "0", "BACKSPACE", ""}
  }
};
#else
constexpr KeypadLayout KEYPAD_LAYOUT = {
  "legacy 4x4",
  0b0000'1111,  // PA0..PA3 are row outputs
  0b1111'0000,  // PA4..PA7 are column inputs
  {0, 1, 2, 3},
  {4, 5, 6, 7},
  {
    {"1", "2", "3", "A"},
    {"4", "5", "6", "B"},
    {"7", "8", "9", "C"},
    {"*", "0", "#", "D"}
  }
};
#endif

static_assert((KEYPAD_LAYOUT.row_mask & KEYPAD_LAYOUT.col_mask) == 0,
              "row and column GPIO masks must not overlap");
static_assert((KEYPAD_LAYOUT.row_mask | KEYPAD_LAYOUT.col_mask) == 0xFF,
              "keypad layout must use all MCP23017 port A pins");

static volatile std::sig_atomic_t g_stop = 0;

static void on_sigint(int) { g_stop = 1; }

static void usage(const char* argv0) {
  std::cerr
    << "Usage: " << argv0 << " [--dev /dev/i2c-X] [--addr 0x20] [--poll-ms 5]\n"
    << "\nDefaults:\n"
    << "  --dev     /dev/i2c-3\n"
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

static std::string_view scan_keypad(MCP23017& mcp) {
  for (int row = 0; row < MATRIX_SIZE; row++) {
    uint8_t output = KEYPAD_LAYOUT.row_mask;
    output &= static_cast<uint8_t>(
      ~(1u << KEYPAD_LAYOUT.row_bits[row])
    );
    mcp.write_olata(output);

    // Allow the GPIO expander and membrane contacts to settle.
    std::this_thread::sleep_for(std::chrono::microseconds(300));

    const uint8_t columns = static_cast<uint8_t>(
      mcp.read_gpioA() & KEYPAD_LAYOUT.col_mask
    );
    if (columns == KEYPAD_LAYOUT.col_mask) {
      continue;
    }

    for (int column = 0; column < MATRIX_SIZE; column++) {
      const uint8_t bit = static_cast<uint8_t>(
        1u << KEYPAD_LAYOUT.col_bits[column]
      );
      if ((columns & bit) == 0) {
        const std::string_view key = KEYPAD_LAYOUT.keymap[row][column];
        if (!key.empty()) {
          mcp.write_olata(KEYPAD_LAYOUT.row_mask);
          return key;
        }
      }
    }
  }

  mcp.write_olata(KEYPAD_LAYOUT.row_mask);
  return {};
}

static void report_press(std::string_view key, std::string_view duration) {
  std::cout << "Pressed: " << key << " (" << duration << ")\n";
}

}  // namespace

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

  try {
    MCP23017 mcp(dev, addr);
    mcp.open_bus();

    // Rows = outputs (0), Cols = inputs (1)
    // IODIR bit: 1=input, 0=output
    mcp.configure_portA(
      KEYPAD_LAYOUT.col_mask,
      KEYPAD_LAYOUT.col_mask
    );

    // Set all rows HIGH initially (inactive)
    // For outputs, OLATA bit=1 -> drive high.
    // Column latch bits are don't-care because those pins are inputs.
    mcp.write_olata(KEYPAD_LAYOUT.row_mask);

    std::cout << "MCP23017 keypad demo started\n"
              << "  Layout  : " << KEYPAD_LAYOUT.name << "\n"
              << "  Long key: " << LONG_PRESS_THRESHOLD.count() << " ms\n"
              << "  I2C dev : " << dev << "\n"
              << "  Address : 0x" << std::hex << int(addr) << std::dec << "\n"
              << "Press Ctrl+C to stop.\n";

    bool waiting_release = false;
    bool long_reported = false;
    std::string_view active_key;
    std::chrono::steady_clock::time_point pressed_at;

    while (!g_stop) {
      const std::string_view found = scan_keypad(mcp);

      if (!waiting_release) {
        if (!found.empty()) {
          const auto detected_at = std::chrono::steady_clock::now();

          // Debounce: confirm after a short delay
          std::this_thread::sleep_for(std::chrono::milliseconds(20));

          const std::string_view confirm = scan_keypad(mcp);

          if (confirm == found) {
            active_key = found;
            pressed_at = detected_at;
            long_reported = false;
            waiting_release = true;

            if (std::chrono::steady_clock::now() - pressed_at >=
                LONG_PRESS_THRESHOLD) {
              report_press(active_key, "long");
              long_reported = true;
            }
          }
        }
      } else {
        if (!found.empty()) {
          if (!long_reported && found == active_key &&
              std::chrono::steady_clock::now() - pressed_at >=
                LONG_PRESS_THRESHOLD) {
            report_press(active_key, "long");
            long_reported = true;
          }
        } else {
          // Confirm release before classifying a press shorter than the
          // long-press threshold.
          const auto release_detected_at = std::chrono::steady_clock::now();
          std::this_thread::sleep_for(std::chrono::milliseconds(30));
          const std::string_view again = scan_keypad(mcp);

          if (again.empty()) {
            if (!long_reported) {
              const bool is_long =
                release_detected_at - pressed_at >= LONG_PRESS_THRESHOLD;
              report_press(active_key, is_long ? "long" : "short");
            }

            waiting_release = false;
            long_reported = false;
            active_key = {};
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
