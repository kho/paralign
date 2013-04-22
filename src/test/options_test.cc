#include <iostream>
#include "options.h"

int main() {
  paralign::Options opts = paralign::Options::FromEnv();
  std::cout << opts;
  return 0;
}
