#include "arp_paket.hpp"
#include <array>
#include <string>

#define ANZAHL 15

#define STATIONA 0
#define STATIONB 1
#define STATIONC 2
#define STATIOND 3
#define STATIONE 4
#define STATIONF 5
#define STATIONG 6
#define STATIONH 7
#define STATIONI 8
#define STATIONJ 9
#define STATIONK 10
#define STATIONL 11
#define STATIONM 12
#define STATIONN 13
#define STATIONO 14

#define xxx 10000

struct Knoten {
  std::string name;
  mac macAddress;
  int maxSupportedDisplayOption = 0;
  Knoten(std::string name, mac macAddress, int maxSupportedDisplayOption) {
    this->name = name;
    this->macAddress = macAddress;
    this->maxSupportedDisplayOption = maxSupportedDisplayOption;
  }
};

class dijkstra {
private:
  void init(int start_n);
  int node_select();
  Knoten find_path(int i);
  void print_path(std::stringstream& ss, int i);
  void print_path_mac(std::stringstream& ss, int i);
  std::vector<Knoten> knoten_broadcast();
  void get_path(int i, std::vector<Knoten> &knotenListe);

public:
  dijkstra(int start_n);
  void print_path(std::stringstream& ss, mac eingabeAddresse, int displayoption = 0);
  void print_all(std::stringstream& ss);
  bool knoten_vorhanden(mac suchAddresse);
  Knoten macToKnoten(mac eingabeAddresse);
  Knoten knoten[ANZAHL] = {
      {"StationA", mac(std::string("28:4d:8f:d9:42:c2")), 3},
      {"StationB", mac(std::string("d6:68:a8:f4:bb:6b")), 3},
      {"StationC", mac(std::string("6f:bf:92:d2:aa:a6")), 1},
      {"StationD", mac(std::string("9d:67:42:02:37:63")), 3},
      {"StationE", mac(std::string("87:0b:d3:fb:f8:f2")), 3},
      {"StationF", mac(std::string("f6:ce:61:12:03:c6")), 1},
      {"StationG", mac(std::string("85:1f:13:37:c9:07")), 1},
      {"StationH", mac(std::string("6e:45:d5:fa:fb:4a")), 3},
      {"StationI", mac(std::string("64:59:d7:d7:66:d5")), 1},
      {"StationJ", mac(std::string("e5:a6:d7:8b:cc:a9")), 3},
      {"StationK", mac(std::string("29:a1:6d:bb:8b:6b")), 3},
      {"StationL", mac(std::string("f5:70:05:20:32:8f")), 3}
      #if ANZAHL==15
      ,{"StationM", mac(std::string("5a:e5:0a:06:85:82")), 3}
      ,{"StationN", mac(std::string("73:1e:92:5f:62:3e")), 3}
      ,{"StationO", mac(std::string("66:d6:86:1e:27:6b")), 3}
      #endif
  };

const unsigned int adistance[ANZAHL][ANZAHL] = {
#if ANZAHL==15
{  0, xxx, xxx, 205, xxx, xxx, 284, 282, xxx, 179, xxx, xxx, xxx, xxx, xxx},
{xxx,   0, 233, xxx, xxx, xxx, 119, 125, xxx, xxx, xxx, xxx, 161, xxx, xxx},
{xxx, 233,   0, xxx,  63, 264, xxx, 208,  83, xxx, xxx, xxx, xxx, xxx, xxx},
{205, xxx, xxx,   0, xxx, xxx, xxx, xxx, xxx, 108, xxx, xxx, xxx,  92, xxx},
{xxx, xxx,  63, xxx,   0, xxx, xxx, xxx,  47, xxx, xxx, xxx, 187, xxx, xxx},
{xxx, xxx, 264, xxx, xxx,   0, xxx, 352, 189, 395, 400, 217, xxx, xxx, 365},
{284, 119, xxx, xxx, xxx, xxx,   0, 154, xxx, xxx, xxx, xxx, xxx, xxx, xxx},
{282, 125, 208, xxx, xxx, 352, 154,   0, xxx, 256, xxx, xxx, xxx, xxx, xxx},
{xxx, xxx,  83, xxx,  47, 189, xxx, xxx,   0, xxx, xxx, xxx, xxx, xxx, 609},
{179, xxx, xxx, 108, xxx, 395, xxx, 256, xxx,   0, 425, xxx, xxx, xxx, xxx},
{xxx, xxx, xxx, xxx, xxx, 400, xxx, xxx, xxx, 425,   0, 220, xxx, 314, xxx},
{xxx, xxx, xxx, xxx, xxx, 217, xxx, xxx, xxx, xxx, 220,   0, xxx, xxx, 147},
{xxx, 161, xxx, xxx, 187, xxx, xxx, xxx, xxx, xxx, xxx, xxx,   0, xxx, xxx},
{xxx, xxx, xxx,  92, xxx, xxx, xxx, xxx, xxx, xxx, 314, xxx, xxx,   0, xxx},
{xxx, xxx, xxx, xxx, xxx, 365, xxx, xxx, 609, xxx, xxx, 147, xxx, xxx,   0},
#elif ANZAHL == 12
{   0, xxx, xxx, 205, xxx, xxx, 284, 282, xxx, 179, xxx, xxx },
{ xxx,   0, 233, xxx, xxx, xxx, 119, 125, xxx, xxx, xxx, xxx },
{ xxx, 233,   0, xxx,  63, 264, xxx, 208,  83, xxx, xxx, xxx },
{ 205, xxx, xxx,   0, xxx, xxx, xxx, xxx, xxx, 108, xxx, xxx },
{ xxx, xxx,  63, xxx,   0, xxx, xxx, xxx,  47, xxx, xxx, xxx },
{ xxx, xxx, 264, xxx, xxx,   0, xxx, 352, 189, 395, 400, 217 },
{ 284, 119, xxx, xxx, xxx, xxx,   0, 154, xxx, xxx, xxx, xxx },
{ 282, 125, 208, xxx, xxx, 352, 154,   0, xxx, 256, xxx, xxx },
{ xxx, xxx,  83, xxx,  47, 189, xxx, xxx,   0, xxx, xxx, xxx },
{ 179, xxx, xxx, 108, xxx, 395, xxx, 256, xxx,   0, 425, xxx },
{ xxx, xxx, xxx, xxx, xxx, 400, xxx, xxx, xxx, 425,   0, 220 },
{ xxx, xxx, xxx, xxx, xxx, 217, xxx, xxx, xxx, xxx, 220,   0 },
#endif // ANZAHL==15
};

  class CNodeinfo {
  public:
    int distance;
    int predecessor;
    char done;
    CNodeinfo(){};
  };
  CNodeinfo n_info[ANZAHL];
};
