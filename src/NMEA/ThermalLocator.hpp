/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_NMEA_THERMAL_LOCATOR_HPP
#define XCSOAR_NMEA_THERMAL_LOCATOR_HPP

#include "Engine/Navigation/GeoPoint.hpp"

/**
 * Structure to hold information on identified thermal sources on the ground
 */
struct THERMAL_SOURCE_INFO
{
  GeoPoint Location;
  fixed GroundHeight;
  fixed LiftRate;
  fixed Time;
};

/**
 * Structure for current thermal estimate from ThermalLocator
 * 
 */
struct THERMAL_LOCATOR_INFO
{
  static const unsigned MAX_THERMAL_SOURCES = 20;

  /** Location of thermal at aircraft altitude */
  GeoPoint ThermalEstimate_Location;
  /** Is thermal estimation valid? */
  bool ThermalEstimate_Valid;

  /** Position and data of the last thermal sources */
  THERMAL_SOURCE_INFO ThermalSources[MAX_THERMAL_SOURCES];
};

#endif
