#include "config.h"
#include "view.h"

#include <cstddef>

#include "panic.h"

static View *views[3]{};
static size_t viewsLen = 0;

View *View::peek() {
  if (viewsLen == 0)
    return nullptr;
  return views[viewsLen - 1];
}

void View::push(View *view) {
  if (viewsLen >= sizeof(views) / sizeof(*views))
    panic2("View stack overflow");

  views[viewsLen++] = view;
}

void View::pop() {
  if (viewsLen == 0)
    panic2("View stack underflow");

  views[--viewsLen] = nullptr;
}

void View::paint() {
  if (viewsLen == 0)
    panic2("View stack empty");

  views[viewsLen - 1]->handlePaint();
}

void View::sendKeys(const UsbkChanges &changes) {
  if (viewsLen == 0)
    panic2("View stack empty");

  views[viewsLen - 1]->handleKey(changes);
}

void View::sendMakeBreak(std::bitset<8> usbkModifiers, uint8_t usbkSelector) {
  // FIXME: may not be correct if interleaved with real usbk input via sendKeys!
  UsbkChanges makeChanges{
    UsbkReport{(uint8_t)usbkModifiers.to_ulong(), {}, {usbkSelector}},
    {}, {{usbkSelector, true}}, 0, 1};
  UsbkChanges breakChanges{
    UsbkReport{0, {}, {}},
    {}, {{usbkSelector, false}}, 0, 1};
  for (size_t i = 0; i < 8; i++) {
    if (usbkModifiers[i]) {
      makeChanges.dv[makeChanges.dvLen++] = DvChange{(uint8_t)((uint8_t)1 << i), true};
      breakChanges.dv[breakChanges.dvLen++] = DvChange{(uint8_t)((uint8_t)1 << i), false};
    }
  }
  View::sendKeys(makeChanges);
  View::sendKeys(breakChanges);
}

std::ostream &operator<<(std::ostream &s, const View &v) {
  return s << &v;
}

std::ostream &operator<<(std::ostream &s, const View *v) {
  return s << v->name() << "@" << (void *) v;
}
