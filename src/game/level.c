#include "game/game_level.h"
#include "filesystem/path.h"
#include "Nebu_scripting.h"

#include "base/nebu_debug_memory.h"

void game_FreeLevel(game_level* l) {
  free(l->boundaries);
  free(l->spawnPoints);
  free(l);
}

void game_ScaleLevel(game_level* l, float fSize) {
  int i;
  for (i = 0; i < l->nBoundaries; i++) {
    l->boundaries[i].vStart = HMM_MulV2F(l->boundaries[i].vStart, fSize);
    l->boundaries[i].vDirection =
        HMM_MulV2F(l->boundaries[i].vDirection, fSize);
  }
  for (i = 0; i < l->nSpawnPoints; i++) {
    l->spawnPoints[i].v = HMM_MulV2F(l->spawnPoints[i].v, fSize);
  }

  l->boundingBox.vMin = HMM_MulV2F(l->boundingBox.vMin, fSize);
  l->boundingBox.vMax = HMM_MulV2F(l->boundingBox.vMax, fSize);
}

void computeBoundingBox(game_level* l) {
  int i;

  box2_Init(&l->boundingBox);
  for (i = 0; i < l->nBoundaries; i++) {
    vec2 vEnd = HMM_AddV2(l->boundaries[i].vStart, l->boundaries[i].vDirection);
    box2_Extend(&l->boundingBox, l->boundaries[i].vStart);
    box2_Extend(&l->boundingBox, vEnd);
  }
}

game_level* game_CreateLevel(void) {
  int i;
  game_level* l;

  l = malloc(sizeof(game_level));
  scripting_GetGlobal("level", NULL);
  // get scalability flag
  scripting_GetValue("scalable");
  scripting_GetIntegerResult(&l->scalable);
  // get number of spawnpoints
  scripting_GetValue("spawn");
  scripting_GetArraySize(&l->nSpawnPoints);
  // copy spawnpoints into vec2's
  l->spawnPoints = malloc(l->nSpawnPoints * sizeof(game_spawnpoint));

  // fixme, use scalability
  for (i = 0; i < l->nSpawnPoints; i++) {
    scripting_GetArrayIndex(i + 1);

    scripting_GetValue("x");
    scripting_GetFloatResult(&l->spawnPoints[i].v.X);
    scripting_GetValue("y");
    scripting_GetFloatResult(&l->spawnPoints[i].v.Y);
    scripting_GetValue("dir");
    scripting_GetIntegerResult(&l->spawnPoints[i].dir);

    scripting_Pop();  // index i
  }
  scripting_Pop();  // spawn

  // get number of boundary segments
  scripting_GetValue("boundary");
  scripting_GetArraySize(&l->nBoundaries);
  // copy boundaries into segments
  l->boundaries = malloc(l->nBoundaries * sizeof(segment2));
  for (i = 0; i < l->nBoundaries; i++) {
    scripting_GetArrayIndex(i + 1);

    scripting_GetArrayIndex(1);
    scripting_GetValue("x");
    scripting_GetFloatResult(&l->boundaries[i].vStart.X);
    scripting_GetValue("y");
    scripting_GetFloatResult(&l->boundaries[i].vStart.Y);
    scripting_Pop();  // index 0

    scripting_GetArrayIndex(2);
    {
      vec2 v;
      scripting_GetValue("x");
      scripting_GetFloatResult(&v.X);
      scripting_GetValue("y");
      scripting_GetFloatResult(&v.Y);
      l->boundaries[i].vDirection = HMM_SubV2(v, l->boundaries[i].vStart);
    }
    scripting_Pop();  // index 1

    scripting_Pop();  // index i
  }
  scripting_Pop();  // boundary

  scripting_Pop();  // level

  computeBoundingBox(l);

  return l;
}
