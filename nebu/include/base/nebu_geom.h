#ifndef NEBU_GEOM_H
#define NEBU_GEOM_H

#include <HandmadeMath.h>

/* As of the HandmadeMath port, all linear-algebra operations live in
 * <HandmadeMath.h> as HMM_*. What's left here is the small set of geometric
 * helpers HMM intentionally doesn't cover: 2D segment-segment intersection,
 * AABBs, the 2D perpendicular, the triangle-normal helper, and the RGB
 * float-to-uint packer. See docs/plans/handmademath-port.md. */

/* Short aliases for the HMM types so the geometric structs below read
 * naturally. Old call sites in src/ also still use these names. */
typedef HMM_Vec2 vec2;
typedef HMM_Vec3 vec3;
typedef HMM_Mat4 matrix;

typedef struct {
  vec2 vStart, vDirection;
} segment2;

typedef struct {
  vec2 vMin, vMax;
} box2;

typedef struct {
  vec3 vMin, vMax;
} box3;

vec3 vec3_TriNormalDirection(vec3 v1, vec3 v2, vec3 v3);
unsigned int uintFromVec3(vec3 v);

vec2* segment2_Intersect(vec2* pOut, float* t1, float* t2, const segment2* s1,
                         const segment2* s2);
float segment2_Length(const segment2* s);

vec2 vec2_Orthogonal(vec2 v);

float box2_Width(const box2* pBox);
float box2_Height(const box2* pBox);
float box2_Diameter(const box2* pBox);
void box2_Center(vec2* pOut, const box2* pBox);
void box2_Init(box2* pBox);
void box2_Extend(box2* pBox, vec2 v);
void box3_Compute(box3* pBox, const vec3* pVertices, int nVertices);

#endif
