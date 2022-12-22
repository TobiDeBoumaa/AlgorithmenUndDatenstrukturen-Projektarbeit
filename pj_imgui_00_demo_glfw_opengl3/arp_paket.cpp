#include "arp_paket.hpp"

bool operator==(const mac &m1, const mac &m2) {
  return m1.macString == m2.macString;
}