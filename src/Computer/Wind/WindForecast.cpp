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

#include "WindForecast.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/MoreData.hpp"
#include "LogFile.hpp"
#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/ToBuffer.hpp"
#include "Net/HTTP/Request.hpp"
#include "Util/StaticString.hxx"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Units/System.hpp"

bool
WindForecast::ReadLine(const std::string& line, Data *data)
{
  int num(0);
  std::istringstream strm(line);
  if (!(strm >> num)) {
    return false;
  }
  if (4 <= num && num <= 9) {
    int pres, alt, temp, dewpt, wdir, wspd;
    strm >> pres >> alt >> temp >> dewpt >> wdir >> wspd;
    if (alt == 99999 || wdir == 99999 || wspd == 99999) {
      return false;
    }
    data->pres = pres / 10.0;
    data->alt = alt;
    data->temp = temp / 10.0;
    data->dewpt = dewpt / 10.0;
    data->wspd = wspd;
    data->wdir = wdir;
    return true;
  } else {
    return false;
  }
}

void
WindForecast::Tick()
{
  mutex.Unlock();

  // Build file url
  NarrowString<1024> url;
  url.Format("https://rucsoundings.noaa.gov/get_soundings.cgi?data_source=Op40&airport=%f,%f&start=latest",
             last_position.latitude.Degrees(), last_position.longitude.Degrees());

  std::cout << "Calling " <<  url << std::endl;

// Open download session
  Net::Session session;
  if (session.Error())
    return;

  char buffer[1024 * 10];
  Net::Request request(session, url, 3000);
  if (!request.Send(10000))
    return;

  ssize_t size = request.Read(buffer, sizeof(buffer), 10000);
  if (size <= 0)
    return;

  buffer[size] = 0;

  std::stringstream strm(buffer);
  std::string line;
  while (getline(strm, line)) {
    WindForecast::Data row;
    if (WindForecast::ReadLine(line, &row)) {
      std::cout << "out = " << (row.alt * 3.3) << " " << row.wdir << " "
          << row.wspd << std::endl;

      data.push_back(row);
    }
  }

  mutex.Lock();
}

void
WindForecast::Init()
{
  data.clear();
}

WindForecast::Result
WindForecast::Update(const MoreData &basic, const DerivedInfo &derived)
{
  if (!basic.NavAltitudeAvailable() || !basic.location_available) {
    return WindForecast::Result(0);
  }

  ScopeLock protect(mutex);

  if (IsBusy())
    /* still running, skip this submission */
    return WindForecast::Result(0);

  last_position = basic.location;

  if (data.empty()) {
    Trigger();
    return WindForecast::Result(0);
  }

  WindForecast::Result result = WindForecast::Result(0);

  Data *previous = NULL;
  for (unsigned int i = 0; i < data.size(); i++) {
    Data *row = &data[i];
    if (Units::ToSysUnit(row->alt, Unit::METER) > basic.nav_altitude) {
      result.wind = WindForecast::LinearApprox(basic.nav_altitude, *previous,
                                               *row);
      result.quality = 1;
      break;
    } else {
      previous = row;
    }
  }

  return result;
}

SpeedVector
WindForecast::LinearApprox(fixed altitude, WindForecast::Data &prev,
                           WindForecast::Data &next)
{
  return SpeedVector(Angle::Degrees(next.wdir),
                     Units::ToSysUnit(next.wspd, Unit::KNOTS));
}
