#ifndef ASSERT_H
#define ASSERT_H
#include <assert.h>

int assertSize() {
  // this will pass if in `m64`
  assert(sizeof(char) == 1);
  assert(sizeof(short) == 2);
  assert(sizeof(int) == 4);
  assert(sizeof(long) == 8);  // maybe 8 in m64 , 4 in m32
  assert(sizeof(long long) == 8);
  assert(sizeof(float) == 4);
  assert(sizeof(double) == 8);
  assert(sizeof(char*) == 8);  // maybe 8 in m64 , 4 in m32
}

// int main() {
//   assertSize();
// }
#endif