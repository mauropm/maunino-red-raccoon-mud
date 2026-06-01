#include "items.h"
#include <string.h>
#include <strings.h>

const item_def_t item_defs[MAX_ITEMS] = {
    {ITEM_HALF_EATEN_SANDWICH,  "Half-Eaten Sandwich",  "A suspicious sandwich. Possibly still edible. Probably.", 5, true},
    {ITEM_SHINY_BOTTLE_CAP,     "Shiny Bottle Cap",     "Reflects light beautifully. Worth exactly nothing.", 10, true},
    {ITEM_FISH_SKELETON,        "Fish Skeleton",        "Someone had dinner here. A very neat eater.", 20, true},
    {ITEM_SAILOR_HAT,           "Sailor Hat",           "Smells faintly of sea salt and poor decisions.", 30, false},
    {ITEM_FANCY_TRASH_BAG,      "Fancy Trash Bag",      "A premium garbage receptacle. The raccoon elite approve.", 50, true},
    {ITEM_RUBBER_BOOT,          "Rubber Boot",          "One boot. Left foot only. The right boot is out there somewhere.", 15, true},
    {ITEM_SUSPICIOUS_BANANA,    "Suspicious Banana",    "It is blue. Do not question it.", 25, true},
    {ITEM_WOLF_PORTRAIT,        "Wolf Portrait",        "The Wolf looks dashing. Better than the real Wolf.", 35, false},
    {ITEM_RUSTY_KEY,            "Rusty Key",            "Opens nothing important. But it feels important.", 20, false},
    {ITEM_MOSSY_STONE,          "Mossy Stone",          "A stone with moss. The moss is judging you.", 10, true},
    {ITEM_OLD_NEWSPAPER,        "Old Newspaper",        "Headline: 'LOCAL WOLF DATES SAILOR, COMMUNITY SHOCKED'", 15, true},
    {ITEM_GOLDEN_ACORN,         "Golden Acorn",         "The raccoon council's most prized possession. Somehow here.", 100, false},
    {ITEM_BROKEN_COMPASS,       "Broken Compass",       "Points toward the nearest trash can. Actually useful.", 20, true},
    {ITEM_TIN_WHISTLE,          "Tin Whistle",          "Plays a mournful tune. Or a squeak. Hard to tell.", 25, false},
    {ITEM_RED_HOOD,             "Red Hood",             "The iconic garment. Slightly singed.", 40, false},
    {ITEM_PET_ROCK,             "Pet Rock",             "It does nothing. It is perfect.", 5, true},
};

const item_def_t *item_get_by_id(int id)
{
    if (id >= 0 && id < MAX_ITEMS) {
        return &item_defs[id];
    }
    return NULL;
}

int item_find_by_name(const char *name)
{
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (strcasecmp(item_defs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
