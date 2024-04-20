#ifndef USB3SUN_MENU_H
#define USB3SUN_MENU_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include "settings.h"
#include "view.h"

enum class MenuItem : size_t {
  GoBack,
  ForceClick,
  ClickDuration,
  MouseBaud,
  Hostid,
  ReprogramIdprom,
  WipeIdprom,
};

struct MenuView : View {
  bool isOpen = false;
  size_t selectedItem = 0u;
  size_t topItem = 0u;
  int16_t marqueeX = 0;
  unsigned marqueeTick = 0;

  const char *name() const override;
  void handlePaint() override;
  void handleKey(const UsbkChanges &) override;
  void open();
  void close();
  void closeWithoutConfirmSave();
  void sel(uint8_t usbkSelector);
};

struct WaitView : View {
  bool isOpen = false;
  const char *message = "";
  std::optional<HostidV2::Value> hostid{};

  const char *name() const override;
  void handlePaint() override;
  void handleKey(const UsbkChanges &) override;
  void open(const char *message, std::optional<HostidV2::Value> hostid);
  void close();
};

struct SaveSettingsView : View {
  bool isOpen = false;

  const char *name() const override;
  void handlePaint() override;
  void handleKey(const UsbkChanges &) override;
  void open();
  void close();
};

extern MenuView MENU_VIEW;
extern WaitView WAIT_VIEW;
extern SaveSettingsView SAVE_SETTINGS_VIEW;

#endif
