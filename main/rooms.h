#ifndef ROOMS_H
#define ROOMS_H

#include <stdint.h>
#include <stdbool.h>
#include "items.h"

#define MAX_ROOMS        16
#define MAX_EXITS        6
#define MAX_ROOM_NAME    48
#define MAX_ROOM_DESC    512
#define MAX_EXIT_NAME    12

typedef struct {
    int from_room;
    int to_room;
    const char *direction;
} exit_t;

typedef struct {
    int id;
    const char *name;
    const char *description;
    int items[MAX_ITEMS];
    int item_count;
    exit_t exits[MAX_EXITS];
    int exit_count;
} room_t;

extern room_t rooms[MAX_ROOMS];

#define ROOM_FOREST_PATH        0
#define ROOM_BERRY_BUSHES       1
#define ROOM_CREEK_CROSSING     2
#define ROOM_ABANDONED_CANOE    3
#define ROOM_WOLF_FRONT_YARD    4
#define ROOM_WOLF_KITCHEN       5
#define ROOM_WOLF_LIVING_ROOM   6
#define ROOM_TRASH_STORAGE      7
#define ROOM_OREGON_DOCK        8
#define ROOM_SAILBOAT_DECK      9
#define ROOM_LIGHTHOUSE_HILL   10
#define ROOM_RACCOON_COUNCIL   11
#define ROOM_DARK_ALLEY        12
#define ROOM_WOLF_BEDROOM      13
#define ROOM_SECRET_GARDEN     14
#define ROOM_RIVERBANK         15

const room_t *room_get_by_id(int id);
int room_find_item(int room_id, int item_id);
bool room_has_exit(int room_id, const char *direction);
int room_get_exit_room(int room_id, const char *direction);
const char *room_get_exit_name(int room_id, int exit_index);
void _init_rooms(void);

#endif /* ROOMS_H */
