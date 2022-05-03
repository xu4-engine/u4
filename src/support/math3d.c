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


#include <math.h>
#include "math3d.h"


enum eMatrixIndex {
    k00, k01, k02, k03,
    k10, k11, k12, k13,
    k20, k21, k22, k23,
    k30, k31, k32, k33,

    kX = 12,
    kY,
    kZ
};


float m4_identity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};


#if 0
/**
  \param mat   Result matrix.
  \param axis  Unit vector on axis of rotation.
*/
void m4_loadRotation( float* mat, const float* axis, float radians )
{
    float x, y, z;
    float c, s, t;

    x = axis[0];
    y = axis[1];
    z = axis[2];

    c = cosf( radians );
    s = sinf( radians );
    t = 1.0f - c;

    mat[ 0 ] = t*x*x + c;
    mat[ 1 ] = t*x*y + s*z;
    mat[ 2 ] = t*x*z - s*y;
    mat[ 3 ] = 0.0f;

    mat[ 4 ] = t*x*y - s*z;
    mat[ 5 ] = t*y*y + c;
    mat[ 6 ] = t*y*z + s*x;
    mat[ 7 ] = 0.0f;

    mat[  8 ] = t*x*z + s*y;
    mat[  9 ] = t*y*z - s*x;
    mat[ 10 ] = t*z*z + c;
    mat[ 11 ] = 0.0f;

    mat[ 12 ] = 0.0f;
    mat[ 13 ] = 0.0f;
    mat[ 14 ] = 0.0f;
    mat[ 15 ] = 1.0f;
}
#endif


void m4_perspective( float* mat, float fovYDegrees, float aspect,
                     float zNear, float zFar )
{
    float xmax, ymax, depth;

    ymax = zNear * tanf(fovYDegrees * PI / 360.0f);
    xmax = ymax * aspect;
    depth = zFar - zNear;

    mat[0] = zNear / xmax;
    mat[1] = mat[2] = mat[3] = mat[4] = 0.0f;
    mat[5] = zNear / ymax;
    mat[6] = mat[7] = mat[8] = mat[9] = 0.0f;
    mat[10] = (-zFar - zNear) / depth;
    mat[11] = -1.0f;
    mat[14] = (-2.0f * zNear * zFar) / depth;
    mat[12] = mat[13] = mat[15] = 0.0f;
}


void m4_ortho( float* mat, float left, float right, float bottom, float top,
               float zNear, float zFar )
{
    //if( left == right || bottom == top || zNear == zFar )
    //    return;

    mat[k00] = 2.0f / (right - left);
    mat[k10] = 0.0f;
    mat[k20] = 0.0f;
    mat[k30] = -(right + left) / (right - left);

    mat[k01] = 0.0f;
    mat[k11] = 2.0f / (top - bottom);
    mat[k21] = 0.0f;
    mat[k31] = -(top + bottom) / (top - bottom);

    mat[k02] = 0.0f;
    mat[k12] = 0.0f;
    mat[k22] = -2.0f / (zFar - zNear);
    mat[k32] = -(zFar + zNear) / (zFar - zNear);

    mat[k03] = 0.0f;
    mat[k13] = 0.0f;
    mat[k23] = 0.0f;
    mat[k33] = 1.0f;
}


/**
  Result can be the same matrix as A, but not B.
*/
void m4_matrixMult( const float* a, const float* b, float* result )
{
#define A(col,row)  a[(col<<2)+row]
#define R(col,row)  result[(col<<2)+row]

    int i;
    for( i = 0; i < 4; i++ )
    {
        float ai0=A(0,i), ai1=A(1,i), ai2=A(2,i), ai3=A(3,i);
        R(0,i) = ai0 * b[k00] + ai1 * b[k01] + ai2 * b[k02] + ai3 * b[k03];
        R(1,i) = ai0 * b[k10] + ai1 * b[k11] + ai2 * b[k12] + ai3 * b[k13];
        R(2,i) = ai0 * b[k20] + ai1 * b[k21] + ai2 * b[k22] + ai3 * b[k23];
        R(3,i) = ai0 * b[k30] + ai1 * b[k31] + ai2 * b[k32] + ai3 * b[k33];
    }
}


#if 0
void m4_transLocal( float* mat, float x, float y, float z )
{
    if( x )
    {
        mat[ kX ] += mat[ k00 ] * x;
        mat[ kY ] += mat[ k01 ] * x;
        mat[ kZ ] += mat[ k02 ] * x;
    }
    if( y )
    {
        mat[ kX ] += mat[ k10 ] * y;
        mat[ kY ] += mat[ k11 ] * y;
        mat[ kZ ] += mat[ k12 ] * y;
    }
    if( z )
    {
        mat[ kX ] += mat[ k20 ] * z;
        mat[ kY ] += mat[ k21 ] * z;
        mat[ kZ ] += mat[ k22 ] * z;
    }
}


/**
  Transpose 3x3 submatrix to negate rotation.
*/
void m4_matrixTranspose( float* mat, const float* a )
{
    mat[ k00 ] = a[ k00 ];
    mat[ k01 ] = a[ k10 ];
    mat[ k02 ] = a[ k20 ];
    mat[ k03 ] = a[ k03 ];

    mat[ k10 ] = a[ k01 ];
    mat[ k11 ] = a[ k11 ];
    mat[ k12 ] = a[ k21 ];
    mat[ k13 ] = a[ k13 ];

    mat[ k20 ] = a[ k02 ];
    mat[ k21 ] = a[ k12 ];
    mat[ k22 ] = a[ k22 ];
    mat[ k23 ] = a[ k23 ];
}


/**
  Negates rotation and translation.
*/
void m4_matrixInverse( float* mat, const float* a )
{
    ur_matrixTranspose( mat, a );

    // Negate translation.

    mat[ kX ] = - (a[kX] * a[k00]) - (a[kY] * a[k01]) - (a[kZ] * a[k02]);
    mat[ kY ] = - (a[kX] * a[k10]) - (a[kY] * a[k11]) - (a[kZ] * a[k12]);
    mat[ kZ ] = - (a[kX] * a[k20]) - (a[kY] * a[k21]) - (a[kZ] * a[k22]);
    mat[ k33 ] = a[ k33 ];
}
#endif


/*--------------------------------------------------------------------------*/


/**
  Returns the distance from vecA to vecB.
*/
float v3_distance( const float* vecA, const float* vecB )
{
    float dx, dy, dz;

    dx = vecB[0] - vecA[0];
    dy = vecB[1] - vecA[1];
    dz = vecB[2] - vecA[2];

    return sqrtf( dx * dx + dy * dy + dz * dz );
}


#if 0
/**
  Applies the entire matrix to this point.
*/
void v3_transform( float* pnt, const float* mat )
{
    float ox, oy, oz;

    ox = pnt[0];
    oy = pnt[1];
    oz = pnt[2];

    pnt[0] = mat[ k00 ] * ox + mat[ k10 ] * oy + mat[ k20 ] * oz + mat[ k30 ];
    pnt[1] = mat[ k01 ] * ox + mat[ k11 ] * oy + mat[ k21 ] * oz + mat[ k31 ];
    pnt[2] = mat[ k02 ] * ox + mat[ k12 ] * oy + mat[ k22 ] * oz + mat[ k32 ];
}


/**
  Applies the upper 3x3 portion of the matrix to this point.
  Result can be the same as pnt.
*/
void v3_transform3x3( const float* pnt, const float* mat, float* result )
{
    float ox, oy, oz;

    ox = pnt[0];
    oy = pnt[1];
    oz = pnt[2];

    result[0] = mat[ k00 ] * ox + mat[ k10 ] * oy + mat[ k20 ] * oz;
    result[1] = mat[ k01 ] * ox + mat[ k11 ] * oy + mat[ k21 ] * oz;
    result[2] = mat[ k02 ] * ox + mat[ k12 ] * oy + mat[ k22 ] * oz;
}


/**
  Result can be the same as either a or b.
*/
void v3_reflect( const float* a, const float* b, float* result )
{
    float dot2 = v3_dot(a, b) * 2.0f;

    result[0] = a[0] - (b[0] * dot2);
    result[1] = a[1] - (b[1] * dot2);
    result[2] = a[2] - (b[2] * dot2);
}


/**
  Returns shortest vector from line a-b to point.

  \param a    First line endpoint.
  \param b    Second line endpoint.
  \param pnt
  \param vec  Vector result.  This pointer may be the same as a, b, or pnt.
*/
void v3_lineToPoint( const float* a, const float* b, const float* pnt,
                     float* vec )
{
    float dir[3];
    float ft;

    dir[0] = b[0] - a[0];
    dir[1] = b[1] - a[1];
    dir[2] = b[2] - a[2];

    vec[0] = pnt[0] - a[0];
    vec[1] = pnt[1] - a[1];
    vec[2] = pnt[2] - a[2];

    ft = v3_dot( vec, dir );
    if( ft > 0.0 )
    {
        float rlen = v3_lengthSq( dir );
        if( rlen < ft )
        {
            vec[0] -= dir[0];
            vec[1] -= dir[1];
            vec[2] -= dir[2];
        }
        else
        {
            ft /= rlen;
            vec[0] -= ft * dir[0];
            vec[1] -= ft * dir[1];
            vec[2] -= ft * dir[2];
        }
    }
}
#endif


float v3_normalize( float* vec )
{
    float len = sqrtf( v3_lengthSq(vec) );
    if( len > 0.0f )
    {
        float inv = 1.0f / len;
        vec[0] *= inv;
        vec[1] *= inv;
        vec[2] *= inv;
    }
    return len;
}
