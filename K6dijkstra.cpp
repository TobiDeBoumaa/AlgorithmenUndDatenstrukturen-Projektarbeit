// K6dijkstra.cpp
//

#include <iostream>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

/*
** Der Algorithmus von Dijkstra
*/

#define ANZAHL 12

#define BERLIN 0
#define BREMEN 1
#define DORTMUND 2
#define DRESDEN 3
#define DUESSELDORF 4
#define FRANKFURT 5
#define HAMBURG 6
#define HANNOVER 7
#define KOELN 8
#define LEIPZIG 9
#define MUENCHEN 10
#define STUTTGART 11

string location[ANZAHL] = {"Berlin",      "Bremen",    "Dortmund", "Dresden",
                           "Duesseldorf", "Frankfurt", "Hamburg",  "Hannover",
                           "Koeln",       "Leipzig",   "Muenchen", "Stuttgart"};

#define xxx 10000

unsigned int adistance[ANZAHL][ANZAHL] = {
    {0, xxx, xxx, 205, xxx, xxx, 284, 282, xxx, 179, xxx, xxx},
    {xxx, 0, 233, xxx, xxx, xxx, 119, 125, xxx, xxx, xxx, xxx},
    {xxx, 233, 0, xxx, 63, 264, xxx, 208, 83, xxx, xxx, xxx},
    {205, xxx, xxx, 0, xxx, xxx, xxx, xxx, xxx, 108, xxx, xxx},
    {xxx, xxx, 63, xxx, 0, xxx, xxx, xxx, 47, xxx, xxx, xxx},
    {xxx, xxx, 264, xxx, xxx, 0, xxx, 352, 189, 395, 400, 217},
    {284, 119, xxx, xxx, xxx, xxx, 0, 154, xxx, xxx, xxx, xxx},
    {282, 125, 208, xxx, xxx, 352, 154, 0, xxx, 256, xxx, xxx},
    {xxx, xxx, 83, xxx, 47, 189, xxx, xxx, 0, xxx, xxx, xxx},
    {179, xxx, xxx, 108, xxx, 395, xxx, 256, xxx, 0, 425, xxx},
    {xxx, xxx, xxx, xxx, xxx, 400, xxx, xxx, xxx, 425, 0, 220},
    {xxx, xxx, xxx, xxx, xxx, 217, xxx, xxx, xxx, xxx, 220, 0},
};

class CNodeinfo {
public:
  int distance;
  int predecessor;
  char done;
  CNodeinfo(){};
};

CNodeinfo n_info[ANZAHL];

void print_path(int i) {
  if (n_info[i].predecessor != -1) {
    print_path(n_info[i].predecessor);
    printf("->%s", location[i].c_str());
  } else
    printf("%s", location[i].c_str());
}

void print_all() {
  int i;

  for (i = 0; i < ANZAHL; i++) {
    print_path(i);
    printf(" (%d)\n", n_info[i].distance);
  }
}

void init(int start_n) {
  int i;

  for (i = 0; i < ANZAHL; i++) {
    n_info[i].done = 0;
    n_info[i].distance = adistance[start_n][i];
    n_info[i].predecessor = start_n;
  }
  n_info[start_n].done = 1;
  n_info[start_n].predecessor = -1;
}

int node_select() {
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

void dijkstra(int start_n) {
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

// void main()
// {
//     dijkstra( BERLIN);
//     print_all();
// 	char c; cin>>c;

// }
