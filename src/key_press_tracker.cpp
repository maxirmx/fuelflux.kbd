#include "key_press_tracker.hpp"

KeyPressTracker::KeyPressTracker(
  std::chrono::milliseconds long_press_threshold,
  std::chrono::milliseconds press_debounce,
  std::chrono::milliseconds release_debounce
)
  : long_press_threshold_(long_press_threshold),
    press_debounce_(press_debounce),
    release_debounce_(release_debounce) {}

void KeyPressTracker::begin_press_candidate(
  std::string_view key,
  TimePoint sampled_at
) {
  state_ = State::DebouncingPress;
  candidate_key_ = key;
  candidate_since_ = sampled_at;
  active_key_ = {};
  long_reported_ = false;
}

void KeyPressTracker::reset_to_idle() {
  state_ = State::Idle;
  candidate_key_ = {};
  active_key_ = {};
  long_reported_ = false;
}

std::optional<KeyPressEvent> KeyPressTracker::activate_candidate(
  TimePoint sampled_at
) {
  state_ = State::Active;
  active_key_ = candidate_key_;
  pressed_at_ = candidate_since_;
  candidate_key_ = {};
  long_reported_ = false;
  return maybe_report_long(sampled_at);
}

std::optional<KeyPressEvent> KeyPressTracker::maybe_report_long(
  TimePoint sampled_at
) {
  if (!long_reported_ &&
      sampled_at - pressed_at_ >= long_press_threshold_) {
    long_reported_ = true;
    return KeyPressEvent{active_key_, KeyPressKind::Long};
  }

  return std::nullopt;
}

std::optional<KeyPressEvent> KeyPressTracker::classify_active(
  TimePoint released_at
) const {
  if (long_reported_) {
    return std::nullopt;
  }

  const KeyPressKind kind =
    released_at - pressed_at_ >= long_press_threshold_
      ? KeyPressKind::Long
      : KeyPressKind::Short;
  return KeyPressEvent{active_key_, kind};
}

std::optional<KeyPressEvent> KeyPressTracker::update(
  std::string_view sampled_key,
  TimePoint sampled_at
) {
  switch (state_) {
    case State::Idle:
      if (!sampled_key.empty()) {
        begin_press_candidate(sampled_key, sampled_at);
      }
      return std::nullopt;

    case State::DebouncingPress:
      if (sampled_key.empty()) {
        reset_to_idle();
      } else if (sampled_key != candidate_key_) {
        begin_press_candidate(sampled_key, sampled_at);
      } else if (sampled_at - candidate_since_ >= press_debounce_) {
        return activate_candidate(sampled_at);
      }
      return std::nullopt;

    case State::Active:
      if (sampled_key == active_key_) {
        return maybe_report_long(sampled_at);
      }

      if (sampled_key.empty()) {
        state_ = State::DebouncingRelease;
        release_candidate_since_ = sampled_at;
        return std::nullopt;
      }

      {
        const auto event = classify_active(sampled_at);
        begin_press_candidate(sampled_key, sampled_at);
        return event;
      }

    case State::DebouncingRelease:
      if (sampled_key.empty()) {
        if (sampled_at - release_candidate_since_ >= release_debounce_) {
          const auto event = classify_active(release_candidate_since_);
          reset_to_idle();
          return event;
        }
        return std::nullopt;
      }

      if (sampled_key == active_key_) {
        state_ = State::Active;
        return maybe_report_long(sampled_at);
      }

      {
        const auto event = classify_active(release_candidate_since_);
        begin_press_candidate(sampled_key, sampled_at);
        return event;
      }
  }

  return std::nullopt;
}
