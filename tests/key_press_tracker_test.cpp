#include "key_press_tracker.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

using namespace std::chrono_literals;

using Event = std::optional<KeyPressEvent>;
using TimePoint = KeyPressTracker::TimePoint;

TimePoint at(std::chrono::milliseconds offset) {
  return TimePoint{} + offset;
}

[[noreturn]] void fail(std::string_view message) {
  std::cerr << "FAIL: " << message << "\n";
  std::exit(1);
}

void expect_none(const Event& event, std::string_view context) {
  if (event) {
    fail(context);
  }
}

void expect_event(
  const Event& event,
  std::string_view key,
  KeyPressKind kind,
  std::string_view context
) {
  if (!event || event->key != key || event->kind != kind) {
    fail(context);
  }
}

KeyPressTracker make_tracker() {
  return KeyPressTracker(1000ms, 20ms, 30ms);
}

void test_short_press() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "short: initial sample");
  expect_none(tracker.update("A", at(20ms)), "short: press debounce");
  expect_none(tracker.update("", at(999ms)), "short: release candidate");
  expect_event(
    tracker.update("", at(1029ms)),
    "A",
    KeyPressKind::Short,
    "short: confirmed release"
  );
}

void test_long_press_reports_once() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "long: initial sample");
  expect_none(tracker.update("A", at(20ms)), "long: press debounce");
  expect_none(tracker.update("A", at(999ms)), "long: before threshold");
  expect_event(
    tracker.update("A", at(1000ms)),
    "A",
    KeyPressKind::Long,
    "long: threshold"
  );
  expect_none(tracker.update("A", at(1500ms)), "long: no repeat");
  expect_none(tracker.update("", at(1501ms)), "long: release candidate");
  expect_none(tracker.update("", at(1531ms)), "long: no short on release");
}

void test_release_bounce_does_not_end_press() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "bounce: initial sample");
  expect_none(tracker.update("A", at(20ms)), "bounce: press debounce");
  expect_none(tracker.update("", at(990ms)), "bounce: transient empty");
  expect_none(tracker.update("A", at(995ms)), "bounce: active key returned");
  expect_event(
    tracker.update("A", at(1000ms)),
    "A",
    KeyPressKind::Long,
    "bounce: original timing retained"
  );
}

void test_direct_key_change_restarts_timing() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "change: A initial sample");
  expect_none(tracker.update("A", at(20ms)), "change: A press debounce");
  expect_event(
    tracker.update("B", at(500ms)),
    "A",
    KeyPressKind::Short,
    "change: A completed"
  );
  expect_none(tracker.update("B", at(520ms)), "change: B press debounce");
  expect_none(tracker.update("B", at(1499ms)), "change: B independent timer");
  expect_event(
    tracker.update("B", at(1500ms)),
    "B",
    KeyPressKind::Long,
    "change: B threshold"
  );
}

void test_key_change_during_release_debounce() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "release change: A initial");
  expect_none(tracker.update("A", at(20ms)), "release change: A debounce");
  expect_none(tracker.update("", at(400ms)), "release change: empty");
  expect_event(
    tracker.update("B", at(405ms)),
    "A",
    KeyPressKind::Short,
    "release change: A uses first continuous empty"
  );
  expect_none(tracker.update("B", at(425ms)), "release change: B debounce");
  expect_none(tracker.update("", at(500ms)), "release change: B empty");
  expect_event(
    tracker.update("", at(530ms)),
    "B",
    KeyPressKind::Short,
    "release change: B release"
  );
}

void test_long_boundary_on_release() {
  auto tracker = make_tracker();

  expect_none(tracker.update("A", at(0ms)), "boundary: initial sample");
  expect_none(tracker.update("A", at(20ms)), "boundary: press debounce");
  expect_none(tracker.update("", at(1000ms)), "boundary: release candidate");
  expect_event(
    tracker.update("", at(1030ms)),
    "A",
    KeyPressKind::Long,
    "boundary: exactly threshold is long"
  );
}

}  // namespace

int main() {
  test_short_press();
  test_long_press_reports_once();
  test_release_bounce_does_not_end_press();
  test_direct_key_change_restarts_timing();
  test_key_change_during_release_debounce();
  test_long_boundary_on_release();

  std::cout << "All key press tracker tests passed.\n";
  return 0;
}
