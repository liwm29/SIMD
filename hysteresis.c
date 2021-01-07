/*******************************************************************************
 * FILE: hysteresis.c
 * This code was re-written by Mike Heath from original code obtained indirectly
 * from Michigan State University. heath@csee.usf.edu (Re-written in 1996).
 *******************************************************************************/
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "funcProtocol.h"
#include "typedef.h"
#define VERBOSE 0

#define NOEDGE 255
#define POSSIBLE_EDGE 128
#define EDGE 0

/*******************************************************************************
 * PROCEDURE: follow_edges
 * PURPOSE: This procedure edges is a recursive routine that traces edgs along
 * all paths whose magnitude values remain above some specifyable lower
 * threshhold.
 * NAME: Mike Heath
 * DATE: 2/15/96
 *******************************************************************************/
// void follow_edges_wrapper(unsigned char* edgemapptr,
//                           short* edgemagptr,
//                           short lowval,
//                           int cols) {}
void follow_edges(unsigned char* edgemapptr,
                  short* edgemagptr,
                  short lowval,
                  int cols) {
  int x[8] = {1, 1, 0, -1, -1, -1, 0, 1}, y[8] = {0, 1, 1, 1, 0, -1, -1, -1};

#pragma omp simd
  for (int i = 0; i < 8; i++) {
    short* tempmapptr = edgemapptr - y[i] * cols + x[i];
    unsigned char* tempmagptr = edgemagptr - y[i] * cols + x[i];

    if ((*tempmapptr == POSSIBLE_EDGE) && (*tempmagptr > lowval)) {
      *tempmapptr = (unsigned char)EDGE;
      follow_edges(tempmapptr, tempmagptr, lowval, cols);
    }
  }
}

/*******************************************************************************
 * PROCEDURE: apply_hysteresis
 * PURPOSE: This routine finds edges that are above some high threshhold or
 * are connected to a high pixel by a path of pixels greater than a low
 * threshold.
 * NAME: Mike Heath
 * DATE: 2/15/96
 *******************************************************************************/
void apply_hysteresis(short int* mag,
                      unsigned char* nms,
                      int rows,
                      int cols,
                      float tlow,
                      float thigh,
                      unsigned char* edge) {
  int numedges, lowcount, highcount, lowthreshold, highthreshold, i,
      hist[32768], rr, cc;
  short int maximum_mag, sumpix;

  /****************************************************************************
   * Initialize the edge map to possible edges everywhere the non-maximal
   * suppression suggested there could be an edge except for the border. At
   * the border we say there can not be an edge because it makes the
   * follow_edges algorithm more efficient to not worry about tracking an
   * edge off the side of the image.
   ****************************************************************************/
#pragma omp simd
  for (int i = 0; i < rows * cols; i++) {
    edge[i] = nms[i] == POSSIBLE_EDGE ? POSSIBLE_EDGE : NOEDGE;
  }
  // for (r = 0, pos = 0; r < rows; r++) {
  //   for (c = 0; c < cols; c++, pos++) {
  //     if (nms[pos] == POSSIBLE_EDGE)
  //       edge[pos] = POSSIBLE_EDGE;
  //     else
  //       edge[pos] = NOEDGE;
  //   }
  // }

#pragma omp simd
  for (int r = 0; r < rows; r++) {
    edge[r * cols] = NOEDGE;
    edge[(r + 1) * cols - 1] = NOEDGE;
  }
  // for (r = 0, pos = 0; r < rows; r++, pos += cols) {
  //   edge[pos] = NOEDGE;
  //   edge[pos + cols - 1] = NOEDGE;
  // }
  const int pos = (rows - 1) * cols;
#pragma omp simd
  for (int c = 0; c < cols; c++) {
    edge[c] = NOEDGE;
    edge[pos + c] = NOEDGE;
  }
  // for (c = 0; c < cols; c++, pos++) {
  //   edge[c] = NOEDGE;
  //   edge[pos] = NOEDGE;
  // }

  /****************************************************************************
   * Compute the histogram of the magnitude image. Then use the histogram to
   * compute hysteresis thresholds.
   ****************************************************************************/
  memset(hist, 0, 32768 * sizeof(hist[0]));
// for (int r = 0; r < 32768; r++)
//   hist[r] = 0;

// u cant use simd here;but we can unroll or parallel for with atomic_add
#pragma unroll
  for (int i = 0; i < rows * cols; i++) {
#pragma omp atomic
    hist[mag[i]] += edge[i] == POSSIBLE_EDGE ? 1 : 0;
    // __sync_fetch_and_add(&hist[mag[i]], edge[i] == POSSIBLE_EDGE ? 1 : 0);
  }
  // for (int r = 0, pos = 0; r < rows; r++) {
  //   for (int c = 0; c < cols; c++, pos++) {
  //     if (edge[pos] == POSSIBLE_EDGE)
  //       hist[mag[pos]]++;
  //   }
  // }

  /****************************************************************************
   * Compute the number of pixels that passed the nonmaximal suppression.
   ****************************************************************************/
  numedges = 0;
#pragma omp simd reduction(+ : numedges) reduction(max : maximum_mag)
  for (int r = 1; r < 32768; r++) {
    if (hist[r] != 0) {
      maximum_mag = r;
      numedges += hist[r];
    }
  }

  highcount = (int)(numedges * thigh + 0.5);

  /****************************************************************************
   * Compute the high threshold value as the (100 * thigh) percentage point
   * in the magnitude of the gradient histogram of all the pixels that passes
   * non-maximal suppression. Then calculate the low threshold as a fraction
   * of the computed high threshold value. John Canny said in his paper
   * "A Computational Approach to Edge Detection" that "The ratio of the
   * high to low threshold in the implementation is in the range two or three
   * to one." That means that in terms of this implementation, we should
   * choose tlow ~= 0.5 or 0.33333.
   ****************************************************************************/
  int r = 1;
  numedges = hist[1];
  while ((r < (maximum_mag - 1)) && (numedges < highcount)) {
    r++;
    numedges += hist[r];
  }
  highthreshold = r;
  lowthreshold = (int)(highthreshold * tlow + 0.5);

  if (VERBOSE) {
    printf("The input low and high fractions of %f and %f computed to\n", tlow,
           thigh);
    printf("magnitude of the gradient threshold values of: %d %d\n",
           lowthreshold, highthreshold);
  }

/****************************************************************************
 * This loop looks for pixels above the highthreshold to locate edges and
 * then calls follow_edges to continue the edge.
 ****************************************************************************/
#pragma omp simd
  for (int i = 0; i < rows * cols; i++) {
    if (edge[i] == POSSIBLE_EDGE && mag[i] >= highthreshold) {
      edge[i] = EDGE;
      follow_edges(edge + i, mag + i, lowthreshold, cols);
    }
  }
// for (r = 0, pos = 0; r < rows; r++) {
//   for (c = 0; c < cols; c++, pos++) {
//     if ((edge[pos] == POSSIBLE_EDGE) && (mag[pos] >= highthreshold)) {
//       edge[pos] = EDGE;
//       follow_edges((edge + pos), (mag + pos), lowthreshold, cols);
//     }
//   }
// }

/****************************************************************************
 * Set all the remaining possible edges to non-edges.
 ****************************************************************************/
#pragma omp simd
  for (int i = 0; i < rows * cols; i++) {
    edge[i] = (!(!(edge[i] - EDGE))) * NOEDGE;
  }
  // for (r = 0, pos = 0; r < rows; r++) {
  //   for (c = 0; c < cols; c++, pos++)
  //     if (edge[pos] != EDGE)
  //       edge[pos] = NOEDGE;
  // }
}

/*******************************************************************************
 * PROCEDURE: non_max_supp
 * PURPOSE: This routine applies non-maximal suppression to the magnitude of
 * the gradient image.
 * NAME: Mike Heath
 * DATE: 2/15/96
 *******************************************************************************/
void non_max_supp(short* mag,
                  short* gradx,
                  short* grady,
                  int nrows,
                  int ncols,
                  unsigned char* result) {
  int rowcount, colcount, count;
  short *magrowptr, *magptr;
  short *gxrowptr, *gxptr;
  short *gyrowptr, *gyptr, z1, z2;
  short m00, gx, gy;
  float mag1, mag2, xperp, yperp;
  unsigned char *resultrowptr, *resultptr;

  /****************************************************************************
   * Zero the edges of the result image.
   ****************************************************************************/
  resultrowptr = result;
  resultptr = result + ncols * (nrows - 1);

  memset(resultrowptr, 0, ncols * sizeof(resultrowptr[0]));
  memset(resultptr, 0, ncols * sizeof(resultptr[0]));
  // #pragma omp simd
  //   for (int i = 0; i < ncols; i++) {
  //     *(resultrowptr + i) = (unsigned char)0;
  //     *(resultptr + i) = (unsigned char)0;
  //   }

  // for (count = 0, resultrowptr = result,
  //     resultptr = result + ncols * (nrows - 1);
  //      count < ncols; resultptr++, resultrowptr++, count++) {
  //   *resultrowptr = *resultptr = (unsigned char)0;
  // }

  resultptr = result;
  resultrowptr = result + ncols - 1;
#pragma omp simd
  for (int i = 0; i < nrows; i++) {
    *(resultptr + i * ncols) = (unsigned char)0;
    *(resultrowptr + i * ncols) = (unsigned char)0;
  }
  // for (count = 0, resultptr = result, resultrowptr = result + ncols - 1;
  //      count < nrows; count++, resultptr += ncols, resultrowptr += ncols) {
  //   *resultptr = *resultrowptr = (unsigned char)0;
  // }

  /****************************************************************************
   * Suppress non-maximum points.
   ****************************************************************************/

  // magrowptr = mag + ncols + 1;
  // gxrowptr = gradx + ncols + 1;
  // gyrowptr = grady + ncols + 1;
  // resultrowptr = result + ncols + 1;
  // int offset = 0;
  // #pragma omp simd
  // for (int i = 1; i < (nrows - 2) * (ncols - 2); i++,
  //          offset = ncols * (i / (nrows - 2)), magrowptr += offset,
  //          gyrowptr += offset, resultrowptr += offset, magptr+=1,
  //          gxptr+=1,gyptr+=1,resultptr+=1) {
  for (rowcount = 1, magrowptr = mag + ncols + 1, gxrowptr = gradx + ncols + 1,
      gyrowptr = grady + ncols + 1, resultrowptr = result + ncols + 1;
       rowcount < nrows - 2; rowcount++, magrowptr += ncols, gyrowptr += ncols,
      gxrowptr += ncols, resultrowptr += ncols) {
    for (colcount = 1, magptr = magrowptr, gxptr = gxrowptr, gyptr = gyrowptr,
        resultptr = resultrowptr;
         colcount < ncols - 2;
         colcount++, magptr++, gxptr++, gyptr++, resultptr++) {
      m00 = *magptr;
      if (m00 == 0) {
        *resultptr = (unsigned char)NOEDGE;
      } else {
        xperp = -(gx = *gxptr) / ((float)m00);
        yperp = (gy = *gyptr) / ((float)m00);
      }

      if (gx >= 0) {
        if (gy >= 0) {
          if (gx >= gy) {
            /* 111 */
            /* Left point */
            z1 = *(magptr - 1);
            z2 = *(magptr - ncols - 1);

            mag1 = (m00 - z1) * xperp + (z2 - z1) * yperp;

            /* Right point */
            z1 = *(magptr + 1);
            z2 = *(magptr + ncols + 1);

            mag2 = (m00 - z1) * xperp + (z2 - z1) * yperp;
          } else {
            /* 110 */
            /* Left point */
            z1 = *(magptr - ncols);
            z2 = *(magptr - ncols - 1);

            mag1 = (z1 - z2) * xperp + (z1 - m00) * yperp;

            /* Right point */
            z1 = *(magptr + ncols);
            z2 = *(magptr + ncols + 1);

            mag2 = (z1 - z2) * xperp + (z1 - m00) * yperp;
          }
        } else {
          if (gx >= -gy) {
            /* 101 */
            /* Left point */
            z1 = *(magptr - 1);
            z2 = *(magptr + ncols - 1);

            mag1 = (m00 - z1) * xperp + (z1 - z2) * yperp;

            /* Right point */
            z1 = *(magptr + 1);
            z2 = *(magptr - ncols + 1);

            mag2 = (m00 - z1) * xperp + (z1 - z2) * yperp;
          } else {
            /* 100 */
            /* Left point */
            z1 = *(magptr + ncols);
            z2 = *(magptr + ncols - 1);

            mag1 = (z1 - z2) * xperp + (m00 - z1) * yperp;

            /* Right point */
            z1 = *(magptr - ncols);
            z2 = *(magptr - ncols + 1);

            mag2 = (z1 - z2) * xperp + (m00 - z1) * yperp;
          }
        }
      } else {
        if ((gy = *gyptr) >= 0) {
          if (-gx >= gy) {
            /* 011 */
            /* Left point */
            z1 = *(magptr + 1);
            z2 = *(magptr - ncols + 1);

            mag1 = (z1 - m00) * xperp + (z2 - z1) * yperp;

            /* Right point */
            z1 = *(magptr - 1);
            z2 = *(magptr + ncols - 1);

            mag2 = (z1 - m00) * xperp + (z2 - z1) * yperp;
          } else {
            /* 010 */
            /* Left point */
            z1 = *(magptr - ncols);
            z2 = *(magptr - ncols + 1);

            mag1 = (z2 - z1) * xperp + (z1 - m00) * yperp;

            /* Right point */
            z1 = *(magptr + ncols);
            z2 = *(magptr + ncols - 1);

            mag2 = (z2 - z1) * xperp + (z1 - m00) * yperp;
          }
        } else {
          if (-gx > -gy) {
            /* 001 */
            /* Left point */
            z1 = *(magptr + 1);
            z2 = *(magptr + ncols + 1);

            mag1 = (z1 - m00) * xperp + (z1 - z2) * yperp;

            /* Right point */
            z1 = *(magptr - 1);
            z2 = *(magptr - ncols - 1);

            mag2 = (z1 - m00) * xperp + (z1 - z2) * yperp;
          } else {
            /* 000 */
            /* Left point */
            z1 = *(magptr + ncols);
            z2 = *(magptr + ncols + 1);

            mag1 = (z2 - z1) * xperp + (m00 - z1) * yperp;

            /* Right point */
            z1 = *(magptr - ncols);
            z2 = *(magptr - ncols - 1);

            mag2 = (z2 - z1) * xperp + (m00 - z1) * yperp;
          }
        }
      }

      /* Now determine if the current point is a maximum point */

      if ((mag1 > 0.0) || (mag2 > 0.0)) {
        *resultptr = (unsigned char)NOEDGE;
      } else {
        if (mag2 == 0.0)
          *resultptr = (unsigned char)NOEDGE;
        else
          *resultptr = (unsigned char)POSSIBLE_EDGE;
      }
    }
  }
}