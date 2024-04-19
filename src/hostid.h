#ifndef USB3SUN_HOSTID_H
#define USB3SUN_HOSTID_H

#include <cstddef>

#include "settings.h"
#include "view.h"

struct HostidView : View {
  bool isOpen = false;
  Hostid newHostid{};
  Hostid *hostidOut = nullptr;
  size_t cursorIndex = 0;

  void handlePaint() override;
  void handleKey(const UsbkChanges &) override;
  void open(Hostid *hostidInOut);
  void ok();
  void cancel();
  void left();
  void right();
  void type(unsigned char);
};

extern HostidView HOSTID_VIEW;

#endif
