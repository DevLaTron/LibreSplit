#ifndef FILEFORMAT_H
#define FILEFORMAT_H
#include "errors.h"
#include "timer.h"
#include <jansson.h>
#include <stdbool.h>

enum ERRORCODE ff_load_splitsio_game(ls_game* game, json_t* json);

enum ERRORCODE ff_load_urn_game(ls_game* game, json_t* json);

#endif //FILEFORMAT_H
