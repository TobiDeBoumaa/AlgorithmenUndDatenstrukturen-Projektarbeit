#include "lexikalischeAnalyse.hpp"
#include <iostream>

using namespace std;

int main() {
  CParser obj;
  obj.InitParse(stdin, stderr, stdout);
  // obj.pr_tokentable();
  auto test = obj.yyparse();
  if (1 == 0)
    ;
}
