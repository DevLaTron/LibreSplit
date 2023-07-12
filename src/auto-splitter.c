#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "headers/memory.h"
#include "headers/auto-splitter.h"
#include "headers/process.h"

#define MAX_PATH_LENGTH 256

char auto_splitter_file[MAX_PATH_LENGTH];
int refresh_rate = 60;
atomic_bool auto_splitter_enabled = true;
atomic_bool call_start = false;
atomic_bool call_split = false;
atomic_bool toggle_loading = false;
atomic_bool call_reset = false;
bool prev_is_loading;

extern last_process process;

void check_directories()
{
    // Get the path to the user's directory
    struct passwd *pw = getpwuid(getuid());
    const char *user_directory = pw->pw_dir;
    char last_directory[241];
    char auto_splitters_directory[MAX_PATH_LENGTH];
    char themes_directory[MAX_PATH_LENGTH];
    char splits_directory[MAX_PATH_LENGTH];
    snprintf(last_directory, MAX_PATH_LENGTH, "%s/.last", user_directory);
    snprintf(auto_splitters_directory, MAX_PATH_LENGTH, "%s/auto-splitters", last_directory);
    snprintf(themes_directory, MAX_PATH_LENGTH, "%s/themes", last_directory);
    snprintf(splits_directory, MAX_PATH_LENGTH, "%s/splits", last_directory);

    // Make the LAST directory if it doesn't exist
    if (mkdir(last_directory, 0755) == -1) {
        // Directory already exists or there was an error
    }

    // Make the autosplitters directory if it doesn't exist
    if (mkdir(auto_splitters_directory, 0755) == -1) {
        // Directory already exists or there was an error
    }

    // Make the themes directory if it doesn't exist
    if (mkdir(themes_directory, 0755) == -1) {
        // Directory already exists or there was an error
    }

    // Make the splits directory if it doesn't exist
    if (mkdir(splits_directory, 0755) == -1) {
        // Directory already exists or there was an error
    }
}

void startup(lua_State* L)
{
    lua_getglobal(L, "startup");
    lua_pcall(L, 0, 0, 0);

    lua_getglobal(L, "refreshRate");
    if (lua_isnumber(L, -1))
    {
        refresh_rate = lua_tointeger(L, -1);
    }
    lua_pop(L, 1); // Remove 'refreshRate' from the stack
}

void state(lua_State* L)
{
    lua_getglobal(L, "state");
    lua_pcall(L, 0, 0, 0);
}

void update(lua_State* L)
{
    lua_getglobal(L, "update");
    lua_pcall(L, 0, 0, 0);
}

void start(lua_State* L)
{
    lua_getglobal(L, "start");
    lua_pcall(L, 0, 1, 0);
    if (lua_toboolean(L, -1))
    {
        atomic_store(&call_start, true);
    }
    lua_pop(L, 1); // Remove the return value from the stack
}

void split(lua_State* L)
{
    lua_getglobal(L, "split");
    lua_pcall(L, 0, 1, 0);
    if (lua_toboolean(L, -1))
    {
        atomic_store(&call_split, true);
    }
    lua_pop(L, 1); // Remove the return value from the stack
}

void is_loading(lua_State* L)
{
    lua_getglobal(L, "isLoading");
    lua_pcall(L, 0, 1, 0);
    if (lua_toboolean(L, -1) != prev_is_loading)
    {
        atomic_store(&toggle_loading, true);
        prev_is_loading = !prev_is_loading;
    }
    lua_pop(L, 1); // Remove the return value from the stack
}

void reset(lua_State* L)
{
    lua_getglobal(L, "reset");
    lua_pcall(L, 0, 1, 0);
    if (lua_toboolean(L, -1))
    {
        atomic_store(&call_reset, true);
    }
    lua_pop(L, 1); // Remove the return value from the stack
}

void run_auto_splitter()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    lua_pushcfunction(L, find_process_id);
    lua_setglobal(L, "process");
    lua_pushcfunction(L, read_address);
    lua_setglobal(L, "readAddress");

    char current_file[MAX_PATH_LENGTH];
    strcpy(current_file, auto_splitter_file);

    // Load the Lua file
    if (luaL_loadfile(L, auto_splitter_file) != LUA_OK)
    {
        // Error loading the file
        const char* error_msg = lua_tostring(L, -1);
        lua_pop(L, 1); // Remove the error message from the stack
        fprintf(stderr, "Lua syntax error: %s\n", error_msg);
        lua_close(L);
        return;
    }

    // Execute the Lua file
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK)
    {
        // Error executing the file
        const char* error_msg = lua_tostring(L, -1);
        lua_pop(L, 1); // Remove the error message from the stack
        fprintf(stderr, "Lua runtime error: %s\n", error_msg);
        lua_close(L);
        return;
    }

    lua_getglobal(L, "state");
    bool state_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'state' from the stack

    lua_getglobal(L, "start");
    bool start_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'start' from the stack

    lua_getglobal(L, "split");
    bool split_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'split' from the stack

    lua_getglobal(L, "isLoading");
    bool is_loading_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'isLoading' from the stack

    lua_getglobal(L, "startup");
    bool startup_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'startup' from the stack

    lua_getglobal(L, "reset");
    bool reset_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'reset' from the stack

    lua_getglobal(L, "update");
    bool update_exists = lua_isfunction(L, -1);
    lua_pop(L, 1); // Remove 'update' from the stack

    if (startup_exists)
    {
        startup(L);
    }

    if (state_exists)
    {
        state(L);
    }

    printf("Refresh rate: %d\n", refresh_rate);
    int rate = 1000000 / refresh_rate;

    while (1)
    {
        struct timespec clock_start;
        clock_gettime(CLOCK_MONOTONIC, &clock_start);

        if (!auto_splitter_enabled || strcmp(current_file, auto_splitter_file) != 0 || !process_exists() || process.pid == 0)
        {
            break;
        }

        if (state_exists)
        {
            state(L);
        }

        if (update_exists)
        {
            update(L);
        }

        if (start_exists)
        {
            start(L);
        }

        if (split_exists)
        {
            split(L);
        }

        if (is_loading_exists)
        {
            is_loading(L);
        }

        if (reset_exists)
        {
            reset(L);
        }

        struct timespec clock_end;
        clock_gettime(CLOCK_MONOTONIC, &clock_end);
        long long duration = (clock_end.tv_sec - clock_start.tv_sec) * 1000000 + (clock_end.tv_nsec - clock_start.tv_nsec) / 1000;
        if (duration < rate)
        {
            usleep(rate - duration);
        }
    }

    lua_close(L);
}

void *last_auto_splitter()
{
    while (true)
    {
        if (atomic_load(&auto_splitter_enabled) && auto_splitter_file[0] != '\0')
        {
            run_auto_splitter();
        }
        usleep(1000000); // Wait for 1 second before checking again
    }
}