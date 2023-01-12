// K6dijkstra.cpp
//

#include "K6dijkstra.hpp"
#include <iostream>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

/*
** Der Algorithmus von Dijkstra
*/

void dijkstra::print_path(std::stringstream& ss, int i) {
  if (n_info[i].predecessor != -1)
    print_path(ss,n_info[i].predecessor);
  ss << knoten[i].name << '\n';
  printf("↳ %s\n", knoten[i].name.c_str());
}
void dijkstra::get_path(int i, vector<Knoten> &knotenListe) {
  if (n_info[i].predecessor != -1)
    get_path(n_info[i].predecessor, knotenListe);
  knotenListe.push_back(knoten[i]);
}
void dijkstra::print_path_mac(std::stringstream& ss, int i) {
  if (n_info[i].predecessor != -1)
    print_path_mac(ss,n_info[i].predecessor);
  ss << knoten[i].name << " MAC:(" << knoten[i].macAddress.macString<<")\n";
  printf("↳ %s MAC(%s)\n", knoten[i].name.c_str(),
         knoten[i].macAddress.macString.c_str());
}

void dijkstra::print_path(std::stringstream& ss, mac eingabeAddresse, int displayoption) {
  int i = 0;
  for (i = 0; i < ANZAHL; i++) {
    if (knoten[i].macAddress == eingabeAddresse)
      break;
  }
  if (displayoption == 1)
    print_path(ss,i);
  if (displayoption == 2)
    print_path_mac(ss,i);
}

Knoten dijkstra::find_path(int i) {
  if (n_info[i].predecessor != -1)
    return find_path(n_info[i].predecessor);
  else
    return knoten[i];
}

void dijkstra::print_all(std::stringstream& ss) {
  int i;
  for (i = 0; i < ANZAHL; i++) {
    vector<Knoten> liste;
    get_path(i, liste);
    ss << liste.back().name << '\t' << liste.back().macAddress.macString << '\t';
    if (liste.size() > 1) {
        for (vector<Knoten>::reverse_iterator it = liste.rbegin();
            it != liste.rend() - 1; it++)
            ss << it->name << "->";
        ss << liste.front().name;
    }
    else
        ss << '-';
      ss << '\n';
  }
}

/**
 * Holt alle Knoten
 */
std::vector<Knoten> dijkstra::knoten_broadcast() {
  std::vector<Knoten> alleKnoten;
  for (int i = 0; i < ANZAHL; i++)
    alleKnoten.push_back(find_path(i));
  return alleKnoten;
}

bool dijkstra::knoten_vorhanden(mac suchAddresse) {
  printf("Suche MAC: %s\n", suchAddresse.macString.c_str());
  for (auto &KnotenListenelement : knoten) {
    if (KnotenListenelement.macAddress == suchAddresse)
      return true;
  }
  return false;
}

Knoten dijkstra::macToKnoten(mac eingabeAddresse) {
  int i = 0;
  for (i = 0; i < ANZAHL; i++) {
      if (knoten[i].macAddress == eingabeAddresse)
      break;
  }
  printf("Es ist Knoten:%i\n",i);
  return knoten[i];
}

void dijkstra::init(int start_n) {
  int i;

  for (i = 0; i < ANZAHL; i++) {
    n_info[i].done = 0;
    n_info[i].distance = adistance[start_n][i];
    n_info[i].predecessor = start_n;
  }
  n_info[start_n].done = 1;
  n_info[start_n].predecessor = -1;
}

int dijkstra::node_select() {
  int i, minpos;
  int min;

  min = xxx;
  minpos = -1;
  for (i = 0; i < ANZAHL; i++) {
    if (n_info[i].distance < min && !n_info[i].done) {
      min = n_info[i].distance;
      minpos = i;
    }
  }
  return minpos;
}

dijkstra::dijkstra(int start_n) {
  int i, node, k;
  int d;

  init(start_n);
  for (i = 0; i < ANZAHL - 2; i++) {
    node = node_select();
    n_info[node].done = 1;
    for (k = 0; k < ANZAHL; k++) {
      if (!(n_info[k].done == 1)) {
        d = n_info[node].distance + adistance[node][k];
        if (d < n_info[k].distance) {
          n_info[k].distance = d;
          n_info[k].predecessor = node;
        }
      }
    }
  }
}
