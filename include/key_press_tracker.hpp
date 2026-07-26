#pragma once

#include <chrono>
#include <optional>
#include <string_view>

enum class KeyPressKind {
  Short,
  Long
};

struct KeyPressEvent {
  std::string_view key;
  KeyPressKind kind;
};

class KeyPressTracker {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  KeyPressTracker(
    std::chrono::milliseconds long_press_threshold,
    std::chrono::milliseconds press_debounce,
    std::chrono::milliseconds release_debounce
  );

  std::optional<KeyPressEvent> update(
    std::string_view sampled_key,
    TimePoint sampled_at
  );

private:
  enum class State {
    Idle,
    DebouncingPress,
    Active,
    DebouncingRelease
  };

  std::chrono::milliseconds long_press_threshold_;
  std::chrono::milliseconds press_debounce_;
  std::chrono::milliseconds release_debounce_;

  State state_{State::Idle};
  std::string_view candidate_key_;
  std::string_view active_key_;
  TimePoint candidate_since_;
  TimePoint pressed_at_;
  TimePoint release_candidate_since_;
  bool long_reported_{false};

  void begin_press_candidate(std::string_view key, TimePoint sampled_at);
  void reset_to_idle();
  std::optional<KeyPressEvent> activate_candidate(TimePoint sampled_at);
  std::optional<KeyPressEvent> maybe_report_long(TimePoint sampled_at);
  std::optional<KeyPressEvent> classify_active(TimePoint released_at) const;
};
