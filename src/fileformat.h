#ifndef FILEFORMAT_H
#define FILEFORMAT_H
#include "timer.h"
#include <jansson.h>
#include <stdbool.h>

bool ff_load_splitsio_game(ls_game* game, json_t* json, char** error_msg);

bool ff_load_urn_game(ls_game* game, json_t* json, char** error_msg);

#endif //FILEFORMAT_H
