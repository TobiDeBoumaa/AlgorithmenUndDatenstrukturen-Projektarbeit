#pragma once

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

struct mac {
private:
  unsigned int a : 8;
  unsigned int b : 8;
  unsigned int c : 8;
  unsigned int d : 8;
  unsigned int e : 8;
  unsigned int f : 8;

public:
  std::string macString;
  mac(std::string sMac) {
    std::string segment;
    std::vector<int> seglist;
    std::stringstream ss(sMac);
    while (std::getline(ss, segment, ':')) {
      seglist.push_back(strtol(segment.c_str(), NULL, 16));
      if (seglist.back() > 0xFF || seglist.back() < 0) {
        assert(!"Wrong formating in MAC");
        return;
      }
    }
    if (seglist.size() != 6) {
      assert(!"Length of MAC Address wrong");
      return;
    }
    a = seglist.at(0);
    b = seglist.at(1);
    c = seglist.at(2);
    d = seglist.at(3);
    e = seglist.at(4);
    f = seglist.at(5);
    std::stringstream stream;
    stream << std::hex << a << ':' << b << ':' << c << ':' << d << ':' << e
           << ':' << f;
    macString = stream.str();
  }
  mac() { mac("00:00:00:00:00:00"); }
};

bool operator==(const mac &m1, const mac &m2);

struct arp_paket {
  mac macAddress;
  int displayoption = 0;
};
