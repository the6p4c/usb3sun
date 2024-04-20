#ifndef USB3SUN_HOSTID_H
#define USB3SUN_HOSTID_H

#include <cstddef>

#include "settings.h"
#include "view.h"

struct HostidView : View {
  bool isOpen = false;
  HostidV2::Value newHostid{};
  HostidV2::Value *hostidOut = nullptr;
  size_t cursorIndex = 0;

  const char *name() const override;
  void handlePaint() override;
  void handleKey(const UsbkChanges &) override;
  void open(HostidV2::Value *hostidInOut);
  void ok();
  void cancel();
  void left();
  void right();
  void type(unsigned char);
};

extern HostidView HOSTID_VIEW;

#endif
