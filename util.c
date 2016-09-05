/* util routines */
#include "raster.h"
#include <math.h>

/* Triangle rasterization based on:
 * http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
 * Copyright (c) 2015 Bastian Molkenthin

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

void fill_triang_top(gfx_surface *surface, vec3_t *v1, vec3_t *v2, vec3_t *v3, unsigned int color) {

  // v1, v2, v3 already have screen coords
  unsigned int cur_y;
  float v2v1 = (v2->x - v1->x) / (v2->y - v1->y);
  float v3v1 = (v3->x - v1->x) / (v3->y - v1->y);

  float x1 = v1->x; // v1 = top vec3_t
  float x2 = v1->x;

  for (cur_y = (unsigned int)v1->y; cur_y <= (unsigned int)v2->y; cur_y++) {
    gfx_line(surface, x1, cur_y, x2, cur_y, color);
    x1 += v2v1;
    x2 += v3v1;
  }

}

void fill_triang_bottom(gfx_surface *surface, vec3_t *v1, vec3_t *v2, vec3_t *v3, unsigned int color) {

  // v1, v2, v3 already have screen coords
  unsigned int cur_y;
  float v1v3 = (v3->x - v1->x) / (v3->y - v1->y);
  float v2v3 = (v3->x - v2->x) / (v3->y - v2->y);

  float x1 = v3->x; // v3 = bottom vec3_t
  float x2 = v3->x;

  for (cur_y = (unsigned int)v3->y; cur_y > (unsigned int)v1->y; cur_y--) {
    gfx_line(surface, x1, cur_y, x2, cur_y, color);
    x1 -= v1v3;
    x2 -= v2v3;
  }

}


void fill_triangle(gfx_surface *surface, vec3_t *v1, vec3_t *v2, vec3_t *v3, unsigned int color) {

  static vec3_t vt1, vt2, vt3, vt4, temp;
  vt1 = *v1;
  vt2 = *v2;
  vt3 = *v3;

  if (vt1.y > vt2.y) {
    vt1 = *v2;
    vt2 = *v1;
  }

  if (vt1.y > vt3.y) {
    vt3 = vt1;
    vt1 = *v3;
  }

  if (vt2.y > vt3.y) {
    temp = vt3;
    vt3 = vt2;
    vt2 = temp;
  }

  if (vt2.y == vt3.y) {
    fill_triang_top(surface, &vt1, &vt2, &vt3, color);
  } else if (vt1.y == vt2.y) {
    fill_triang_bottom(surface, &vt1, &vt2, &vt3, color);
  } else {
    vt4.x = (vt1.x + ((vt2.y - vt1.y) / (vt3.y - vt1.y)) * (vt3.x - vt1.x));
    vt4.y = vt2.y;
    fill_triang_top(surface, &vt1, &vt2, &vt4, color);
    fill_triang_bottom(surface, &vt2, &vt4, &vt3, color);
  }
}


