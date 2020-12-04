#pragma once
#include <assert.h>

int assertSize() {
  assert(sizeof(char) == 1);
  assert(sizeof(short) == 2);
  assert(sizeof(int) == 4);
  assert(sizeof(long) == 8);
  assert(sizeof(long long) == 8);
  assert(sizeof(float) == 4);
  assert(sizeof(double) == 8);
  assert(sizeof(char*) == 8);
}

// int main() {
//   assertSize();
// }