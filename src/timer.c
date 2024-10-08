#include "timer.h"
#include "fileformat.h"
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

long long ls_time_now(void)
{
    struct timespec timespec;
    clock_gettime(CLOCK_MONOTONIC, &timespec);
    return timespec.tv_sec * 1000000LL + timespec.tv_nsec / 1000;
}

long long ls_time_value(const char* string)
{
    char seconds_part[256];
    double subseconds_part = 0.;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int sign = 1;
    if (!string || !strlen(string)) {
        return 0;
    }
    sscanf(string, "%[^.]%lf", seconds_part, &subseconds_part);
    string = seconds_part;
    if (string[0] == '-') {
        sign = -1;
        ++string;
    }
    switch (sscanf(string, "%d:%d:%d", &hours, &minutes, &seconds)) {
        case 2:
            seconds = minutes;
            minutes = hours;
            hours = 0;
            break;
        case 1:
            seconds = hours;
            minutes = 0;
            hours = 0;
            break;
    }
    return sign * ((hours * 60 * 60 + minutes * 60 + seconds) * 1000000LL + (int)(subseconds_part * 1000000.));
}

static void ls_time_string_format(char* string,
    char* millis,
    long long time,
    int serialized,
    int delta,
    int compact)
{
    char dot_subsecs[256];
    const char* sign = "";
    if (time < 0) {
        time = -time;
        sign = "-";
    } else if (delta) {
        sign = "+";
    }
    int hours = time / (1000000LL * 60 * 60);
    int minutes = (time / (1000000LL * 60)) % 60;
    int seconds = (time / 1000000LL) % 60;
    sprintf(dot_subsecs, ".%06lld", time % 1000000LL);
    if (!serialized) {
        /* Show only a dot and 2 decimal places instead of all 6 */
        dot_subsecs[3] = '\0';
    }
    if (millis) {
        strcpy(millis, &dot_subsecs[1]);
        dot_subsecs[0] = '\0';
    }
    if (hours) {
        if (!compact) {
            sprintf(string, "%s%d:%02d:%02d%s",
                sign, hours, minutes, seconds, dot_subsecs);
        } else {
            sprintf(string, "%s%d:%02d", sign, hours, minutes);
        }
    } else if (minutes) {
        if (!compact) {
            sprintf(string, "%s%d:%02d%s",
                sign, minutes, seconds, dot_subsecs);
        } else {
            sprintf(string, "%s%d:%02d", sign, minutes, seconds);
        }
    } else {
        sprintf(string, "%s%d%s", sign, seconds, dot_subsecs);
    }
}

static void ls_time_string_serialized(char* string,
    long long time)
{
    sprintf(string, "%lld", time);
}

void ls_time_string(char* string, long long time)
{
    ls_time_string_format(string, NULL, time, 0, 0, 0);
}

void ls_time_millis_string(char* seconds, char* millis, long long time)
{
    ls_time_string_format(seconds, millis, time, 0, 0, 0);
}

void ls_split_string(char* string, long long time)
{
    ls_time_string_format(string, NULL, time, 0, 0, 1);
}

void ls_delta_string(char* string, long long time)
{
    ls_time_string_format(string, NULL, time, 0, 1, 1);
}

void ls_game_release(ls_game* game)
{
    if (game->path) {
        free(game->path);
    }
    if (game->title) {
        free(game->title);
    }
    if (game->theme) {
        free(game->theme);
    }
    if (game->theme_variant) {
        free(game->theme_variant);
    }
    if (game->split_titles) {
        for (int i = 0; i < game->split_count; ++i) {
            if (game->split_titles[i]) {
                free(game->split_titles[i]);
            }
        }
        free(game->split_titles);
    }
    if (game->split_times) {
        free(game->split_times);
    }
    if (game->segment_times) {
        free(game->segment_times);
    }
    if (game->best_splits) {
        free(game->best_splits);
    }
    if (game->best_segments) {
        free(game->best_segments);
    }
}

int ls_game_create(ls_game** game_ptr, const char* path)
{
    enum ERRORCODE error;

    json_t* json = 0;
    json_error_t json_error;

    // Allocate the game structure
    ls_game* game = calloc(1, sizeof(ls_game));
    if (!game) {

        error = 1;
        goto game_create_done;
    }

    // copy path to file
    game->path = strdup(path);
    if (!game->path) {
        error = 1;
        goto game_create_done;

    }

    // load json
    json = json_load_file(game->path, 0, &json_error);
    if (!json) {
        error = 1;
        size_t msg_len = snprintf(NULL, 0, "%s (%d:%d)", json_error.text, json_error.line, json_error.column);
        //*error_msg = calloc(msg_len + 1, sizeof(char));
        //sprintf(*error_msg, "%s (%d:%d)", json_error.text, json_error.line, json_error.column);
        goto game_create_done;
    }

    // Detect generic format from splits.io
    const json_t* ref = json_object_get(json, "_schemaVersion");

    if(ref) {
        printf("Detected generic format: %s\n", strdup(json_string_value(ref)));
        error = ff_load_splitsio_game(game, json);
        if(error != NONE) {
            return error;
        };
    } else {
        error = ff_load_urn_game(game, json);
        if(error != NONE) {
            return error;
        };
    }

game_create_done:
    if (error == NONE) {
        *game_ptr = game;
    } else if (game) {
        ls_game_release(game);
    }
    if (json) {
        json_decref(json);
    }
    return error;
}

void ls_game_update_splits(ls_game* game,
    const ls_timer* timer)
{
    if (timer->curr_split) {
        int size;
        if ((timer->split_times[game->split_count - 1]
            && timer->split_times[game->split_count - 1]
                < game->world_record) || game->world_record == 0) {
            game->world_record = timer->split_times[game->split_count - 1];
        }
        size = timer->curr_split * sizeof(long long);

        if (timer->split_times[game->split_count - 1]
            < game->split_times[game->split_count - 1] || game->split_times[game->split_count -1] == 0) {
            memcpy(game->split_times, timer->split_times, size);
        }
        memcpy(game->segment_times, timer->segment_times, size);        
        for (int i = 0; i < game->split_count; ++i) {
            printf("Splits: %d, %lld, %lld\n", i, timer->split_times[i], game->split_times[i]);
            if (timer->split_times[i] < game->best_splits[i] || game->best_splits[i] == 0) {
                game->best_splits[i] = timer->split_times[i];
            }
            if (timer->segment_times[i] < game->best_segments[i] || game->best_segments[i] == 0) {
                game->best_segments[i] = timer->segment_times[i];
            }
        }
    }
}

void ls_game_update_bests(ls_game* game,
    const ls_timer* timer)
{
    if (timer->curr_split) {
        int size;
        size = timer->curr_split * sizeof(long long);
        memcpy(game->best_splits, timer->best_splits, size);
        memcpy(game->best_segments, timer->best_segments, size);
    }
}

int ls_game_save(const ls_game* game)
{
    int result = 0;
    char str[256];
    json_t* json = json_object();
    json_t* splits = json_array();
    int i;
    if (game->title) {
        json_object_set_new(json, "title", json_string(game->title));
    }
    if (game->attempt_count) {
        json_object_set_new(json, "attempt_count",
            json_integer(game->attempt_count));
    }
    if (game->finished_count) {
        json_object_set_new(json, "finished_count",
            json_integer(game->finished_count));
    }
    if (game->world_record) {
        ls_time_string_serialized(str, game->world_record);
        json_object_set_new(json, "world_record", json_string(str));
    }
    if (game->start_delay) {
        ls_time_string_serialized(str, game->start_delay);
        json_object_set_new(json, "start_delay", json_string(str));
    }
    for (i = 0; i < game->split_count; ++i) {
        json_t* split = json_object();
        json_object_set_new(split, "title", json_string(game->split_titles[i]));
        ls_time_string_serialized(str, game->split_times[i]);
        json_object_set_new(split, "time", json_string(str));
        ls_time_string_serialized(str, game->best_splits[i]);
        json_object_set_new(split, "best_time", json_string(str));
        ls_time_string_serialized(str, game->best_segments[i]);
        json_object_set_new(split, "best_segment", json_string(str));
        json_array_append_new(splits, split);
    }
    json_object_set_new(json, "splits", splits);
    if (game->theme) {
        json_object_set_new(json, "theme", json_string(game->theme));
    }
    if (game->theme_variant) {
        json_object_set_new(json, "theme_variant",
            json_string(game->theme_variant));
    }
    if (game->width) {
        json_object_set_new(json, "width", json_integer(game->width));
    }
    if (game->height) {
        json_object_set_new(json, "height", json_integer(game->height));
    }

    result = json_dump_file(json, game->path, JSON_PRESERVE_ORDER | JSON_INDENT(2));
    if (result != 0) {
        printf("Error dumping JSON:\n%s\n", json_dumps(json, JSON_PRESERVE_ORDER | JSON_INDENT(2)));
        printf("Error: '%d'\n", result);
        printf("Path: %s\n", game->path);
    }
    json_decref(json);

    return result;
}

void ls_timer_release(ls_timer* timer)
{
    if (timer->split_times) {
        free(timer->split_times);
    }
    if (timer->split_deltas) {
        free(timer->split_deltas);
    }
    if (timer->segment_times) {
        free(timer->segment_times);
    }
    if (timer->segment_deltas) {
        free(timer->segment_deltas);
    }
    if (timer->split_info) {
        free(timer->split_info);
    }
    if (timer->best_splits) {
        free(timer->best_splits);
    }
    if (timer->best_segments) {
        free(timer->best_segments);
    }
}

static void reset_timer(ls_timer* timer)
{
    int i;
    int size;
    timer->started = 0;
    timer->start_time = 0;
    timer->curr_split = 0;
    timer->time = -timer->game->start_delay;
    size = timer->game->split_count * sizeof(long long);
    memcpy(timer->split_times, timer->game->split_times, size);
    memset(timer->split_deltas, 0, size);
    memcpy(timer->segment_times, timer->game->segment_times, size);
    memset(timer->segment_deltas, 0, size);
    memcpy(timer->best_splits, timer->game->best_splits, size);
    memcpy(timer->best_segments, timer->game->best_segments, size);
    size = timer->game->split_count * sizeof(int);
    memset(timer->split_info, 0, size);
    timer->sum_of_bests = 0;
    for (i = 0; i < timer->game->split_count; ++i) {
        if (timer->best_segments[i]) {
            timer->sum_of_bests += timer->best_segments[i];
        } else if (timer->game->best_segments[i]) {
            timer->sum_of_bests += timer->game->best_segments[i];
        } else {
            timer->sum_of_bests = 0;
            break;
        }
    }
}

int ls_timer_create(ls_timer** timer_ptr, ls_game* game)
{
    int error = 0;
    ls_timer* timer;
    // allocate timer

    timer = calloc(1, sizeof(ls_timer));
    if (!timer) {
        error = 1;
        goto timer_create_done;
    }
    timer->game = game;
    timer->attempt_count = &game->attempt_count;
    timer->finished_count = &game->finished_count;
    // alloc splits
    timer->split_times = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->split_times) {
        error = 1;
        goto timer_create_done;
    }
    timer->split_deltas = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->split_deltas) {
        error = 1;
        goto timer_create_done;
    }
    timer->segment_times = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->segment_times) {
        error = 1;
        goto timer_create_done;
    }
    timer->segment_deltas = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->segment_deltas) {
        error = 1;
        goto timer_create_done;
    }
    timer->best_splits = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->best_splits) {
        error = 1;
        goto timer_create_done;
    }
    timer->best_segments = calloc(timer->game->split_count,
        sizeof(long long));
    if (!timer->best_segments) {
        error = 1;
        goto timer_create_done;
    }
    timer->split_info = calloc(timer->game->split_count,
        sizeof(int));
    if (!timer->split_info) {
        error = 1;
        goto timer_create_done;
    }
    reset_timer(timer);
timer_create_done:
    if (!error) {
        *timer_ptr = timer;
    } else if (timer) {
        ls_timer_release(timer);
    }
    return error;
}

void ls_timer_step(ls_timer* timer, long long now)
{
    timer->now = now;
    if (timer->running) {
        long long delta = timer->now - timer->start_time;
        timer->time += delta; // Accumulate the elapsed time
        if (timer->curr_split < timer->game->split_count) {
            timer->split_times[timer->curr_split] = timer->time;
            // calc delta
            if (timer->game->split_times[timer->curr_split]) {
                timer->split_deltas[timer->curr_split] = timer->split_times[timer->curr_split]
                    - timer->game->split_times[timer->curr_split];
            }
            // check for behind time
            if (timer->split_deltas[timer->curr_split] > 0) {
                timer->split_info[timer->curr_split] |= LS_INFO_BEHIND_TIME;
            } else {
                timer->split_info[timer->curr_split] &= ~LS_INFO_BEHIND_TIME;
            }
            if (!timer->curr_split || timer->split_times[timer->curr_split - 1]) {
                // calc segment time and delta
                timer->segment_times[timer->curr_split] = timer->split_times[timer->curr_split];
                if (timer->curr_split) {
                    timer->segment_times[timer->curr_split] -= timer->split_times[timer->curr_split - 1];
                }
                if (timer->game->segment_times[timer->curr_split]) {
                    timer->segment_deltas[timer->curr_split] = timer->segment_times[timer->curr_split]
                        - timer->game->segment_times[timer->curr_split];
                }
            }
            // check for losing time
            if (timer->curr_split) {
                if (timer->split_deltas[timer->curr_split]
                    > timer->split_deltas[timer->curr_split - 1]) {
                    timer->split_info[timer->curr_split]
                        |= LS_INFO_LOSING_TIME;
                } else {
                    timer->split_info[timer->curr_split]
                        &= ~LS_INFO_LOSING_TIME;
                }
            } else if (timer->split_deltas[timer->curr_split] > 0) {
                timer->split_info[timer->curr_split]
                    |= LS_INFO_LOSING_TIME;
            } else {
                timer->split_info[timer->curr_split]
                    &= ~LS_INFO_LOSING_TIME;
            }
        }
    }
    timer->start_time = now; // Update the start time for the next iteration
}

int ls_timer_start(ls_timer* timer)
{
    if (timer->curr_split < timer->game->split_count) {
        if (!timer->started) {
            ++*timer->attempt_count;
            timer->started = 1;
        }
        timer->running = 1;
    }
    return timer->running;
}

int ls_timer_split(ls_timer* timer)
{

    if (timer->running && timer->time > 0) {
        if (timer->curr_split < timer->game->split_count) {
            int i;
            // check for best split and segment
            if (!timer->best_splits[timer->curr_split]
                || timer->split_times[timer->curr_split]
                    < timer->best_splits[timer->curr_split]) {
                timer->best_splits[timer->curr_split] = timer->split_times[timer->curr_split];
                timer->split_info[timer->curr_split]
                    |= LS_INFO_BEST_SPLIT;
            }
            if (!timer->best_segments[timer->curr_split]
                || timer->segment_times[timer->curr_split]
                    < timer->best_segments[timer->curr_split]) {
                timer->best_segments[timer->curr_split] = timer->segment_times[timer->curr_split];
                timer->split_info[timer->curr_split]
                    |= LS_INFO_BEST_SEGMENT;
            }
            // update sum of bests
            timer->sum_of_bests = 0;
            for (i = 0; i < timer->game->split_count; ++i) {
                if (timer->best_segments[i]) {
                    timer->sum_of_bests += timer->best_segments[i];
                } else if (timer->game->best_segments[i]) {
                    timer->sum_of_bests += timer->game->best_segments[i];
                } else {
                    timer->sum_of_bests = 0;
                    break;
                }
            }
            ++timer->curr_split;
            // stop timer if last split
            if (timer->curr_split == timer->game->split_count) {
                // Increment finished_count
                ++*timer->finished_count;
                ls_timer_stop(timer);
                ls_game_update_splits((ls_game*)timer->game, timer);
            }
            return timer->curr_split;
        }
    }
    return 0;
}

int ls_timer_skip(ls_timer* timer)
{
    if (timer->running && timer->time > 0) {
        if (timer->curr_split < timer->game->split_count) {
            timer->split_times[timer->curr_split] = 0;
            timer->split_deltas[timer->curr_split] = 0;
            timer->split_info[timer->curr_split] = 0;
            timer->segment_times[timer->curr_split] = 0;
            timer->segment_deltas[timer->curr_split] = 0;
            return ++timer->curr_split;
        }
    }
    return 0;
}

int ls_timer_unsplit(ls_timer* timer)
{
    if (timer->curr_split) {
        int i;
        int curr = --timer->curr_split;
        for (i = curr; i < timer->game->split_count; ++i) {
            timer->split_times[i] = timer->game->split_times[i];
            timer->split_deltas[i] = 0;
            timer->split_info[i] = 0;
            timer->segment_times[i] = timer->game->segment_times[i];
            timer->segment_deltas[i] = 0;
        }
        if (timer->curr_split + 1 == timer->game->split_count) {
            timer->running = 1;
        }
        return timer->curr_split;
    }
    return 0;
}

void ls_timer_stop(ls_timer* timer)
{
    timer->running = 0;
}

int ls_timer_reset(ls_timer* timer)
{
    if (!timer->running) {
        if (timer->started && timer->time <= 0) {
            return ls_timer_cancel(timer);
        }
        reset_timer(timer);
        return 1;
    }
    return 0;
}

int ls_timer_cancel(ls_timer* timer)
{
    if (!timer->running) {
        if (timer->started) {
            if (*timer->attempt_count > 0) {
                --*timer->attempt_count;
            }
        }
        reset_timer(timer);
        return 1;
    }
    return 0;
}
