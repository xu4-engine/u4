#ifndef MATH3D_H
#define MATH3D_H
/*
  3D math library
  Copyright 2022 Karl Robillard

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#define PI              3.14159265358979323846
#define degToRad(deg)   ((PI/180.0)*(double)(deg))
#define radToDeg(rad)   ((180.0/PI)*(double)(rad))

#ifdef __cplusplus
extern "C" {
#endif

/* Matrix functions */

extern float m4_identity[16];
#define m4_loadIdentity(mat)    memcpy(mat, m4_identity, sizeof(m4_identity))

void m4_loadRotation( float* mat, const float* axis, float radians );
void m4_perspective( float* mat, float fovYDegrees, float aspect,
                     float zNear, float zFar );
void m4_ortho( float* mat, float left, float right, float bottom, float top,
               float zNear, float zFar );
void m4_matrixMult( const float* a, const float* b, float* result );
void m4_transLocal( float* mat, float x, float y, float z );
void m4_matrixTranspose( float* mat, const float* a );
void m4_matrixInverse( float* mat, const float* a );


/* Vector functions */

float v3_distance( const float* vecA, const float* vecB );
void  v3_transform( float* pnt, const float* mat );
void  v3_transform3x3( const float* pnt, const float* mat, float* result );
void  v3_reflect( const float* a, const float* b, float* result );
void  v3_lineToPoint( const float* a, const float* b, const float* pnt,
                      float* vec );
float v3_normalize( float* vec );

#define v3_dot(A,B)     (A[0]*B[0] + A[1]*B[1] + A[2]*B[2])
#define v3_lengthSq(A)  (A[0]*A[0] + A[1]*A[1] + A[2]*A[2])

#define v3_cross(a,b,res) \
    res[0] = (a[1] * b[2]) - (a[2] * b[1]); \
    res[1] = (a[2] * b[0]) - (a[0] * b[2]); \
    res[2] = (a[0] * b[1]) - (a[1] * b[0])

#define v3_set(R,X,Y,Z) R[0] = X; R[1] = Y; R[2] = Z

#define v3_sub(R,A,B) \
    R[0] = A[0] - B[0]; \
    R[1] = A[1] - B[1]; \
    R[2] = A[2] - B[2]

#ifdef __cplusplus
}
#endif

#endif  /* MATH3D_H */
