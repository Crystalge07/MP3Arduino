#include "Settings.h"
#include <EEPROM.h>

// Byte 0 marks the data as ours; bump the version when Saved changes so old
// data is ignored instead of misread.
constexpr int     ADDR_MAGIC = 0;
constexpr int     ADDR_DATA  = 1;
constexpr uint8_t MAGIC      = 0xA1;   // 0xA0 + layout version 1

bool settingsLoad(Saved &s) {
  if (EEPROM.read(ADDR_MAGIC) != MAGIC) return false;
  EEPROM.get(ADDR_DATA, s);
  return true;
}

void settingsSave(const Saved &s) {
  EEPROM.put(ADDR_DATA, s);            // put() skips bytes that are unchanged
  EEPROM.update(ADDR_MAGIC, MAGIC);
}
