/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/DialogSettings.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Fonts.hpp"
#include "UIGlobals.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ActionListener.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Canvas.hpp"
#include "NMEA/Checksum.hpp"
#include "Util/StaticString.hpp"
#include "Screen/TerminalWindow.hpp"
#include "Look/TerminalLook.hpp"
#include "Event/DelayedNotify.hpp"
#include "Thread/Mutex.hpp"
#include "Util/Macros.hpp"
#include "Util/FifoBuffer.hpp"
#include "IO/DataHandler.hpp"

#include <algorithm>
#include <stdio.h>
#include <unistd.h>

enum Buttons {
  LAUNCH_XCSOAR = 100,
  LAUNCH_NICKEL,
  WIFI,
  PAUSE,
  CLEAR,
  REBOOT,
  POWEROFF
};

/**
 * A bridge between DataHandler and TerminalWindow: copy all data
 * received from the Log to the TerminalWindow.
 */
class LogTerminalBridge
  : public DataHandler,
    private DelayedNotify {
  TerminalWindow &terminal;
  Mutex mutex;
  FifoBuffer<char, 1024> buffer;

public:
  LogTerminalBridge(TerminalWindow &_terminal)
    :DelayedNotify(100), terminal(_terminal) {}

  virtual void DataReceived(const void *data, size_t length) {
    mutex.Lock();
    auto range = buffer.Write();
    if (range.length < length)
      length = range.length;
    memcpy(range.data, data, length);
    buffer.Append(length);
    mutex.Unlock();
    SendNotification();
  }

private:
  virtual void OnNotification() {
    while (true) {
      char data[64];
      size_t length;

      {
        ScopeLock protect(mutex);
        auto range = buffer.Read();
        if (range.IsEmpty())
          break;

        length = std::min(ARRAY_SIZE(data), size_t(range.length));
        memcpy(data, range.data, length);
        buffer.Consume(length);
      }

      terminal.Write(data, length);
    }
  }
};

class LogMonitorGlue : public ActionListener {
  TerminalWindow terminal;
  LogTerminalBridge bridge;

  WndButton *pause_button;
  bool paused;

  WndButton *wifi_button;
  bool wifi;
  WndForm &dialog;

public:
  int last_action;

  LogMonitorGlue(const TerminalLook &look, WndForm &_dialog)
    :terminal(look), bridge(terminal), paused(false), wifi(false), dialog(_dialog),last_action(0) {}

  ~LogMonitorGlue() {
    // device.SetMonitor(nullptr);
  }

  void CreateButtons(ButtonPanel &buttons);

  void CreateTerminal(ContainerWindow &parent, const PixelRect &rc) {
    terminal.Create(parent, rc);
    // device.SetMonitor(&bridge);
  }

  void Clear() {
    terminal.Clear();
  }
  void Reboot() {
#ifdef KOBO
    system("reboot");
#endif
  }
  void Poweroff() {
#ifdef KOBO
    system("poweroff");
#endif
  }

  void TogglePause();
  void ToggleWifi();

  void LaunchXCSoar() {
    dialog.SetModalResult(mrOK);
  }

  void LaunchNickel() {
    dialog.SetModalResult(mrOK);
  }

  virtual void OnAction(int id) override {
    last_action = id;
    switch (id) {
    case LAUNCH_XCSOAR:
      LaunchXCSoar();
      break;
    case LAUNCH_NICKEL:
      LaunchNickel();
      break;
    case CLEAR:
      Clear();
      break;
    case REBOOT:
      Reboot();
      break;
    case POWEROFF:
      Poweroff();
      break;
    case PAUSE:
      TogglePause();
      break;
    case WIFI:
      ToggleWifi();
      break;
    default:
      break;
    }
  }
};

void
LogMonitorGlue::CreateButtons(ButtonPanel &buttons)
{
  buttons.Add(("XCSoar"), *this, LAUNCH_XCSOAR);
  buttons.Add(("Nickel"), *this, LAUNCH_NICKEL);
  buttons.Add(("Clear"), *this, CLEAR);
  pause_button = buttons.Add(("Pause"), *this, PAUSE);
  wifi_button = buttons.Add(("Wifi ON"), *this, WIFI);
  buttons.Add(("Reboot"), *this, REBOOT);
  buttons.Add(("Poweroff"), *this, POWEROFF);
}

void
LogMonitorGlue::TogglePause()
{
  paused = !paused;

  if (paused) {
    pause_button->SetCaption(("Resume"));
    //    device.SetMonitor(nullptr);
  } else {
    pause_button->SetCaption(("Pause"));
    //    device.SetMonitor(&bridge);
  }
}

void
LogMonitorGlue::ToggleWifi()
{
  wifi = !wifi;

  if (wifi) {
    wifi_button->SetCaption(("Wifi OFF"));
#ifdef KOBO
    system("/mnt/onboard/XCSoar/wifiup.sh");
#endif
    //    device.SetMonitor(nullptr);
  } else {
    wifi_button->SetCaption(("Wifi ON"));
    //    device.SetMonitor(&bridge);
#ifdef KOBO
    system("/mnt/onboard/XCSoar/wifidown.sh");
#endif
  }
}

static DialogLook *dialog_look;
static SingleWindow main_window;
static DialogSettings dialog_settings;

static int action = 0;

static void
Main()
{
  WndForm dialog(*dialog_look);
  dialog.Create(main_window, _T("Kobo XCSoar Launcher"));
  ContainerWindow &client_area = dialog.GetClientAreaWindow();

  TerminalLook terminal_look;
  terminal_look.Initialise(normal_font);

  LogMonitorGlue glue(terminal_look, dialog);

  ButtonPanel buttons(client_area, *dialog_look);

  glue.CreateButtons(buttons);
  glue.CreateTerminal(client_area, buttons.UpdateLayout());

#ifdef KOBO
  // must be down at start
  system("/mnt/onboard/XCSoar/wifidown.sh");
#endif

  dialog.ShowModal();

  action = glue.last_action;

  // /proc/kmsg
}


int main(int argc, char **argv)
{
  {
    ScreenGlobalInit screen_init;
    Layout::Initialize({600, 800});
    InitialiseFonts();

    main_window.Create(_T("Test"), {600, 800});
    main_window.Show();

    dialog_settings.SetDefaults();

    dialog_look = new DialogLook();
    dialog_look->Initialise(bold_font, normal_font, small_font,
                            bold_font, bold_font, bold_font);

    Main();

    main_window.Destroy();

    delete dialog_look;
    DeinitialiseFonts();
  }

#ifdef KOBO
  if (action == LAUNCH_NICKEL) {
    printf("launch nickel\n");
    fflush(stdout);

    const char cmd[] = "/mnt/onboard/XCSoar/restartnickel.sh";
    execl(cmd, cmd, nullptr);
  } else if (action == LAUNCH_XCSOAR) {
    printf("launch xcsoar\n");
    fflush(stdout);

    const char cmd[] = "/mnt/onboard/XCSoar/xcsoar";
    execl(cmd, cmd, nullptr);
  } else {
    fflush(stdout);
  }
#endif
  return 0;
}