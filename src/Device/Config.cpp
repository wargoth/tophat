/*
Copyright_License {

  Top Hat Soaring Glide Computer - http://www.tophatsoaring.org/
  Copyright (C) 2000-2016 The Top Hat Soaring Project
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

#include "Config.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "Android/Nook.hpp"

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#include "Java/Global.hxx"
#endif

bool
DeviceConfig::IsAvailable() const
{
  if (!enabled)
    return false;

  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
    return true;

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
    return IsAndroid();

  case PortType::IOIOUART:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
  case PortType::IOIOVOLTAGE:
    return HasIOIOLib();

  case PortType::AUTO:
    return IsWindowsCE();

  case PortType::NOOK_NST_USB_HOST:
#ifdef ANDROID
    return IsNookSimpleTouch();
#else
    return false;
#endif

  case PortType::INTERNAL:
    return IsAndroid() || IsApple();

  case PortType::TCP_CLIENT:
    return !IsWindowsCE();

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    return true;

  case PortType::PTY:
#if defined(HAVE_POSIX) && !defined(ANDROID)
    return true;
#else
    return false;
#endif
  }

  /* unreachable */
  return false;
}

bool
DeviceConfig::ShouldReopenOnTimeout() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
#ifdef ANDROID
    return IsNookSimpleTouch();
#else
    return (IsWindowsCE() && !IsAltair());
#endif

  case PortType::AUTO:
    /* auto-reopen on Windows CE due to its quirks, but not Altair,
       because Altair ports are known to be "kind of sane" (no flaky
       Bluetooth drivers, because there's no Bluetooth) */
    return IsWindowsCE() && !IsAltair();

  case PortType::NOOK_NST_USB_HOST:
#ifdef ANDROID
    return IsNookSimpleTouch();
#else
    return false;
#endif

  case PortType::IOIOUART:
    return true;

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
  case PortType::IOIOVOLTAGE:
  case PortType::TCP_CLIENT:
    /* errors on these are detected automatically by the driver */
    return false;

  case PortType::INTERNAL:
    /* reopening the Android / Apple internal GPS doesn't help */
    return false;

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    /* this is a server, and if no data gets received, this can just
       mean that nobody connected to it, but reopening it periodically
       doesn't help */
    return false;

  case PortType::PTY:
    return false;
  }

  gcc_unreachable();
}

bool
DeviceConfig::MaybeBluetooth(PortType port_type, const TCHAR *path)
{
  /* note: RFCOMM_SERVER is not considered here because this
     function is used to check for the K6-Bt protocol, but the K6-Bt
     will never connect to XCSoar  */

  if (port_type == PortType::RFCOMM)
    return true;

#ifdef HAVE_POSIX
  if (port_type == PortType::SERIAL && _tcsstr(path, _T("/rfcomm")) != nullptr)
    return true;
#endif

#ifdef _WIN32_WCE
  /* on Windows CE, any serial port may be mapped to a Bluetooth
     driver */
  if (port_type == PortType::SERIAL)
    return true;
#endif

  return false;
}

bool
DeviceConfig::MaybeBluetooth() const
{
  /* note: RFCOMM_SERVER is not considered here because this
     function is used to check for the K6-Bt protocol, but the K6-Bt
     will never connect to XCSoar  */

  if (port_type == PortType::RFCOMM)
    return true;

#ifdef HAVE_POSIX
  if (port_type == PortType::SERIAL && path.Contains(_T("/rfcomm")))
    return true;
#endif

#ifdef _WIN32_WCE
  /* on Windows CE, any serial port may be mapped to a Bluetooth
     driver */
  if (port_type == PortType::SERIAL)
    return true;
#endif

  return false;
}

bool
DeviceConfig::BluetoothNameStartsWith(const char *prefix) const
{
#ifdef ANDROID
  if (port_type != PortType::RFCOMM)
    return false;

  const char *name =
    BluetoothHelper::GetNameFromAddress(Java::GetEnv(), bluetooth_mac.c_str());
  return name != nullptr && StringStartsWith(name, prefix);
#else
  return false;
#endif
}

void
DeviceConfig::Clear()
{
  port_type = PortType::DISABLED;
  baud_rate = 4800u;
  bulk_baud_rate = 0u;
  tcp_port = 4353u;
  i2c_bus = 2u;
  i2c_addr = 0;
  press_use = PressureUse::STATIC_ONLY;
  path.clear();
  bluetooth_mac.clear();
  driver_name.clear();
  enabled = true;
  sync_from_device = true;
  sync_to_device = true;
  play_alarms = false;
  k6bt = false;
#ifndef NDEBUG
  dump_port = false;
#endif
}

const TCHAR *
DeviceConfig::GetPortName(TCHAR *buffer, size_t max_size) const
{
  switch (port_type) {
  case PortType::DISABLED:
    return _("Disabled");

  case PortType::SERIAL:
    return path.c_str();

  case PortType::RFCOMM: {
    const TCHAR *name = bluetooth_mac.c_str();
#ifdef ANDROID
    const char *name2 =
      BluetoothHelper::GetNameFromAddress(Java::GetEnv(), name);
    if (name2 != nullptr)
      name = name2;
#endif

    StringFormat(buffer, max_size, _T("Bluetooth %s"), name);
    return buffer;
    }

  case PortType::RFCOMM_SERVER:
    return _("Bluetooth server");

  case PortType::IOIOUART:
    StringFormat(buffer, max_size, _T("IOIO UART %d"), ioio_uart_id);
    return buffer;

  case PortType::DROIDSOAR_V2:
    return _T("DroidSoar V2");

  case PortType::NUNCHUCK:
    return _T("Nunchuck");

  case PortType::I2CPRESSURESENSOR:
    return _T("IOIO i2c pressure sensor");

  case PortType::IOIOVOLTAGE:
    return _T("IOIO voltage sensor");

  case PortType::AUTO:
    return _("GPS Intermediate Driver");

  case PortType::NOOK_NST_USB_HOST:
    return _(GetNookUsbHostDriverName());

  case PortType::INTERNAL:
    return _("Built-in GPS & sensors");

  case PortType::TCP_CLIENT:
    StringFormat(buffer, max_size, _T("TCP client %s:%u"),
                 ip_address.c_str(), tcp_port);
    return buffer;

  case PortType::TCP_LISTENER:
    StringFormat(buffer, max_size, _T("TCP port %d"), tcp_port);
    return buffer;

  case PortType::UDP_LISTENER:
    StringFormat(buffer, max_size, _T("UDP port %d"), tcp_port);
    return buffer;

  case PortType::PTY:
    StringFormat(buffer, max_size, _T("Pseudo-terminal %s"), path.c_str());
    return buffer;
  }

  gcc_unreachable();
}

bool
DeviceConfig::IsSerial()
{
#ifdef ANDROID
  if (IsNookSimpleTouch())
    return port_type == PortType::SERIAL ||
        port_type == PortType::NOOK_NST_USB_HOST;
  else
#endif
    return port_type == PortType::SERIAL;
}
