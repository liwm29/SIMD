# SIMD
## 前置知识
- 代码SIMD的三个层次:
  - 汇编
  - intrisics
  - 自动向量化 -O3
- 一般的,我们着眼在intrisics上,或者添加编译指令辅助编译器的自动向量化优化(-O3)
- intrisics: 指builtin function
- `immintrin.h`
  - 这个头文件指 intel 向量化拓展,会自动引用相关的intrisics,比如AVX,SSE
  - [示例](https://www.cs.uaf.edu/courses/cs441/notes/avx/)
  - [一个不错的教程](https://users.ece.cmu.edu/~franzf/teaching/slides-18-645-simd.pdf)
  - [Interl官方文档](https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2)
- gcc vector extension
  - 编译器也提供了SSE向量化拓展,不需要引用头文件
  - [文档/示例](https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html)
- 向量化只提供对基础存储数据类型的支持,不包括指针(即无法对一个向量解引用)
## 内存对齐
AVX instructions expect input operands to be aligned on a 32-byte boundary.  
  - AVX技术提供256字节的向量拓展
SSE 要求16字节对齐  
  - SSE技术提供128字节的向量拓展
- new/malloc 默认内存对齐
  - 32位机器, new/malloc 返回8字节对齐内存 为什么呢?栈分配也有这个问题吗
  - 64位机器, new/malloc 返回16字节对齐内存 
  - [基本可以理解成,按本机器中最大基本数据类型对齐即可](https://stackoverflow.com/questions/59098246/why-is-dynamically-allocated-memory-always-16-bytes-aligned)
- 使用`_mm_malloc(size_in_bytes,32)` 申请32字节对齐的内存

## gcc 向量拓展
```c
// 用 __attribute__((aligned(16))) 来内存对齐
typedef int d4x4 __attribute__((vector_size(16)));
typedef int d4x8 __attribute__((vector_size(32)));
typedef float f4x4 __attribute__((vector_size(16)));
typedef double lf8x4 __attribute__((vector_size(32)));
typedef unsigned long long ull8x4 __attribute__((vector_size(32)));
typedef unsigned long long ull8x8 __attribute__((vector_size(64)));
typedef short h2x8 __attribute__((vector_size(16)));
typedef unsigned char uc1x8 __attribute__((vector_size(8)));
```
如果想访问各个分量,可以直接(float)a[0]访问不,也可以定义成union [ref](https://stackoverflow.com/questions/1771945/c-how-to-access-elements-of-vector-using-gcc-sse-vector-extension)
```c
// 这里v4sf指 f4x4:4个4字节的float;v4sf是gcc 向量拓展的命名风格,见[文档/示例](https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html)
// 事实上,f4x4也不算是标准的命名风格,但足够清晰
union Vec4 {
    v4sf v;
    float e[4];
};

Vec4 vec;
vec.v = (v4sf){0.1f,0.2f,0.3f,0.4f};
printf("%f %f %f %f\n", vec.e[0], vec.e[1], vec.e[2], vec.e[3]);
```

## Intel intrisics 向量拓展(不是icc,仍用gcc编译)
建议使用此向量拓展,文档清晰  
[Interl官方文档](https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2)
- 