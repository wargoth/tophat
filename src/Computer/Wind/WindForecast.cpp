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
//#include <fstream>
//#include <sstream>
//#include <string>
//#include <regex>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "Units/System.hpp"


unsigned int
WindForecast::ReadLine(const std::string& line, Data *data)
{
  int num(0);
  std::istringstream strm(line);
  if (!(strm >> num)) {
    return 0;
  }
  if (4 <= num && num <= 9) {
    int pres, alt, temp, dewpt, wdir, wspd;
    strm >> pres >> alt >> temp >> dewpt >> wdir >> wspd;
    if (alt == 99999 || wdir == 99999 || wspd == 99999) {
      return 0;
    }
    data->pres = pres / 10.0;
    data->alt = alt;
    data->temp = temp / 10.0;
    data->dewpt = dewpt / 10.0;
    data->wspd = wspd;
    data->wdir = wdir;
    return 1;
  } else {
    return 0;
  }
}

void
WindForecast::Init()
{
  data.clear();

  // https://rucsoundings.noaa.gov/get_soundings.cgi?data_source=Op40&airport=36.908027,%20-121.313979&start=latest&format=csv

  std::ifstream strm("./src/Computer/Wind/s1.txt");
  std::string line;
  while (getline(strm, line)) {
    WindForecast::Data row;
    if(WindForecast::ReadLine(line, &row)) {
      std::cout << "out = " << (row.alt * 3.3) << " " << row.wdir << " " << row.wspd << std::endl;

      data.push_back(row);
    }
  }


}

WindForecast::Result
WindForecast::Update(const MoreData &basic, const DerivedInfo &derived)
{
  if (!basic.NavAltitudeAvailable()) {
    return WindForecast::Result(0);
  }

  WindForecast::Result result = WindForecast::Result(5);
  result.wind = SpeedVector(Angle::Degrees(fixed(90)), fixed(10));

  Data *previous = NULL;
  for (unsigned int i = 0; i < data.size(); i++) {
    Data *row = &data[i];
    if (Units::ToSysUnit(row->alt, Unit::METER) > basic.nav_altitude) {
      result.wind = WindForecast::LinearApprox(basic.nav_altitude, *previous, *row);
      break;
    } else {
      previous = row;
    }
  }

  return result;
}

SpeedVector
WindForecast::LinearApprox(fixed altitude, WindForecast::Data &prev, WindForecast::Data &next)
{
  return SpeedVector(Angle::Degrees(next.wdir), Units::ToSysUnit(next.wspd, Unit::KNOTS)) ;
}

/*
"""
https://rucsoundings.noaa.gov/raob_format.html
"""

import re

decimal = r'(-?\d+)'
space = r'\s+'
rec = re.compile(r'^' + (space + decimal) * 7)

with open('s1.txt', 'r') as f:
    print('ft', 'mb', 't/td', 'wdir/wspd')
    for line in f:
        match = rec.search(line)
        if not match:
            continue
        groups = match.groups()
        if int(groups[0]) not in range(4, 10):
            continue
        n, pres, height, temp, dewpt, wdir, wspd = map(float, groups)

        if 99999 in [height, wdir, wspd]:
            continue
        pres, temp, dewpt = map(lambda x: x / 10, [pres, temp, dewpt])
        print('%d\t%d\t%d/%d\t%d/%d' % (height * 3.28, pres, temp, dewpt, wdir, wspd))

*/
