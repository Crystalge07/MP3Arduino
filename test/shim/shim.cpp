#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <stdlib.h>

uint32_t   fakeMillis = 0;
FakeSerial Serial;

namespace fakedf {
std::deque<Msg>  incoming;
std::vector<int> played;
std::set<int>    files;
int              reportedCount = 0;

void reset() {
  incoming.clear();
  played.clear();
  files.clear();
  reportedCount = 0;
}
}  // namespace fakedf

using namespace fakedf;

bool DFRobotDFPlayerMini::available() {
  if (!_avail && !incoming.empty()) {
    _type = incoming.front().type;
    _value = incoming.front().value;
    incoming.pop_front();
    _avail = true;
  }
  return _avail;
}

int DFRobotDFPlayerMini::readFileCounts() {
  fakeMillis += CMD_MS;
  if (reportedCount < 0) {
    fakeMillis += 500;       // waited for a reply that never came
    _type = TimeOut;
    return -1;
  }
  _type = DFPlayerFeedBack;
  return reportedCount;
}

void DFRobotDFPlayerMini::playMp3Folder(int n) {
  fakeMillis += CMD_MS;
  played.push_back(n);
  if (played.size() > 1000) {
    fprintf(stderr, "fake DFPlayer: over 1000 play commands, Player is looping\n");
    abort();
  }
  // The real module answers a missing file with error 6.
  if (!files.count(n)) incoming.push_back({ DFPlayerError, FileMismatch });
}

void DFRobotDFPlayerMini::stop()      { fakeMillis += CMD_MS; }
void DFRobotDFPlayerMini::volume(uint8_t) { fakeMillis += CMD_MS; }
void DFRobotDFPlayerMini::EQ(uint8_t) { fakeMillis += CMD_MS; }
void DFRobotDFPlayerMini::pause()     { fakeMillis += CMD_MS; }
void DFRobotDFPlayerMini::start()     { fakeMillis += CMD_MS; }
