// Player with the (fake) DFPlayer: bugs 1-3 from the code review.
#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include "Player.h"
#include "check.h"

using namespace fakedf;

static const uint32_t TRACK_LENGTH_MS = 180000;

// A card holding /mp3/0001..NNNN for every number in `present`, on which
// the module reports `reported` files (more than exist if there are stray
// files, such as macOS "._" copies).
static void card(std::set<int> present, int reported) {
  reset();
  files = present;
  reportedCount = reported;
  Serial.out.clear();
  fakeMillis = 10000;
}

static std::set<int> range(int first, int last) {
  std::set<int> s;
  for (int i = first; i <= last; i++) s.insert(i);
  return s;
}

// One loop() pass: `now` is read at the top, then some time goes on
// buttons.update() and Serial output before the player sees the events.
static void pass(Player &p) {
  uint32_t now = millis();
  fakeMillis += 2;
  p.update(now);
}

static void startPlaying(Player &p) {
  p.connect();
  p.togglePlay();
  pass(p);
}

static void trackEnds(Player &p) {
  fakeMillis += TRACK_LENGTH_MS;
  incoming.push_back({ DFPlayerPlayFinished, 0 });
  pass(p);
}

static bool warned() { return Serial.out.find("WARN:") != std::string::npos; }

// ---------------------------------------------------------------------------
// Bug 1: the duplicate-"finished" guard compared against a stale `now`.
// ---------------------------------------------------------------------------

TEST(bug1_duplicate_finished_advances_one_track) {
  Player p;
  card(range(1, 10), 10);
  startPlaying(p);
  fakeMillis += TRACK_LENGTH_MS;
  // Many modules send "finished" twice; both are waiting in the buffer.
  incoming.push_back({ DFPlayerPlayFinished, 1 });
  incoming.push_back({ DFPlayerPlayFinished, 1 });
  pass(p);
  CHECK_EQ(p.state().track, 2);
  CHECK_EQ(played.size(), 2u);   // track 1, then track 2 only
}

TEST(bug1_finished_for_interrupted_track_is_ignored_after_next) {
  Player p;
  card(range(1, 10), 10);
  startPlaying(p);
  fakeMillis += 5000;
  // The loop pass that handles the Next button: `now` is read, time
  // passes, Next starts track 2, and the module reports the interrupted
  // track 1 as "finished".
  uint32_t now = millis();
  fakeMillis += 2;
  p.next();
  incoming.push_back({ DFPlayerPlayFinished, 1 });
  p.update(now);
  CHECK_EQ(p.state().track, 2);
  CHECK_EQ(p.state().status, PS_PLAYING);
}

TEST(bug1_guard_works_across_millis_rollover) {
  Player p;
  card(range(1, 10), 10);
  p.connect();
  fakeMillis = 0xFFFFFFFFu - 5;   // millis() wraps a few ms from now
  p.togglePlay();
  pass(p);
  incoming.push_back({ DFPlayerPlayFinished, 1 });   // duplicate, after the wrap
  pass(p);
  CHECK_EQ(p.state().track, 1);
  trackEnds(p);                                      // the real end still counts
  CHECK_EQ(p.state().track, 2);
}

TEST(bug1_real_track_end_still_advances) {
  Player p;
  card(range(1, 10), 10);
  startPlaying(p);
  trackEnds(p);
  trackEnds(p);
  CHECK_EQ(p.state().track, 3);
}

// ---------------------------------------------------------------------------
// Bug 2: the reported count includes every file on the card.
// ---------------------------------------------------------------------------

TEST(bug2_inflated_count_wraps_to_track_1_after_last_real_track) {
  Player p;
  card(range(1, 3), 6);   // 3 songs + 3 hidden "._" files
  startPlaying(p);
  trackEnds(p);
  trackEnds(p);
  trackEnds(p);           // after track 3, track 4 doesn't exist
  CHECK_EQ(p.state().error, PE_NONE);
  CHECK_EQ(p.state().track, 1);
  CHECK_EQ(p.state().status, PS_PLAYING);
  CHECK_EQ(p.state().trackCount, 3);
  CHECK((played == std::vector<int>{ 1, 2, 3, 4, 1 }));
  CHECK(warned());
}

TEST(bug2_learned_count_is_kept) {
  Player p;
  card(range(1, 3), 6);
  startPlaying(p);
  p.next(); pass(p);
  p.next(); pass(p);
  p.next(); pass(p);      // 4 is missing: wraps to 1 and learns 3
  played.clear();
  p.next(); pass(p);
  p.next(); pass(p);
  p.next(); pass(p);      // 3 -> 1 directly now
  CHECK((played == std::vector<int>{ 2, 3, 1 }));
}

TEST(bug2_prev_from_track_1_finds_real_last_track) {
  Player p;
  card(range(1, 3), 6);
  startPlaying(p);
  p.previous();
  pass(p);
  CHECK_EQ(p.state().error, PE_NONE);
  CHECK_EQ(p.state().track, 3);
  CHECK_EQ(p.state().trackCount, 3);
  CHECK((played == std::vector<int>{ 1, 6, 5, 4, 3 }));
}

TEST(bug2_missing_track_1_is_an_error) {
  Player p;
  card(range(2, 5), 5);   // no 0001.mp3
  startPlaying(p);
  CHECK_EQ(p.state().error, PE_NO_FILES);
  CHECK_EQ(played.size(), 1u);
}

TEST(bug2_correct_count_does_not_warn) {
  Player p;
  card(range(1, 3), 3);
  startPlaying(p);
  trackEnds(p);
  trackEnds(p);
  trackEnds(p);
  CHECK_EQ(p.state().track, 1);
  CHECK((played == std::vector<int>{ 1, 2, 3, 1 }));
  CHECK(!warned());
}

// ---------------------------------------------------------------------------
// Bug 3: missing-file errors added up across separate skips, and Prev into
// a gap went forward.
// ---------------------------------------------------------------------------

TEST(bug3_separate_prev_skips_over_gaps_do_not_error) {
  Player p;
  std::set<int> f = range(1, 10);
  f.erase(3); f.erase(5); f.erase(7);
  card(f, 10);
  startPlaying(p);
  // Walk back from track 1 (wrapping to 10) over all three gaps, one
  // button press at a time, never letting a track finish.
  for (int i = 0; i < 6; i++) {
    fakeMillis += 3000;
    p.previous();
    pass(p);
  }
  CHECK_EQ(p.state().error, PE_NONE);
  CHECK_EQ(p.state().track, 2);   // 10 9 8 6 4 2
  CHECK_EQ(p.state().trackCount, 10);
  CHECK(!warned());
}

TEST(bug3_prev_into_gap_keeps_going_backward) {
  Player p;
  std::set<int> f = range(1, 10);
  f.erase(7);
  card(f, 10);
  startPlaying(p);
  p.previous(); pass(p);   // 10
  p.previous(); pass(p);   // 9
  p.previous(); pass(p);   // 8
  p.previous(); pass(p);   // 7 is missing
  CHECK_EQ(p.state().track, 6);
  CHECK_EQ(played.back(), 6);
}

TEST(bug3_backward_walk_ends_in_error_when_nothing_below_exists) {
  Player p;
  card({ 1, 9, 10 }, 10);
  startPlaying(p);
  p.previous(); pass(p);   // 10
  p.previous(); pass(p);   // 9
  files.erase(1);          // card now only has 9 and 10
  p.previous(); pass(p);   // 8, 7 ... 1 all missing
  CHECK_EQ(p.state().error, PE_NO_FILES);
  CHECK_EQ(played.back(), 1);
}

int main() { return runTests("player (DFPlayer)"); }
