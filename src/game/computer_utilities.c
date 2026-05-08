#include "base/nebu_math.h"
#include "game/game.h"
#include "game/game_data.h"
#include "game/ai.h"
#include "configuration/settings.h"
#include "game/game_level.h"
#include "game/event.h"

void ai_getClosestOpponent(int player, int* opponent, float* distance) {
  int i;
  vec2 v_player;
  vec2 v_opponent;

  *opponent = -1;
  *distance = FLT_MAX;

  getPositionFromIndex(&v_player.X, &v_player.Y, player);

  for (i = 0; i < game->players; i++) {
    if (i == player) continue;
    if (game->player[i].data->speed > 0) {
      float d;

      getPositionFromIndex(&v_opponent.X, &v_opponent.Y, i);
      vec2 diff = HMM_SubV2(v_player, v_opponent);
      // use manhattan distance instead of euclidean distance
      d = (float)(fabs(diff.X) + fabs(diff.Y));
      if (d < *distance) {
        *opponent = i;
        *distance = d;
      }
    }
  }
}

void ai_getDistances(int player, AI_Distances* distances) {
  enum { eFront = 0, eLeft, eRight, eBackleft, eMax };
  segment2 segments[eMax];
  vec2 v, vPos;
  Data* data = game->player[player].data;
  int dirLeft = (data->dir + 3) % 4;
  int dirRight = (data->dir + 1) % 4;
  int i, j;
  float* front = &distances->front;
  float* right = &distances->right;
  float* left = &distances->left;
  float* backleft = &distances->backleft;

  getPositionFromIndex(&vPos.X, &vPos.Y, player);

  for (i = 0; i < eMax; i++) {
    segments[i].vStart = vPos;
  }

  segments[eFront].vDirection = HMM_V2((float)dirsX[data->dir],
                                       (float)dirsY[data->dir]);
  segments[eLeft].vDirection = HMM_V2((float)dirsX[dirLeft],
                                      (float)dirsY[dirLeft]);
  segments[eRight].vDirection = HMM_V2((float)dirsX[dirRight],
                                       (float)dirsY[dirRight]);
  segments[eBackleft].vDirection =
      HMM_NormV2(HMM_V2((float)dirsX[dirLeft] - dirsX[data->dir],
                        (float)dirsY[dirLeft] - dirsY[data->dir]));
  *front = FLT_MAX;
  *left = FLT_MAX;
  *right = FLT_MAX;
  *backleft = FLT_MAX;

  // loop over all segment
  for (i = 0; i < game->players; i++) {
    segment2* wall = game->player[i].data->trails;
    if (game->player[i].data->trail_height < TRAIL_HEIGHT) continue;

    for (j = 0; j < game->player[i].data->trailOffset + 1; j++) {
      float t1, t2;
      if (i == player && j == game->player[i].data->trailOffset) break;
      if (segment2_Intersect(&v, &t1, &t2, segments + eFront, wall) && t1 > 0 &&
          t1 < *front && t2 >= 0 && t2 <= 1)
        *front = t1;
      if (segment2_Intersect(&v, &t1, &t2, segments + eLeft, wall) && t1 > 0 &&
          t1 < *left && t2 >= 0 && t2 <= 1)
        *left = t1;
      if (segment2_Intersect(&v, &t1, &t2, segments + eRight, wall) && t1 > 0 &&
          t1 < *right && t2 >= 0 && t2 <= 1)
        *right = t1;
      if (segment2_Intersect(&v, &t1, &t2, segments + eBackleft, wall) &&
          t1 > 0 && t1 < *backleft && t2 >= 0 && t2 <= 1)
        *backleft = t1;
      wall++;
    }
  }
  for (i = 0; i < game2->level->nBoundaries; i++) {
    float t1, t2;
    segment2* wall = game2->level->boundaries + i;
    if (segment2_Intersect(&v, &t1, &t2, segments + eFront, wall) && t1 > 0 &&
        t1 < *front && t2 >= 0 && t2 <= 1)
      *front = t1;
    if (segment2_Intersect(&v, &t1, &t2, segments + eLeft, wall) && t1 > 0 &&
        t1 < *left && t2 >= 0 && t2 <= 1)
      *left = t1;
    if (segment2_Intersect(&v, &t1, &t2, segments + eRight, wall) && t1 > 0 &&
        t1 < *right && t2 >= 0 && t2 <= 1)
      *right = t1;
    if (segment2_Intersect(&v, &t1, &t2, segments + eBackleft, wall) &&
        t1 > 0 && t1 < *backleft && t2 >= 0 && t2 <= 1)
      *backleft = t1;
  }

  // update debug render segments
  {
    AI* ai = game->player[player].ai;
    ai->front.vStart = vPos;
    ai->left.vStart = vPos;
    ai->right.vStart = vPos;
    ai->backleft.vStart = vPos;

    ai->front.vDirection =
        HMM_V2(*front * dirsX[data->dir], *front * dirsY[data->dir]);
    ai->left.vDirection =
        HMM_V2(*left * dirsX[dirLeft], *left * dirsY[dirLeft]);
    ai->right.vDirection =
        HMM_V2(*right * dirsX[dirRight], *right * dirsY[dirRight]);
    ai->backleft.vDirection = HMM_MulV2F(
        HMM_NormV2(HMM_V2((float)(dirsX[dirLeft] - dirsX[data->dir]),
                          (float)(dirsY[dirLeft] - dirsY[data->dir]))),
        *backleft);
  }

  // printf("%.2f, %.2f, %.2f\n", *front, *right, *left);
  return;
}

void ai_getConfig(int player, int target, AI_Configuration* config) {
  Data* data;

  getPositionFromIndex(&config->player.vStart.X, &config->player.vStart.Y,
                       player);
  getPositionFromIndex(&config->opponent.vStart.X,
                       &config->opponent.vStart.Y, target);

  data = game->player[player].data;
  config->player.vDirection =
      HMM_V2(dirsX[data->dir] * data->speed, dirsY[data->dir] * data->speed);

  data = game->player[target].data;
  config->opponent.vDirection =
      HMM_V2(dirsX[data->dir] * data->speed, dirsY[data->dir] * data->speed);

  // compute sector
  {
    vec3 up = HMM_V3(0, 0, 1);
    float cosphi;
    float phi;
    int i;

    vec2 diff = HMM_SubV2(config->player.vStart, config->opponent.vStart);
    vec3 v1 = HMM_NormV3(HMM_V3(diff.X, diff.Y, 0));
    vec3 v2 = HMM_NormV3(HMM_V3(config->opponent.vDirection.X,
                                config->opponent.vDirection.Y, 0));
    vec3 v3 = HMM_NormV3(HMM_Cross(v1, v2));

    cosphi = HMM_DotV3(v1, v2);
    nebu_Clamp(&cosphi, -1, 1);
    phi = (float)acos(cosphi);
    if (HMM_DotV3(v3, up) > 0) phi = 2 * (float)M_PI - phi;

    for (i = 0; i < 8; i++) {
      phi -= (float)M_PI / 4;
      if (phi < 0) {
        config->location = i;
        break;
      }
    }
  }
  // compute intersection
  {
    segment2 seg1;
    segment2 seg2;
    seg1.vStart = config->opponent.vStart;
    seg1.vDirection = config->opponent.vDirection;
    seg2.vStart = config->player.vStart;
    seg2.vDirection = HMM_MulV2F(
        HMM_NormV2(vec2_Orthogonal(config->opponent.vDirection)),
        HMM_LenV2(config->player.vDirection));

    segment2_Intersect(&config->intersection, &config->t_opponent,
                       &config->t_player, &seg1, &seg2);
    if (config->t_player < 0) config->t_player *= -1;
  }
}

void ai_left(int player, AI_Distances* distances,
             AI_Parameters* pAIParameters) {
  // printf("trying left turn...");
  AI* ai = game->player[player].ai;
  Data* data = game->player[player].data;
  int level = gSettingsCache.ai_level;

  float save_distance =
      (pAIParameters->minTurnTime[level] * data->speed / 1000.0f) + 20;

  if (distances->left > save_distance) {
    createEvent(player, EVENT_TURN_LEFT);
    ai->tdiff++;
    ai->lasttime = game2->time.current;
    // printf("succeeded\n");
  } else {
    // printf("failed\n");
  }
}

void ai_right(int player, AI_Distances* distances,
              AI_Parameters* pAIParameters) {
  // printf("trying right turn...");
  AI* ai = game->player[player].ai;
  Data* data = game->player[player].data;
  int level = gSettingsCache.ai_level;

  float save_distance =
      (pAIParameters->minTurnTime[level] * data->speed / 1000.0f) + 20;

  if (distances->right > save_distance) {
    createEvent(player, EVENT_TURN_RIGHT);
    ai->tdiff--;
    ai->lasttime = game2->time.current;
    // printf("succeeded\n");
  } else {
    // printf("failed\n");
  }
}

static int agressive_action[8][4] = {{2, 0, 2, 2}, {0, 1, 1, 2}, {0, 1, 1, 2},
                                     {0, 1, 1, 2}, {0, 2, 2, 1}, {0, 2, 2, 1},
                                     {0, 2, 2, 1}, {1, 1, 1, 0}};

int evasive_action[8][4] = {{1, 1, 2, 2}, {1, 1, 2, 0}, {1, 1, 2, 0},
                            {1, 1, 2, 0}, {2, 0, 1, 1}, {2, 0, 1, 1},
                            {2, 0, 1, 1}, {2, 2, 1, 1}};

void ai_action(int action, int player, AI_Distances* distances,
               AI_Parameters* pAIParameters) {
  switch (action) {
    case 0:
      break;
    case 1:
      ai_left(player, distances, pAIParameters);
      break;
    case 2:
      ai_right(player, distances, pAIParameters);
      break;
  }
}

void ai_aggressive(int player, int target, int location,
                   AI_Distances* distances, AI_Parameters* pAIParameters) {
  int dirdiff =
      (4 + game->player[player].data->dir - game->player[target].data->dir) % 4;

  // printf("aggressive mode (%d, %d)\n", player, target, location, dirdiff);

  ai_action(agressive_action[location][dirdiff], player, distances,
            pAIParameters);
}

void ai_evasive(int player, int target, int location, AI_Distances* distances,
                AI_Parameters* pAIParameters) {
  int dirdiff =
      (4 + game->player[player].data->dir - game->player[target].data->dir) % 4;
  // printf("evasive mode (%d,%d,%d)\n", player, target, location);

  ai_action(evasive_action[location][dirdiff], player, distances,
            pAIParameters);
}
