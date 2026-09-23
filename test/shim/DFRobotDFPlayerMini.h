// A fake DFPlayer with the same interface as DFRobotDFPlayerMini 1.0.6.
//
// Tests describe the card (which /mp3 numbers exist, what count the module
// reports) and queue messages the module "sends". Like the real library,
// every command costs ~20 ms of blocking time, so the fake clock moves
// while Player talks to it.
#pragma once
#include <stdint.h>
#include <deque>
#include <set>
#include <vector>

// Message types and error codes, same values as the real library.
#define TimeOut              0
#define WrongStack           1
#define DFPlayerCardInserted 2
#define DFPlayerCardRemoved  3
#define DFPlayerCardOnline   4
#define DFPlayerPlayFinished 5
#define DFPlayerError        6
#define DFPlayerFeedBack     11

#define FileIndexOut 5
#define FileMismatch 6

namespace fakedf {
struct Msg { uint8_t type; uint16_t value; };

extern std::deque<Msg>  incoming;       // what the module will send next
extern std::vector<int> played;         // every playMp3Folder() call, in order
extern std::set<int>    files;          // /mp3/NNNN.mp3 numbers on the card
extern int              reportedCount;  // readFileCounts() answer; -1 = silent

// One command: 10 bytes at 9600 baud (10.4 ms, blocking) + the library's delay(10).
constexpr uint32_t CMD_MS = 20;

void reset();
}  // namespace fakedf

class DFRobotDFPlayerMini {
public:
  template <class S>
  bool begin(S &, bool = true, bool = true) { return true; }

  bool     available();
  uint8_t  readType() { _avail = false; return _type; }
  uint16_t read()     { _avail = false; return _value; }
  int      readFileCounts();

  void playMp3Folder(int n);
  void stop();
  void volume(uint8_t);
  void EQ(uint8_t);
  void pause();
  void start();

private:
  uint8_t  _type = 0;
  uint16_t _value = 0;
  bool     _avail = false;
};
