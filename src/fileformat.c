#include "fileformat.h"
#include <stdbool.h>
#include <string.h>

bool ff_load_splitsio_game(ls_game* game, json_t* json, char** error_msg)
{
    return true;
}

bool ff_load_urn_game(ls_game* game, json_t* json, char** error_msg)
{
    json_t* ref;
    
    ref = json_object_get(json, "title");
    if (ref) {
        game->title = strdup(json_string_value(ref));
        if (!game->title) {
            return false;
        }
    }
    // copy theme
    ref = json_object_get(json, "theme");
    if (ref) {
        game->theme = strdup(json_string_value(ref));
        if (!game->theme) {
            return false;
        }
    }
    // copy theme variant
    ref = json_object_get(json, "theme_variant");
    if (ref) {
        game->theme_variant = strdup(json_string_value(ref));
        if (!game->theme_variant) {
            return false;
        }
    }
    // get attempt count
    ref = json_object_get(json, "attempt_count");
    if (ref) {
        game->attempt_count = json_integer_value(ref);
    }
    // get finished count
    ref = json_object_get(json, "finished_count");
    if (ref) {
        game->finished_count = json_integer_value(ref);
    }
    // get width
    ref = json_object_get(json, "width");
    if (ref) {
        game->width = json_integer_value(ref);
    }
    // get height
    ref = json_object_get(json, "height");
    if (ref) {
        game->height = json_integer_value(ref);
    }
    // get delay
    ref = json_object_get(json, "start_delay");
    if (ref) {
        game->start_delay = ls_time_value(
            json_string_value(ref));
    }
    // get wr
    ref = json_object_get(json, "world_record");
    if (ref) {
        game->world_record = strtoll(json_string_value(ref), NULL, 10);
    }
    // get splits
    ref = json_object_get(json, "splits");
    if (ref) {
        game->split_count = json_array_size(ref);
        // allocate titles
        game->split_titles = calloc(game->split_count,
            sizeof(char*));
        if (!game->split_titles) {
            return false;
        }
        // allocate splits
        game->split_times = calloc(game->split_count,
            sizeof(long long));
        if (!game->split_times) {
            return false;;
        }
        game->segment_times = calloc(game->split_count,
            sizeof(long long));
        if (!game->segment_times) {
            return false;
        }
        game->best_splits = calloc(game->split_count,
            sizeof(long long));
        if (!game->best_splits) {
            return false;
        }
        game->best_segments = calloc(game->split_count,
            sizeof(long long));
        if (!game->best_segments) {
            return false;
        }
        // copy splits
        for (int i = 0; i < game->split_count; ++i) {
            json_t* split;
            json_t* split_ref;
            split = json_array_get(ref, i);
            split_ref = json_object_get(split, "title");
            if (split_ref) {
                game->split_titles[i] = strdup(
                    json_string_value(split_ref));
                if (!game->split_titles[i]) {
                    return false;
                }
            }
            split_ref = json_object_get(split, "time");
            if (split_ref) {
                game->split_times[i] = strtoll(json_string_value(split_ref), NULL, 10);
            }
            if (i && game->split_times[i] && game->split_times[i - 1]) {
                game->segment_times[i] = game->split_times[i] - game->split_times[i - 1];
            } else if (!i && game->split_times[0]) {
                game->segment_times[0] = game->split_times[0];
            }
            split_ref = json_object_get(split, "best_time");
            if (split_ref) {
                game->best_splits[i] = strtoll(json_string_value(split_ref), NULL, 10);
            } else if (game->split_times[i]) {
                game->best_splits[i] = game->split_times[i];
            }
            split_ref = json_object_get(split, "best_segment");
            if (split_ref) {
                game->best_segments[i] = strtoll(json_string_value(split_ref), NULL, 10);
            } else if (game->segment_times[i]) {
                game->best_segments[i] = game->segment_times[i];
            }
        }
    }

    return true;
}