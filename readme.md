# 向量化实验报告
18341018 李伟铭
<!-- TOC -->

- [向量化实验报告](#向量化实验报告)
  - [SIMD简单介绍](#simd简单介绍)
  - [优化工具选择](#优化工具选择)
  - [gcc 向量拓展](#gcc-向量拓展)
  - [Intel intrisics 向量拓展](#intel-intrisics-向量拓展)
  - [使用向量变量(向量拓展)面临的问题](#使用向量变量向量拓展面临的问题)
    - [内存对齐](#内存对齐)
  - [OpenMP 自动向量化](#openmp-自动向量化)
  - [Parallel & Unroll & SIMD](#parallel--unroll--simd)
  - [Example](#example)
  - [Img convert](#img-convert)
  - [Result](#result)
    - [对照组0, 普通编译](#对照组0-普通编译)
    - [对照组1, O3优化](#对照组1-o3优化)
    - [实验组, OpenMP优化](#实验组-openmp优化)
  - [Comment](#comment)

<!-- /TOC -->
## SIMD简单介绍
- 代码SIMD的三个层次:
  - 汇编
  - intrisics
  - 自动向量化 -O3
- 一般的,我们着眼在intrisics上,或者在添加编译指令辅助编译器的自动向量化优化(-O3)上
- intrisics: 指 builtin function
  - `immintrin.h`
    - 这个头文件指 intel 向量化拓展,会自动引用相关的intrisics,比如AVX,SSE
    - [示例](https://www.cs.uaf.edu/courses/cs441/notes/avx/)
    - [一个不错的教程](https://users.ece.cmu.edu/~franzf/teaching/slides-18-645-simd.pdf)
    - [Interl官方文档](https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2)
  - gcc vector extension
    - gcc编译器也提供了SSE向量化拓展,不需要引用头文件
    - [文档/示例](https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html)
- 向量化只提供对基础存储数据类型的支持,不包括指针(即无法对一个向量解引用)
  - 这个很关键,对多个指针的操作是无法向量化的
## 优化工具选择
本次选择了openMP这个工具来对这份代码进行优化

## gcc 向量拓展
我尝试使用这个优化程序,但在编码过程中感到实在太繁琐了,后改用openMP
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
如果想访问各个分量, [建议定义成union](https://stackoverflow.com/questions/1771945/c-how-to-access-elements-of-vector-using-gcc-sse-vector-extension)
```c
// 这里v4sf指 f4x4:4个4字节的float;
// 事实上,f4x4不算是标准的命名风格,但足够清晰
union Vec4 {
    v4sf v;
    float e[4];
};

Vec4 vec;
vec.v = (v4sf){0.1f,0.2f,0.3f,0.4f};
printf("%f %f %f %f\n", vec.e[0], vec.e[1], vec.e[2], vec.e[3]);
```
## Intel intrisics 向量拓展
引用immintrin.h, 仍用gcc编译(而不是icc)
- [Interl官方API文档](https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2)
## 使用向量变量(向量拓展)面临的问题
### 内存对齐
AVX instructions expect input operands to be aligned on a 32-byte boundary.  
  - AVX技术提供256字节的向量拓展,要求32字节对齐  
  - SSE技术提供128字节的向量拓展,要求16字节对齐
- new/malloc 默认内存对齐
  - 32位机器, new/malloc 返回8字节对齐内存
  - 64位机器, new/malloc 返回16字节对齐内存 
    - [基本可以理解成,new/malloc返回的地址按本机器中最大基本数据类型大小对齐](https://stackoverflow.com/questions/59098246/why-is-dynamically-allocated-memory-always-16-bytes-aligned)
- 使用`_mm_malloc(size_in_bytes,32)` 申请32字节对齐的内存

## OpenMP 自动向量化
在for循环前添加合适的 `#pragma` 指令,并适度修改for循环代码,以使得循环能被正确向量化

## Parallel & Unroll & SIMD
这是三个典型的 `#pragma` 指令
```c
#pragma omp parallel for
#pragma unroll
#pragma omp simd
```
- Parallel for 
  - 用于将for循环用多进程的方法并行化
- Unroll
  - 用于将for循环展开,预处理成多条串行代码,方便指令级并行
- SIMD
  - 用于将for循环向量化,一次计算多个数据
## Example
优化后代码:
```c
#pragma omp simd
for (int i = 0; i < rows * cols; i++) {
  edge[i] = nms[i] == POSSIBLE_EDGE ? POSSIBLE_EDGE : NOEDGE;
}
```
原代码(hysteresis.c#L64):
```c
for (r = 0, pos = 0; r < rows; r++) {
  for (c = 0; c < cols; c++, pos++) {
      if (nms[pos] == POSSIBLE_EDGE)
          edge[pos] = POSSIBLE_EDGE;
      else
          edge[pos] = NOEDGE;
  }
}
```
## Img convert
使用 `ffmpeg` 工具可以方便的帮助我们转换图片格式
```c
ffmpeg -i elephant.pgm elephant_converted.jpg
```
## Result
### 对照组0, 普通编译
首先编译原代码
```bash
gcc -o canny_edge canny_edge.c hysteresis.c pgm_io.c -lm
```
经过多次运行,取平均值为: `Time elapsed:47465 us`
### 对照组1, O3优化
首先编译原代码
```
gcc -o canny_edge canny_edge.c hysteresis.c pgm_io.c -lm -fopt-info -O3
```
经过多次运行,取平均值为: `Time elapsed:18356 us`
### 实验组, OpenMP优化
首先编译原代码
```
gcc -o canny_edge canny_edge.c hysteresis.c pgm_io.c -lm -fopenmp -fopt-info -O3
```
经过多次运行,取平均值为: `Time elapsed: 17821 us`
## Comment
经过我们openMP优化后,程序运行的事件并没有显著提高,如果略微了解O3优化,便会知道这也情有可原.O3优化已经帮助我们优化了大量的循环,而我们自己在for循环前面增加的#pragma编译指令大多数都是无效的,即那个for循环已经被编译器识别能被向量化  
但这并不意味我们的努力是白费的,经过比对编译过程中的info信息,经过我们的openMP优化后的源代码编译信息和普通代码编译信息虽大部分相同,但仍有差异.最终,这些差异给我们带来了少数的性能提升