#include <stdio.h>
#include <libultraship.h>
#include <defines.h>
#include <macros.h>

#include "lukas_cpu.h"
#include "math_util.h"
#include "common_structs.h"
#include "render_objects.h"
#include "kart_attributes.h"
#include "waypoints.h"
#include "player_controller.h"
#include "code_80005FD0.h"

// frequency to recalculate places (ranks), not needed every tick and player
int set_places_interval_max = 100;
int set_places_interval = 0;

// should be exactly what happens when human presses a button
void lukas_cpu_accelerate(Player* player) {
  player_accelerate_alternative(player);
}

// should be exactly what happens when human presses b button
void lukas_cpu_decelerate(Player* player) {
  func_800323E4(player);
}

// TODO: extract from player_controller?
// should be exactly what happens when human uses stick
void lukas_cpu_steer(Player* player, s16 amount) {
  // s32 old_value = player->unk_07C;
  // player->unk_07C = (amount << 16) & 0xFFFF0000;
  // player->unk_0FA = (s16) (old_value - player->unk_07C) >> 16;
  player->rotation[1] += amount * 5;
}

// attempt to make pathfinding work..
void lukas_cpu_silly(s32 playerId, Player* player) {
  TrackPathPoint checkpoint =
    gTrackPaths
      [gPlayerPathIndex]
      [gNearestPathPointByPlayerId[playerId]];
  Vec3f checkpoint_translated;
  mtxf_translate_vec3f_mat3(
    checkpoint_translated,
    player->orientationMatrix
  );
  // int steer_amount = random_int(20) - 10
  int steer_amount = 0;
  s32 angle_to_checkpoint =
    get_angle_between_two_vectors(
      player->pos, // add orientation vector?
      checkpoint_translated
    );
  // try to steer towards path
  if (angle_to_checkpoint < 0x400) {
    steer_amount = 10;
  } else if (angle_to_checkpoint < 0x800) {
    steer_amount = 10;
  } else if (angle_to_checkpoint < 0x4000) {
    steer_amount = 20;
  } else if (angle_to_checkpoint < 0x8000) {
    steer_amount = 20;
  } else if (angle_to_checkpoint < 0xC000) {
    steer_amount = -10;
  } else if (angle_to_checkpoint < 0xF800) {
    steer_amount = -10;
  } else if (angle_to_checkpoint < 0xFC00) {
    steer_amount = -10;
  } else {
    steer_amount = -10;
  }
  if (player->speed < 0.5) {
    lukas_cpu_accelerate(player);
  }
  lukas_cpu_steer(player, steer_amount);
}

void lukas_cpu_update_player(s32 playerId) {
  Player* player;
  player = &gPlayers[playerId];
  update_player_path_completion(playerId, player);
  if (player->type & PLAYER_HUMAN) {
      detect_wrong_player_direction(playerId, player);
  } else if (player->type & PLAYER_CPU) {
    if (CVarGetInteger("gHaltCPU", 0) == 1) {
      // halt and catch fire
      lukas_cpu_decelerate(player);
      lukas_cpu_steer(player, 0);
    } else if (CVarGetInteger("gSillyCPU", 0) == 1) {
      lukas_cpu_silly(playerId, player);
    } else {
      // TODO: use lua script
    }
  }
  if (set_places_interval > set_places_interval_max) {
    set_places();
    set_places_interval = 0;
  }
  set_places_interval++;
}
