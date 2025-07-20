#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

char* character_names[8] = {
  "Mario",
  "Luigi",
  "Yoshi",
  "Toad",
  "Donkey Kong",
  "Wario",
  "Peach",
  "Bowser"
};

// frequency to recalculate places (ranks), not needed every tick and player
int places_interval_max = 100;
int places_interval = 0;
int initialized = 0;
lua_State * lua_state;
char* lua_code = "";

char* concat(const char *s1, const char *s2)
{
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

void load_code(char* filename) {
  printf("load cpu script: %s\n", filename);
  lua_code = "";
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    exit(EXIT_FAILURE);
  }
  char* line = NULL;
  size_t length = 0;
  ssize_t read;
  while ((read = getline(&line, &length, file)) != -1) {
    lua_code = concat(lua_code, line);
  }
  printf("%s\n", lua_code);
  fclose(file);
}

void add_field_boolean(lua_State *state, char* name, int boolean) {
  lua_pushboolean(state, boolean);
  lua_setfield(state, -2, name);
}
void add_field_integer(lua_State *state, char* name, int integer) {
  lua_pushinteger(state, integer);
  lua_setfield(state, -2, name);
}
void add_field_number(lua_State *state, char* name, double number) {
  lua_pushnumber(state, number);
  lua_setfield(state, -2, name);
}
void add_field_string(lua_State *state, char* name, char* string) {
  lua_pushlstring(state, string, strlen(string));
  lua_setfield(state, -2, name);
}

// should be exactly what happens when human presses a button
void accelerate(Player* player) {
  player_accelerate_alternative(player);
}
// provide function for lua
int lua_accelerate(lua_State *state) {
  int playerId = luaL_checknumber(state, 1);
  Player* player = &gPlayers[playerId];
  accelerate(player);
  return 0;
}

// should be exactly what happens when human presses b button
void decelerate(Player* player) {
  func_800323E4(player);
}
// provide function for lua
int lua_decelerate(lua_State *state) {
  int playerId = luaL_checknumber(state, 1);
  Player* player = &gPlayers[playerId];
  decelerate(player);
  return 0;
}

// should be exactly what happens when human uses stick
void steer(Player* player, s16 amount) {
  // TODO: extract from player_controller?
  // s32 old_value = player->unk_07C;
  // player->unk_07C = (amount << 16) & 0xFFFF0000;
  // player->unk_0FA = (s16) (old_value - player->unk_07C) >> 16;
  player->rotation[1] += amount * 5;
}
// provide function for lua
int lua_steer(lua_State *state) {
  int playerId = luaL_checknumber(state, 1);
  int amount = luaL_checknumber(state, 2);
  Player* player = &gPlayers[playerId];
  steer(player, amount);
  return 0;
}

int lua_get_player(lua_State *state) {
  int playerId = lua_tonumber(state, 1);
  Player* player = &gPlayers[playerId];

  lua_createtable(state, 0, 9);

  add_field_boolean(state, "isHuman", player->type & PLAYER_HUMAN);
  add_field_string(state, "name", character_names[player->characterId]);
  add_field_integer(state, "id", playerId);
  add_field_number(state, "speed", player->speed); // add some factor?
  add_field_number(state, "rank", player->currentRank);
  add_field_number(state, "laps", player->lapCount);
  add_field_number(state, "locX", player->pos[0]);
  add_field_number(state, "locY", player->pos[1]);
  add_field_number(state, "locZ", player->pos[2]);
  add_field_number(state, "topSpeed", player->topSpeed);

  // add_field_number(state, "velX", player->velocity[0]);
  // add_field_number(state, "velY", player->velocity[1]);
  // add_field_number(state, "velZ", player->velocity[2]);
  // add_field_number(state, "rotX", player->rotation[0]);
  // add_field_number(state, "rotY", player->rotation[1]);
  // add_field_number(state, "rotZ", player->rotation[2]);
  // add_field_number(state, "propulsion", player->kartPropulsionStrength);

  // add_field_number(state, "effects", player->effects);
  // add_field_number(state, "currentSpeed", player->currentSpeed);
  return 1;
}

// run once when started
void lukas_cpu_initialize() {
  if (initialized < 1) {
    printf("Lukas CPU started!\n");
    char* script = "../assets/scripts/lukas_cpu/lukas_cpu.lua";
    load_code(script);
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);
    lua_pushcfunction(lua_state, lua_get_player);
    lua_setglobal(lua_state, "getPlayer");
    lua_pushcfunction(lua_state, lua_accelerate);
    lua_setglobal(lua_state, "accelerate");
    lua_pushcfunction(lua_state, decelerate);
    lua_setglobal(lua_state, "decelerate");
    lua_pushcfunction(lua_state, steer);
    lua_setglobal(lua_state, "steer");
    initialized = 1;
  }
}

// run once when stopped
void lukas_cpu_terminate() {
  if (initialized > 0) {
    printf("Lukas CPU stopped!\n");
    lua_close(lua_state);
    initialized = 0;
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
      decelerate(player);
      steer(player, 0);
    } else {
      // hand over to lua scripts
      lua_pushnumber(lua_state, playerId);
      lua_setglobal(lua_state, "playerId");
      if (luaL_dostring(lua_state, lua_code)) {
        printf("Error: %s\n", lua_tostring(lua_state, -1));
      }
    }
  }
  if (places_interval > places_interval_max) {
    set_places();
    places_interval = 0;
  }
  places_interval++;
}
