#ifndef ITEMS_H
#define ITEMS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_ITEMS 16

typedef struct {
    int id;
    const char *name;
    const char *description;
    int points;
    bool is_trash;
} item_def_t;

extern const item_def_t item_defs[MAX_ITEMS];

#define ITEM_HALF_EATEN_SANDWICH  0
#define ITEM_SHINY_BOTTLE_CAP     1
#define ITEM_FISH_SKELETON        2
#define ITEM_SAILOR_HAT           3
#define ITEM_FANCY_TRASH_BAG      4
#define ITEM_RUBBER_BOOT          5
#define ITEM_SUSPICIOUS_BANANA    6
#define ITEM_WOLF_PORTRAIT        7
#define ITEM_RUSTY_KEY            8
#define ITEM_MOSSY_STONE          9
#define ITEM_OLD_NEWSPAPER       10
#define ITEM_GOLDEN_ACORN        11
#define ITEM_BROKEN_COMPASS      12
#define ITEM_TIN_WHISTLE         13
#define ITEM_RED_HOOD            14
#define ITEM_PET_ROCK            15

const item_def_t *item_get_by_id(int id);
int item_find_by_name(const char *name);

#endif /* ITEMS_H */
