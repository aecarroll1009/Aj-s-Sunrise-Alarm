#include "OledDriver.h"

namespace {
constexpr uint32_t IDLE_SLEEP_MS = 12000;  // spec: 10-15 s
}  // namespace

void OledDriver::begin() {
  u8g2_.begin();
  u8g2_.setContrast(255);
}

bool OledDriver::updateSleep(bool canSleep, uint32_t nowMs, bool activity) {
  if (activity) lastActivityMs_ = nowMs;
  bool sleep = canSleep && nowMs - lastActivityMs_ > IDLE_SLEEP_MS;
  if (sleep != asleep_) {
    u8g2_.setPowerSave(sleep);
    asleep_ = sleep;
  }
  return asleep_;
}

void OledDriver::render(const UIMenu& ui, const DisplayStatus& status,
                        uint32_t nowMs, bool activity) {
  // Only HOME sleeps; menus and the alarm screen stay lit.
  bool canSleep = !status.alarmActive && ui.screen() == UIMenu::Screen::HOME;
  if (updateSleep(canSleep, nowMs, activity)) return;

  u8g2_.firstPage();
  do {
    Screens::draw(u8g2_, ui, status, nowMs);
  } while (u8g2_.nextPage());
}
