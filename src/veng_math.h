#ifndef VENG_MATH_H

typedef signed long long s64;
typedef unsigned long long u64;
typedef signed int s32;
typedef unsigned int u32;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int b32;
typedef float f32;
typedef double f64;

#define PI32 3.14159265359f
#define DEG2RAD(degrees) ((degrees) * PI32 / 180.0f)
#define RAD2DEG(radians) ((radians) * 180.0f / PI32)
#define MINIMUM(A, B) ((A < B) ? (A) : (B))
#define MAXIMUM(A, B) ((A > B) ? (A) : (B))
#define ARRAY_COUNT(arr) sizeof(arr) / sizeof(arr[0])

/* 
 * ------------ SCALARS ------------
 */
#include "math.h"
#include "float.h"
inline f32 SquareRoot(f32 value)
{
    f32 result = sqrtf(value);
    return(result);
}
inline f32 Cosine(f32 value)
{
    f32 result = cosf(value);
    return(result);
}
inline f32 Sine(f32 value)
{
    f32 result = sinf(value);
    return(result);
}
inline f32 Tangent(f32 value)
{
    f32 result = tanf(value);
    return(result);
}
inline f32 Arctangent2(f32 a, f32 b)
{
    f32 result = atan2f(a, b);
    return(result);
}
inline f32 Arccosine(f32 a)
{
    f32 result = acosf(a);
    return(result);
}
inline s32 Floor(f32 value)
{
    s32 result = (s32)floorf(value);
    return(result);
}
inline f32 Square(f32 value)
{
    f32 result = value * value;
    return(result);
}
inline f32 RandomFloat()
{
    f32 result = (f32)((f64)rand() / RAND_MAX);
    return(result);
}
inline f32 RandomFloat(f32 min, f32 max)
{
    f32 result = RandomFloat()*(max-min)+min;
    return(result);
}
inline f32 Clamp(f32 value, f32 min, f32 max)
{
    if(value < min) {return(min);}
    if(value > max) {return(max);}
    return(value);
}

/* 
 * ------------ 3D VECTORS ------------
 */
union vec3
{
    f32 e[3];
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 u, v, w;
    };

    inline f32 &operator[](int i) { return(e[i]); }
};

vec3 Vec3(f32 x, f32 y, f32 z)
{
    vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return(result);
}

vec3 Vec3(f32 x)
{
    vec3 result = Vec3(x, x, x);
    return(result);
}

inline vec3 operator*(vec3 v, f32 f)
{
    vec3 result;
    result.x = v.x * f;
    result.y = v.y * f;
    result.z = v.z * f;
    return(result);
}

inline vec3 operator*(f32 f, vec3 v)
{
    vec3 result;
    result.x = v.x * f;
    result.y = v.y * f;
    result.z = v.z * f;
    return(result);
}

inline vec3 operator/(vec3 v, f32 f)
{
    vec3 result;
    result.x = v.x / f;
    result.y = v.y / f;
    result.z = v.z / f;
    return(result);
}

inline vec3 operator+(vec3 v1, vec3 v2)
{
    vec3 result;
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    return(result);
}

inline vec3 operator+=(vec3 &v1, vec3 &v2)
{
    v1 = v1 + v2;
    return(v1);
}

inline vec3 operator-(vec3 v1, vec3 v2)
{
    vec3 result;
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    return(result);
}

inline vec3 operator-=(vec3 &v1, vec3 &v2)
{
    v1 = v1 - v2;
    return(v1);
}

inline vec3 operator-(vec3 v1)
{
    vec3 result;
    result.x = -v1.x;
    result.y = -v1.y;
    result.z = -v1.z;
    return(result);
}

inline vec3 HadamardProduct(vec3 v1, vec3 v2)
{
    vec3 result;
    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    result.z = v1.z * v2.z;
    return(result);
}

inline f32 DotProduct(vec3 v1, vec3 v2)
{
    f32 result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    return(result);
}

inline vec3 CrossProduct(vec3 v1, vec3 v2)
{
    vec3 result;
    result.x = v1.y * v2.z - v1.z * v2.y;
    result.y = v1.z * v2.x - v1.x * v2.z;
    result.z = v1.x * v2.y - v1.y * v2.x;
    return(result);
}

inline f32 LengthSquared(vec3 v1)
{
    f32 result = DotProduct(v1, v1);
    return(result);
}

inline f32 Length(vec3 v1)
{
    f32 result = SquareRoot(LengthSquared(v1));
    return(result);
}

inline vec3 Normalize(vec3 v1)
{
    f32 length = Length(v1);
    vec3 result;
    result.x = v1.x / length;
    result.y = v1.y / length;
    result.z = v1.z / length;
    return(result);
}

inline b32 NearZero(vec3 v1)
{
    f32 zero = 1e-8f;
    if((fabsf(v1.x) < zero) && (fabsf(v1.y) < zero) && (fabsf(v1.z) < zero))
    {
        return(1);
    }
    return(0);
}

/* 
 * ------------ MATRIX 3x3 ------------
 */
union mat3x3
{
    f32 e[3][3];

    f32 &operator()(u32 i, u32 j)
    {
        return(e[j][i]);
    }
    vec3 &operator[](u32 j)
    {
        return(*((vec3*)e[j]));
    }
};

mat3x3 operator*(mat3x3 &A, mat3x3 &B)
{
    mat3x3 result =
    {
        A(0,0) * B(0,0) + A(0,1) * B(1,0) + A(0,2) * B(2,0),
        A(0,0) * B(0,1) + A(0,1) * B(1,1) + A(0,2) * B(2,1),
        A(0,0) * B(0,2) + A(0,1) * B(1,2) + A(0,2) * B(2,2),
        A(1,0) * B(0,0) + A(1,1) * B(1,0) + A(1,2) * B(2,0),
        A(1,0) * B(0,1) + A(1,1) * B(1,1) + A(1,2) * B(2,1),
        A(1,0) * B(0,2) + A(1,1) * B(1,2) + A(1,2) * B(2,2),
        A(2,0) * B(0,0) + A(2,1) * B(1,0) + A(2,2) * B(2,0),
        A(2,0) * B(0,1) + A(2,1) * B(1,1) + A(2,2) * B(2,1),
        A(2,0) * B(0,2) + A(2,1) * B(1,2) + A(2,2) * B(2,2),
    };
    return(result);
}

vec3 operator*(mat3x3 &M, vec3 &v)
{
    vec3 result =
    {
        M(0,0) * v.x + M(0,1) * v.y + M(0,2) * v.z,
        M(1,0) * v.x + M(1,1) * v.y + M(1,2) * v.z,
        M(2,0) * v.x + M(2,1) * v.y + M(2,2) * v.z,
    };
    return(result);
}

#define VENG_MATH_H
#endif