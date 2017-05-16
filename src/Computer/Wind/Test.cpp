#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

struct Data {
  float pres;
  int alt;
  float temp;
  float dewpt;
  int wdir;
  int wspd;
};

unsigned int
read_line(const std::string& line, Data *data)
{
  int num(0);
  string str;
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

int
main()
{
  std::ifstream strm("./src/Computer/Wind/s1.txt");
  std::string line;
  int count = 0;
  while (getline(strm, line)) {
    Data data;
    if(read_line(line, &data)) {
      std::cout << "out = " << data.pres << " " << data.alt << " " << data.temp << " "
          << data.dewpt << " " << data.wdir << " " << data.wspd << std::endl;

    }
  }
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

//int
//main()
//{
//  string line;
//
//  // our 2 dimensional table
//  vector<vector<int>> table;
//
//  istringstream strm;
//  int num;
//  ifstream ifs("./src/Computer/Wind/s1.txt");
//  while (getline(ifs, line)) {
//    vector<int> vInt;
//    istringstream strm(line);
//    while (strm >> num)
//      vInt.push_back(num);
//    table.push_back(vInt);
//  }
//}
