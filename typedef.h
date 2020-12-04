typedef int d4x4 __attribute__((vector_size(16)));
typedef int d4x8 __attribute__((vector_size(32)));
typedef float f4x4 __attribute__((vector_size(16)));
typedef double lf8x4 __attribute__((vector_size(32)));
typedef unsigned long long ull8x4 __attribute__((vector_size(32)));
typedef unsigned long long ull8x8 __attribute__((vector_size(64)));
typedef short h2x8 __attribute__((vector_size(16)));
typedef unsigned char uc1x8 __attribute__((vector_size(8)));

// typedef union d4x8_u {
//   d4x8 v;
//   int e[8];
// } d4x8_u;
// typedef union lf8x4_u {
//   lf8x4 v;
//   double e[8];
// } lf8x4_u;
// typedef union ull8x8_u {
//   ull8x8 v;
//   unsigned long long e[8];
// } ull8x8_u;
// typedef union uc1x8_u {
//   uc1x8 v;
//   unsigned char e[8];
// } uc1x8_u;
// typedef union h2x8_u {
//   h2x8 v;
//   short e[8];
// } h2x8_u;