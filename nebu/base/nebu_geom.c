#include "base/nebu_geom.h"
#include "base/nebu_math.h"

#include <assert.h>
#include <math.h>

/* Geometric helpers that HMM intentionally doesn't cover: 2D segment-segment
 * intersection, AABBs, the 2D perpendicular, the triangle-normal helper, and
 * the RGB packer. The linear-algebra helpers that used to live here moved to
 * HMM_* during the HandmadeMath port. */

vec3 vec3_TriNormalDirection(vec3 v1, vec3 v2, vec3 v3) {
  return HMM_Cross(HMM_SubV3(v2, v1), HMM_SubV3(v3, v1));
}

unsigned int uintFromVec3(vec3 v) {
  return (((unsigned int)(v.X * 127.0f + 128.0f)) << 16) +
         (((unsigned int)(v.Y * 127.0f + 128.0f)) << 8) +
         (((unsigned int)(v.Z * 127.0f + 128.0f)) << 0);
}

vec2 vec2_Orthogonal(vec2 v) { return HMM_V2(v.Y, -v.X); }

static int segment2_findT(float* t, const segment2* s, vec2 v) {
  float epsilon = 0.001f;
  if (fabs(s->vDirection.X) > fabs(s->vDirection.Y)) {
    *t = (v.X - s->vStart.X) / s->vDirection.X;
    if (fabs(v.Y - (s->vStart.Y + *t * s->vDirection.Y)) > epsilon) {
      return 1;
    }
  } else {
    *t = (v.Y - s->vStart.Y) / s->vDirection.Y;
    if (fabs(v.X - (s->vStart.X + *t * s->vDirection.X)) > epsilon) {
      return 1;
    }
  }
  return 0;
}

static vec2* segment2_IntersectParallel(vec2* pOut, float* t1, float* t2,
                                        const segment2* s1,
                                        const segment2* s2) {
  // if the lines don't overlap, return NULL
  // else find t2 for t1 == 0
  // if t2 in [0,1] return t2, t1 = 0
  // else find t1 for t2 == 0 and t2 == 1
  // if t1 < 0 return NULL (no intersection)
  // else return the smaller t1 and the corresponding t2

  float t;

  if (segment2_findT(t2, s2, s1->vStart)) {
    return NULL;
  }

  if (*t2 >= 0 && *t2 <= 1) {
    *pOut = s1->vStart;
    *t1 = 0;
    return pOut;
  }
  if (segment2_findT(t1, s1, s2->vStart)) return NULL;
  if (*t1 < 0) return NULL;
  vec2 vEnd2 = HMM_AddV2(s2->vStart, s2->vDirection);
  if (segment2_findT(&t, s1, vEnd2)) return NULL;
  assert(t >= 0);

  if (*t1 > 1 && t > 1) return NULL;
  if (t < *t1) {
    *t1 = t;
    *t2 = 1;
    *pOut = vEnd2;
  } else {
    *t2 = 0;
    *pOut = s2->vStart;
  }
  return pOut;
}

static vec2* segment2_IntersectNonParallel(vec2* pOut, float* t1, float* t2,
                                           const segment2* s1,
                                           const segment2* s2) {
  // homogeneous-coordinates line/line intersection
  vec3 tmp1 = HMM_V3(s1->vStart.X, s1->vStart.Y, 1.0f);
  vec3 tmp2 = HMM_V3(s1->vStart.X + s1->vDirection.X,
                     s1->vStart.Y + s1->vDirection.Y, 1.0f);
  vec3 v1 = HMM_Cross(tmp1, tmp2);

  tmp1 = HMM_V3(s2->vStart.X, s2->vStart.Y, 1.0f);
  tmp2 = HMM_V3(s2->vStart.X + s2->vDirection.X,
                s2->vStart.Y + s2->vDirection.Y, 1.0f);
  vec3 v2 = HMM_Cross(tmp1, tmp2);

  vec3 vIntersection = HMM_Cross(v1, v2);
  pOut->X = vIntersection.X / vIntersection.Z;
  pOut->Y = vIntersection.Y / vIntersection.Z;

  if (fabs(s1->vDirection.X) > fabs(s1->vDirection.Y))
    *t1 = (pOut->X - s1->vStart.X) / s1->vDirection.X;
  else
    *t1 = (pOut->Y - s1->vStart.Y) / s1->vDirection.Y;
  if (fabs(s2->vDirection.X) > fabs(s2->vDirection.Y))
    *t2 = (pOut->X - s2->vStart.X) / s2->vDirection.X;
  else
    *t2 = (pOut->Y - s2->vStart.Y) / s2->vDirection.Y;

  return pOut;
}

vec2* segment2_Intersect(vec2* pOut, float* t1, float* t2, const segment2* s1,
                         const segment2* s2) {
  // check if s1, s2 are parallel — orthogonal-of-s2 dotted with s1 ~ 0
  vec2 perp = vec2_Orthogonal(s2->vDirection);
  if (fabs(HMM_DotV2(s1->vDirection, perp)) < 0.1) {
    pOut = segment2_IntersectParallel(pOut, t1, t2, s1, s2);
    if (!pOut) {
      *t1 = 0;
      *t2 = 0;
    }
  } else {
    pOut = segment2_IntersectNonParallel(pOut, t1, t2, s1, s2);
  }
  return pOut;
}

float segment2_Length(const segment2* s) { return HMM_LenV2(s->vDirection); }

float box2_Width(const box2* pBox) { return pBox->vMax.X - pBox->vMin.X; }
float box2_Height(const box2* pBox) { return pBox->vMax.Y - pBox->vMin.Y; }

float box2_Diameter(const box2* pBox) {
  return HMM_LenV2(HMM_SubV2(pBox->vMax, pBox->vMin));
}

void box2_Center(vec2* pOut, const box2* pBox) {
  *pOut = HMM_MulV2F(HMM_AddV2(pBox->vMin, pBox->vMax), 0.5f);
}

void box2_Init(box2* pBox) {
  pBox->vMin.X = FLT_MAX;
  pBox->vMin.Y = FLT_MAX;
  pBox->vMax.X = FLT_MIN;
  pBox->vMax.Y = FLT_MIN;
}

void box2_Extend(box2* pBox, vec2 v) {
  if (pBox->vMin.X > v.X) pBox->vMin.X = v.X;
  if (pBox->vMin.Y > v.Y) pBox->vMin.Y = v.Y;
  if (pBox->vMax.X < v.X) pBox->vMax.X = v.X;
  if (pBox->vMax.Y < v.Y) pBox->vMax.Y = v.Y;
}

static void box3_Init(box3* pBox) {
  pBox->vMin.X = FLT_MAX;
  pBox->vMin.Y = FLT_MAX;
  pBox->vMin.Z = FLT_MAX;
  pBox->vMax.X = FLT_MIN;
  pBox->vMax.Y = FLT_MIN;
  pBox->vMax.Z = FLT_MIN;
}

void box3_Compute(box3* pBox, const vec3* pVertices, int nVertices) {
  box3_Init(pBox);
  for (int i = 0; i < nVertices; i++) {
    for (int j = 0; j < 3; j++) {
      float f = pVertices[i].Elements[j];
      if (f < pBox->vMin.Elements[j]) pBox->vMin.Elements[j] = f;
      if (f > pBox->vMax.Elements[j]) pBox->vMax.Elements[j] = f;
    }
  }
}
