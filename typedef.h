typedef int d4x4 __attribute__((vector_size(16)));
typedef int d4x8 __attribute__((vector_size(32)));
typedef float f4x4 __attribute__((vector_size(16)));
typedef double lf8x4 __attribute__((vector_size(32)));
typedef unsigned long long ull8x4 __attribute__((vector_size(32)));
typedef long long ll8x8 __attribute__((vector_size(64)));
typedef short h2x8 __attribute__((vector_size(16)));
typedef unsigned char uc1x8 __attribute__((vector_size(8)));

typedef union ll8x8_u {
  ll8x8 v;
  long long e[8];
} ll8x8_u;