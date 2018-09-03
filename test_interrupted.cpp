#include "interrupted.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

void testInterrupted()
{
  std::cout << "\n============= testInterrupted\n";
  // create, copy, assign and destroy:
  std::interrupted i1;
  std::interrupted i2{i1};
  std::interrupted i3 = i1;
  i1 = i2;
  std::swap(i1,i2);
  //i1.swap(i2);

  // what():
  auto s1 = i1.what();
  auto s2 = i1.what();
  assert(std::strcmp(s1, s2) == 0);
  std::cout << "what(): " << s1 << '\n';

  std::cout << "**** all OK\n";
}

int main()
{
  testInterrupted();
}

