#pragma once
#include "imath-api-export.h"

#include <imath_vec.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Imath_2_5__Box_Imath__Vec3_float___t_s {
    Imath_V3f_t min;
    Imath_V3f_t max;
} IMATH_CPPMM_ALIGN(4) Imath_2_5__Box_Imath__Vec3_float___t;
typedef Imath_2_5__Box_Imath__Vec3_float___t Imath_Box3f_t;

typedef struct Imath_2_5__Box_Imath__Vec3_int___t_s {
    Imath_V3i_t min;
    Imath_V3i_t max;
} IMATH_CPPMM_ALIGN(4) Imath_2_5__Box_Imath__Vec3_int___t;
typedef Imath_2_5__Box_Imath__Vec3_int___t Imath_Box3i_t;


IMATH_CPPMM_API unsigned int Imath_2_5__Box_Imath__Vec3_float___extendBy(
    Imath_Box3f_t * this_
    , Imath_V3f_t const * point);
#define Imath_Box3f_extendBy Imath_2_5__Box_Imath__Vec3_float___extendBy


IMATH_CPPMM_API unsigned int Imath_2_5__Box_Imath__Vec3_float___extendBy_1(
    Imath_Box3f_t * this_
    , Imath_Box3f_t const * box);
#define Imath_Box3f_extendBy_1 Imath_2_5__Box_Imath__Vec3_float___extendBy_1


IMATH_CPPMM_API unsigned int Imath_2_5__Box_Imath__Vec3_int___extendBy(
    Imath_Box3i_t * this_
    , Imath_V3i_t const * point);
#define Imath_Box3i_extendBy Imath_2_5__Box_Imath__Vec3_int___extendBy


IMATH_CPPMM_API unsigned int Imath_2_5__Box_Imath__Vec3_int___extendBy_1(
    Imath_Box3i_t * this_
    , Imath_Box3i_t const * box);
#define Imath_Box3i_extendBy_1 Imath_2_5__Box_Imath__Vec3_int___extendBy_1


#ifdef __cplusplus
}
#endif
