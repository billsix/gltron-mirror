#ifndef GRAPHICS_UTILITY_H
#define GRAPHICS_UTILITY_H

#include "video/nebu_font.h"
#include "video/nebu_video_types.h"

void rasonly(const Visual* d);
void doPerspective(float fov, float ratio, float znear, float zfar);
void doLookAt(const float* cam, const float* target, const float* up);
void drawText(const nebu_Font* ftx, float x, float y, float size,
              const char* text);

#endif
