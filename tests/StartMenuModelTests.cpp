#include "StartMenuModel.h"
#include <iostream>

int main() {
  if (!noya::ModelSelfTest()) {
    std::cerr << "Start menu search model failed.\n";
    return 1;
  }
  std::cout << "Start menu model tests passed.\n";
  return 0;
}
