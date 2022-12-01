#include <cassert>
#include <sstream>
#include <string>
#include <vector>

typedef std::string mac;

struct arp_paket {
  mac macAddress;
  int displayoption;
  arp_paket(std::string macAddString, int displayOpt)
      : displayoption(displayOpt) {
    if (displayOpt > 3)
      assert(!"Maximale Displayoption ist 3");
    std::string segment;
    std::vector<int> seglist;
    std::stringstream ss(macAddString);
    while (std::getline(ss, segment, ':')) {
      seglist.push_back(strtol(segment.c_str(), NULL, 16));
      if (seglist.back() > 0xFF || seglist.back() < 0)
        assert(!"Wrong formating in MAC");
    }
    if (seglist.size() != 6) {
      assert(!"Length of MAC Address wrong");
    }
    macAddress = macAddString;
  }
};
