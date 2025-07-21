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

char* player_state[16] = {
  "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}",
  "{}", "{}", "{}", "{}", "{}", "{}", "{}", "{}"
};

// frequency to recalculate places (ranks), not needed every tick and player
int places_interval_max = 100;
int places_interval = 0;
int initialized = 0;
int lastPlayerId = 0;
int firstRun = 1;

lua_State * lua_state;
char* lua_start_file = "../assets/scripts/lukas_cpu/on_start.lua";
char* lua_file = "../assets/scripts/lukas_cpu/test_001.lua";
char* lua_end_file = "../assets/scripts/lukas_cpu/on_end.lua";
char* lua_code = "print('no code to run!')";
int lua_file_reload_interval = 0;
int lua_file_reload_interval_max = 1000;

char* concat(const char *s1, const char *s2)
{
  char *result = malloc(strlen(s1) + strlen(s2) + 1);
  strcpy(result, s1);
  strcat(result, s2);
  return result;
}

char* lukas_cpu_read_file(char* filename) {
  char* result = "";
  FILE* file = fopen(filename, "r");
  if (file == NULL) {
    exit(EXIT_FAILURE);
  }
  char* line = NULL;
  size_t length = 0;
  ssize_t read;
  while ((read = getline(&line, &length, file)) != -1) {
    result = concat(result, line);
  }
  fclose(file);
  return result;
}

void load_code() {
  lua_code = concat(concat(
    lukas_cpu_read_file(lua_start_file),
    lukas_cpu_read_file(lua_file)),
    lukas_cpu_read_file(lua_end_file)
  );
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
void steer(Player* player, f32 amount) {
  f32 amount_clamped = amount;
  if (amount < -1.0) {
    amount_clamped = -1.0;
  }
  if (amount > 1.0) {
    amount_clamped = 1.0;
  }
  player->unk_07C = round(3475000.0 * amount_clamped);
}
// provide function for lua
int lua_steer(lua_State *state) {
  int playerId = luaL_checknumber(state, 1);
  float amount = luaL_checknumber(state, 2);
  Player* player = &gPlayers[playerId];
  steer(player, amount);
  return 0;
}

int lua_get_player(lua_State *state) {
  int playerId = lua_tonumber(state, 1);
  Player* player = &gPlayers[playerId];
  lua_createtable(state, 0, 17);
  add_field_boolean(state, "isHuman", player->type & PLAYER_HUMAN);
  add_field_string(state, "name", character_names[player->characterId]);
  add_field_integer(state, "id", playerId);
  add_field_integer(state, "speed", (int) (player->speed * 12.5));
  add_field_number(state, "rank", player->currentRank);
  add_field_number(state, "laps", player->lapCount);
  add_field_number(state, "locX", player->pos[0]);
  add_field_number(state, "locY", player->pos[1]);
  add_field_number(state, "locZ", player->pos[2]);
  add_field_number(state, "topSpeed", player->topSpeed);
  add_field_number(state, "velX", player->velocity[0]);
  add_field_number(state, "velY", player->velocity[1]);
  add_field_number(state, "velZ", player->velocity[2]);
  add_field_number(state, "rotX", player->rotation[0]);
  add_field_number(state, "rotY", player->rotation[1]);
  add_field_number(state, "rotZ", player->rotation[2]);
  add_field_number(state, "propulsion", player->kartPropulsionStrength);
  add_field_integer(state, "driftCombos", player->driftState);
  add_field_number(state, "steering", player->unk_07C);

  // add_field_number(state, "effects", player->effects);
  // add_field_number(state, "currentSpeed", player->currentSpeed);
  return 1;
}

int lua_get_player_state(lua_State *state) {
  int playerId = lua_tonumber(state, 1);
  char* pstate = player_state[playerId];
  lua_pushstring(lua_state, pstate);
  return 1;
}
int lua_set_player_state(lua_State *state) {
  int playerId = lua_tonumber(state, 1);
  char* pstate = lua_tostring(state, 2);
  player_state[playerId] = malloc(strlen(pstate) + 1);
  strcpy(player_state[playerId], pstate);
  return 0;
}

int lua_get_next_checkpoint(lua_State *state) {
  int playerId = lua_tonumber(state, 1);
  TrackPathPoint* checkpoint =
    &gTrackPaths
      [gPlayerPathIndex]
      [gNearestPathPointByPlayerId[playerId]];
  lua_createtable(state, 0, 1);
  add_field_integer(state, "id", checkpoint->trackSectionId);
  add_field_integer(state, "locX", checkpoint->posX);
  add_field_integer(state, "locY", checkpoint->posY);
  add_field_integer(state, "locZ", checkpoint->posZ);
  return 1;
}

// run once when started
void lukas_cpu_initialize() {
  if (initialized < 1) {
    printf("Lukas CPU started!\n");
    firstRun = 1;
    load_code();
    lua_state = luaL_newstate();
    luaL_openlibs(lua_state);

    lua_pushcfunction(lua_state, lua_accelerate);
    lua_setglobal(lua_state, "accelerate");

    lua_pushcfunction(lua_state, lua_decelerate);
    lua_setglobal(lua_state, "decelerate");

    lua_pushcfunction(lua_state, lua_steer);
    lua_setglobal(lua_state, "steer");

    lua_pushcfunction(lua_state, lua_get_player);
    lua_setglobal(lua_state, "getPlayerRaw");

    lua_pushcfunction(lua_state, lua_get_player_state);
    lua_setglobal(lua_state, "getPlayerState");

    lua_pushcfunction(lua_state, lua_set_player_state);
    lua_setglobal(lua_state, "setPlayerState");

    lua_pushcfunction(lua_state, lua_get_next_checkpoint);
    lua_setglobal(lua_state, "getNextCheckpointRaw");

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
  if (lastPlayerId > playerId) {
    firstRun = 0;
  }
  update_player_path_completion(playerId, player);
  if (player->type & PLAYER_HUMAN) {
    detect_wrong_player_direction(playerId, player);
    // if (rand() % 100 < 1) { }
  } else if (player->type & PLAYER_CPU) {
    if (CVarGetInteger("gHaltCPU", 0) == 1) {
      // halt and catch fire
      decelerate(player);
      steer(player, 0);
      return;
    }
  }
  // reload file ever so often
  lua_file_reload_interval++;
  if (lua_file_reload_interval > lua_file_reload_interval_max) {
    load_code();
    lua_file_reload_interval = 0;
  }
  // hand over to lua scripts
  lua_pushnumber(lua_state, playerId);
  lua_setglobal(lua_state, "playerId");
  lua_pushboolean(lua_state, firstRun);
  lua_setglobal(lua_state, "firstRun");
  if (luaL_dostring(lua_state, lua_code)) {
    printf("Error: %s\n", lua_tostring(lua_state, -1));
  }
  // calculate new ranks ever so ofter
  if (places_interval > places_interval_max) {
    set_places();
    places_interval = 0;
  }
  places_interval++;
  lastPlayerId = playerId;
}
