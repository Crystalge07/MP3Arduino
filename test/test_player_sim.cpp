// Player with USE_DFPLAYER 0 (the simulated player): bug 1 affected it too.
#include <Arduino.h>
#include "Player.h"
#include "check.h"

static void fresh(Player &p) {
  Serial.out.clear();
  fakeMillis = 10000;
  p.connect();
}

// One loop() pass that handles a button: `now` is read at the top, some
// time passes (Serial output), then the button acts, then player.update().
template <class Action>
static void buttonPass(Player &p, Action act) {
  uint32_t now = millis();
  fakeMillis += 2;
  act();
  p.update(now);
}

TEST(bug1_sim_next_moves_one_track) {
  Player p;
  fresh(p);
  buttonPass(p, [&] { p.togglePlay(); });
  CHECK_EQ(p.state().track, 1);
  fakeMillis += 1000;
  buttonPass(p, [&] { p.next(); });
  CHECK_EQ(p.state().track, 2);
  CHECK_EQ(p.state().status, PS_PLAYING);
}

TEST(bug1_sim_resume_does_not_end_track) {
  Player p;
  fresh(p);
  buttonPass(p, [&] { p.togglePlay(); });   // play
  fakeMillis += 1000;
  buttonPass(p, [&] { p.togglePlay(); });   // pause
  fakeMillis += 1000;
  buttonPass(p, [&] { p.togglePlay(); });   // resume
  CHECK_EQ(p.state().track, 1);
  CHECK_EQ(p.state().status, PS_PLAYING);
}

TEST(bug1_sim_track_still_ends_after_its_length) {
  Player p;
  fresh(p);
  buttonPass(p, [&] { p.togglePlay(); });
  fakeMillis += SIM_TRACK_MS;
  p.update(millis());
  CHECK_EQ(p.state().track, 2);
}

int main() { return runTests("player (simulated)"); }
