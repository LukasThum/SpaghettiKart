#include <stdio.h>
#include <libultraship.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
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

int lukas_cpu_initialized = 0;
lua_State *lukas_cpu_lua_state;

struct lukas_cpu_player_proxy {
  void *Player;
};

// should be exactly what happens when human presses a button
void lukas_cpu_accelerate(Player* player) {
  player_accelerate_alternative(player);
}
// should be exactly what happens when human presses b button
void lukas_cpu_decelerate(Player* player) {
  func_800323E4(player);
}

// should be exactly what happens when human uses stick
void lukas_cpu_steer(Player* player, s16 amount) {
  // TODO: extract from player_controller?
  // s32 old_value = player->unk_07C;
  // player->unk_07C = (amount << 16) & 0xFFFF0000;
  // player->unk_0FA = (s16) (old_value - player->unk_07C) >> 16;
  player->rotation[1] += amount * 5;
}

int lukas_cpu_lua_get_player(lua_State *L) {
  struct lukas_cpu_initialized *ud =
    (struct lukas_cpu_player_proxy *) lua_newuserdata (
      L, sizeof (struct lukas_cpu_player_proxy)
  );
  return 1;
}

// provide functino for lua
int lukas_cpu_lua_accelerate(lua_State *L) {
  int playerId = luaL_checknumber(L, 1);
  Player* player = &gPlayers[playerId];
  lukas_cpu_accelerate(player);
  return 0;
}

// run once when started
void lukas_cpu_initialize() {
  if (lukas_cpu_initialized < 1) {
    printf("Lukas CPU started!\n");
    lukas_cpu_lua_state = luaL_newstate();
    luaL_openlibs(lukas_cpu_lua_state);

    lua_pushcfunction(
      lukas_cpu_lua_state,
      lukas_cpu_lua_accelerate
    );
    lua_setglobal(
      lukas_cpu_lua_state,
      "accelerate"
    );
    // lua_pushcfunction(
    //   lukas_cpu_lua_state,
    //   lukas_cpu_decelerate
    // );
    // lua_pushcfunction(
    //   lukas_cpu_lua_state,
    //   lukas_cpu_steer
    // );
    lukas_cpu_initialized = 1;
  }
}

// run once when stopped
void lukas_cpu_terminate() {
  if (lukas_cpu_initialized > 0) {
    printf("Lukas CPU stopped!\n");
    lua_close(lukas_cpu_lua_state);
    lukas_cpu_initialized = 0;
  }
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
    } else {
      // hand over to lua scripts
      lua_pushnumber(
        lukas_cpu_lua_state,
        playerId
      );
      lua_setglobal(
        lukas_cpu_lua_state,
        "playerId"
      );
      if (
        luaL_dofile(
          lukas_cpu_lua_state,
          "../assets/scripts/lukas_cpu/lukas_cpu.lua"
        )
      ) {
        printf(
          "Error: %s\n",
          lua_tostring(lukas_cpu_lua_state, -1)
        );
      }
    }
  }
  if (set_places_interval > set_places_interval_max) {
    set_places();
    set_places_interval = 0;
  }
  set_places_interval++;
}
