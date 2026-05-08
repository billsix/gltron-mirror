#include "video/video.h"
#include "video/nebu_font.h"
#include "game/game.h"

#include "video/nebu_video_utility.h"
#include "video/nebu_renderer_gl.h"
#include "base/nebu_math.h"
#include "base/nebu_geom.h"

#include <string.h>

void rasonly(Visual* d) {
  nebu_Video_rasonly(d->vp_x, d->vp_y, d->vp_w, d->vp_h);
}

void doPerspective(float fov, float ratio, float znear, float zfar) {
  float top;
  float left;

  top = tanf(fov * PI / 360.0) * znear;
  left = -top * ratio;
  glFrustum(left, -left, -top, top, znear, zfar);
}

void doLookAt(float* cam, float* target, float* up) {
  vec3 vCam = HMM_V3(cam[0], cam[1], cam[2]);
  vec3 vTarget = HMM_V3(target[0], target[1], target[2]);
  vec3 vUp = HMM_V3(up[0], up[1], up[2]);

  vec3 z = HMM_NormV3(HMM_SubV3(vCam, vTarget));
  vec3 x = HMM_NormV3(HMM_Cross(vUp, z));
  vec3 y = HMM_NormV3(HMM_Cross(z, x));

  /* The view rotation has the basis vectors as ROWS, not columns — that's
     the world-to-camera inverse rotation. HMM_Mat4 is column-major, so
     each Column[c] holds the c-th component of every basis vector. */
  matrix m;
  m.Columns[0] = HMM_V4(x.X, y.X, z.X, 0);
  m.Columns[1] = HMM_V4(x.Y, y.Y, z.Y, 0);
  m.Columns[2] = HMM_V4(x.Z, y.Z, z.Z, 0);
  m.Columns[3] = HMM_V4(0, 0, 0, 1);
  glMultMatrixf(m.Elements[0]);

  /* Translate Eye to Origin */
  glTranslatef(-cam[0], -cam[1], -cam[2]);
}

void drawText(nebu_Font* ftx, float x, float y, float size, const char* text) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);

  glPushMatrix();

  glTranslatef(x, y, 0);
  glScalef(size, size, size);
  nebu_Font_Render(ftx, text, strlen(text));

  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
}
