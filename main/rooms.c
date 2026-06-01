#include "rooms.h"
#include <string.h>
#include <strings.h>
#include "items.h"

room_t rooms[MAX_ROOMS];

static const room_t _room_templates[MAX_ROOMS] = {
    {
        .id = ROOM_FOREST_PATH,
        .name = "Forest Path",
        .description =
            "A winding dirt path through the forest. The trees here seem to whisper\n"
            "gossip about the Wolf's dating life. A faded sign reads: 'Wolf House ->'\n"
            "and 'Raccoon Council <- (members only, no wolves)'.\n"
            "The ground is littered with suspicious debris.",
        .items = {ITEM_HALF_EATEN_SANDWICH, ITEM_MOSSY_STONE},
        .item_count = 2,
        .exits = {
            {ROOM_FOREST_PATH, ROOM_BERRY_BUSHES, "north"},
            {ROOM_FOREST_PATH, ROOM_CREEK_CROSSING, "east"},
            {ROOM_FOREST_PATH, ROOM_WOLF_FRONT_YARD, "south"},
            {ROOM_FOREST_PATH, ROOM_DARK_ALLEY, "west"},
        },
        .exit_count = 4,
    },
    {
        .id = ROOM_BERRY_BUSHES,
        .name = "Berry Bushes",
        .description =
            "Lush berry bushes line this clearing. Most berries have been picked,\n"
            "leaving behind a few bruised survivors. A raccoon-sized picnic blanket\n"
            "lies abandoned nearby. You notice berry stains on everything.\n"
            "The bushes rustle suspiciously.",
        .items = {ITEM_SUSPICIOUS_BANANA},
        .item_count = 1,
        .exits = {
            {ROOM_BERRY_BUSHES, ROOM_FOREST_PATH, "south"},
            {ROOM_BERRY_BUSHES, ROOM_LIGHTHOUSE_HILL, "east"},
            {ROOM_BERRY_BUSHES, ROOM_RIVERBANK, "north"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_CREEK_CROSSING,
        .name = "Creek Crossing",
        .description =
            "A shallow creek babbles over smooth stones. Stepping stones provide\n"
            "a crossing, though some wobble alarmingly. Fish skeletons litter\n"
            "the banks - evidence of a raccoon's fishing expedition.\n"
            "The water smells faintly of adventure and poor hygiene.",
        .items = {ITEM_FISH_SKELETON, ITEM_BROKEN_COMPASS},
        .item_count = 2,
        .exits = {
            {ROOM_CREEK_CROSSING, ROOM_FOREST_PATH, "west"},
            {ROOM_CREEK_CROSSING, ROOM_ABANDONED_CANOE, "east"},
            {ROOM_CREEK_CROSSING, ROOM_RIVERBANK, "north"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_ABANDONED_CANOE,
        .name = "Abandoned Canoe",
        .description =
            "An old canoe rests half-submerged in the creek. It has 'PROPERTY OF\n"
            "OREGON SAILOR' scratched into the side, along with a heart and what\n"
            "appears to be a poorly drawn boat. The canoe smells of regret.\n"
            "A single rubber boot sits inside.",
        .items = {ITEM_RUBBER_BOOT},
        .item_count = 1,
        .exits = {
            {ROOM_ABANDONED_CANOE, ROOM_CREEK_CROSSING, "west"},
            {ROOM_ABANDONED_CANOE, ROOM_OREGON_DOCK, "south"},
        },
        .exit_count = 2,
    },
    {
        .id = ROOM_WOLF_FRONT_YARD,
        .name = "Wolf Front Yard",
        .description =
            "The Wolf's front yard is surprisingly well-kept. Flower beds, a\n"
            "neatly trimmed lawn, and a welcome mat that says 'GO AWAY (politely)'.\n"
            "A garden gnome wearing a tiny sailor hat stands guard.\n"
            "The front door leads inside. A path leads back to the forest.",
        .items = {ITEM_SHINY_BOTTLE_CAP},
        .item_count = 1,
        .exits = {
            {ROOM_WOLF_FRONT_YARD, ROOM_FOREST_PATH, "north"},
            {ROOM_WOLF_FRONT_YARD, ROOM_WOLF_KITCHEN, "east"},
            {ROOM_WOLF_FRONT_YARD, ROOM_WOLF_LIVING_ROOM, "south"},
            {ROOM_WOLF_FRONT_YARD, ROOM_TRASH_STORAGE, "west"},
        },
        .exit_count = 4,
    },
    {
        .id = ROOM_WOLF_KITCHEN,
        .name = "Wolf Kitchen",
        .description =
            "A cozy kitchen with checkered curtains. The Wolf clearly enjoys\n"
            "cooking - recipe books are scattered everywhere. One is open to\n"
            "'How to Cook for Your Sailor Partner (Who is Always Hungry)'.\n"
            "The fridge hums contentedly. Something smells like cookies.",
        .items = {ITEM_HALF_EATEN_SANDWICH},
        .item_count = 1,
        .exits = {
            {ROOM_WOLF_KITCHEN, ROOM_WOLF_FRONT_YARD, "west"},
            {ROOM_WOLF_KITCHEN, ROOM_WOLF_LIVING_ROOM, "south"},
            {ROOM_WOLF_KITCHEN, ROOM_WOLF_BEDROOM, "east"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_WOLF_LIVING_ROOM,
        .name = "Wolf Living Room",
        .description =
            "A comfortable living room with a large sofa, a fireplace, and\n"
            "an alarming number of framed photos of the Wolf with a sailor.\n"
            "A TV plays a nature documentary about raccoons. The Wolf has\n"
            "clearly not noticed the irony. A portrait hangs above the mantel.",
        .items = {ITEM_WOLF_PORTRAIT},
        .item_count = 1,
        .exits = {
            {ROOM_WOLF_LIVING_ROOM, ROOM_WOLF_FRONT_YARD, "north"},
            {ROOM_WOLF_LIVING_ROOM, ROOM_WOLF_KITCHEN, "north"},
            {ROOM_WOLF_LIVING_ROOM, ROOM_WOLF_BEDROOM, "east"},
            {ROOM_WOLF_LIVING_ROOM, ROOM_TRASH_STORAGE, "west"},
        },
        .exit_count = 4,
    },
    {
        .id = ROOM_TRASH_STORAGE,
        .name = "Trash Storage Shed",
        .description =
            "The Wolf's trash shed. This is the raccoon holy grail. Shelves\n"
            "groan under the weight of discarded treasures. A sign reads:\n"
            "'DO NOT TOUCH - WOLF'S PRECIOUS GARBAGE (sorted by day)'.\n"
            "The Fancy Trash Bag sits prominently on a pedestal.",
        .items = {ITEM_FANCY_TRASH_BAG, ITEM_OLD_NEWSPAPER},
        .item_count = 2,
        .exits = {
            {ROOM_TRASH_STORAGE, ROOM_WOLF_FRONT_YARD, "east"},
            {ROOM_TRASH_STORAGE, ROOM_WOLF_LIVING_ROOM, "east"},
            {ROOM_TRASH_STORAGE, ROOM_DARK_ALLEY, "south"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_OREGON_DOCK,
        .name = "Oregon Sailor Dock",
        .description =
            "A weathered wooden dock extends into the creek. Sailboats bob\n"
            "in the water, each one slightly more ridiculous than the last.\n"
            "A sign reads: 'OREGON SAILOR - WILL SAIL FOR SNACKS'.\n"
            "Sailor hats are scattered about like fallen leaves.",
        .items = {ITEM_SAILOR_HAT, ITEM_TIN_WHISTLE},
        .item_count = 2,
        .exits = {
            {ROOM_OREGON_DOCK, ROOM_ABANDONED_CANOE, "north"},
            {ROOM_OREGON_DOCK, ROOM_SAILBOAT_DECK, "south"},
            {ROOM_OREGON_DOCK, ROOM_LIGHTHOUSE_HILL, "east"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_SAILBOAT_DECK,
        .name = "Sailboat Deck",
        .description =
            "The deck of a small sailboat named 'SS Trash Panda'. The Oregon\n"
            "Sailor has decorated it with string lights and a small grill.\n"
            "A map of Oregon is pinned to the mast. The boat rocks gently.\n"
            "A golden acorn sits in a place of honor on the captain's chair.",
        .items = {ITEM_GOLDEN_ACORN},
        .item_count = 1,
        .exits = {
            {ROOM_SAILBOAT_DECK, ROOM_OREGON_DOCK, "north"},
        },
        .exit_count = 1,
    },
    {
        .id = ROOM_LIGHTHOUSE_HILL,
        .name = "Lighthouse Hill",
        .description =
            "A small lighthouse stands on a hill overlooking the creek. The\n"
            "light hasn't worked in years, but the view is spectacular.\n"
            "Graffiti on the wall reads: 'THE WOLF AND SAILOR SITTING IN\n"
            "A TREE, S-T-E-A-L-I-N-G T-R-A-S-H'. Someone has added 'CLASSY.'",
        .items = {ITEM_RUSTY_KEY},
        .item_count = 1,
        .exits = {
            {ROOM_LIGHTHOUSE_HILL, ROOM_BERRY_BUSHES, "west"},
            {ROOM_LIGHTHOUSE_HILL, ROOM_OREGON_DOCK, "west"},
            {ROOM_LIGHTHOUSE_HILL, ROOM_RACCOON_COUNCIL, "south"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_RACCOON_COUNCIL,
        .name = "Secret Raccoon Council",
        .description =
            "A hidden chamber beneath the lighthouse. A round table is surrounded\n"
            "by tiny chairs. Meeting minutes are posted on the wall:\n"
            "'Agenda: 1. More trash. 2. Avoid Wolf. 3. Snacks.'\n"
            "A red hood hangs on a hook. The council takes snacking very seriously.",
        .items = {ITEM_RED_HOOD, ITEM_PET_ROCK},
        .item_count = 2,
        .exits = {
            {ROOM_RACCOON_COUNCIL, ROOM_LIGHTHOUSE_HILL, "north"},
            {ROOM_RACCOON_COUNCIL, ROOM_DARK_ALLEY, "east"},
        },
        .exit_count = 2,
    },
    {
        .id = ROOM_DARK_ALLEY,
        .name = "Dark Alley",
        .description =
            "A narrow, shadowy alley between the Wolf's property and the\n"
            "raccoon council grounds. Perfect for sneaking. Trash bins line\n"
            "the walls. A raccoon-sized escape route runs along the top.\n"
            "You feel like you're being watched. Probably by another raccoon.",
        .items = {ITEM_MOSSY_STONE},
        .item_count = 1,
        .exits = {
            {ROOM_DARK_ALLEY, ROOM_FOREST_PATH, "east"},
            {ROOM_DARK_ALLEY, ROOM_TRASH_STORAGE, "north"},
            {ROOM_DARK_ALLEY, ROOM_RACCOON_COUNCIL, "west"},
        },
        .exit_count = 3,
    },
    {
        .id = ROOM_WOLF_BEDROOM,
        .name = "Wolf Bedroom",
        .description =
            "The Wolf's bedroom. Neatly made bed, a nightstand with a photo\n"
            "of the Wolf and the Oregon Sailor on a beach. A diary lies open:\n"
            "'Dear Diary, today the raccoon stole my good trash bags again.\n"
            "I left her the fancy ones. She seemed happy.' Touching.",
        .items = {ITEM_RUSTY_KEY},
        .item_count = 1,
        .exits = {
            {ROOM_WOLF_BEDROOM, ROOM_WOLF_KITCHEN, "west"},
            {ROOM_WOLF_BEDROOM, ROOM_WOLF_LIVING_ROOM, "west"},
        },
        .exit_count = 2,
    },
    {
        .id = ROOM_SECRET_GARDEN,
        .name = "Secret Garden",
        .description =
            "A hidden garden behind the Wolf's house. The Wolf grows vegetables\n"
            "here with obsessive care. Each plant has a name tag. The tomatoes\n"
            "are named after sailors from Oregon. A pet rock sits on a pedestal\n"
            "labeled 'MR. STONES - HEAD OF SECURITY'.",
        .items = {ITEM_PET_ROCK, ITEM_SHINY_BOTTLE_CAP},
        .item_count = 2,
        .exits = {
            {ROOM_SECRET_GARDEN, ROOM_WOLF_FRONT_YARD, "north"},
        },
        .exit_count = 1,
    },
    {
        .id = ROOM_RIVERBANK,
        .name = "Riverbank",
        .description =
            "The creek widens into a gentle river. The Oregon Sailor's boats\n"
            "are visible downstream. A raccoon fishing spot is marked with\n"
            "a sign: 'BEST FISHING - WOLF NOT ALLOWED'. Fish bones and\n"
            "a suspicious banana suggest recent raccoon activity.",
        .items = {ITEM_FISH_SKELETON, ITEM_SUSPICIOUS_BANANA},
        .item_count = 2,
        .exits = {
            {ROOM_RIVERBANK, ROOM_BERRY_BUSHES, "south"},
            {ROOM_RIVERBANK, ROOM_CREEK_CROSSING, "south"},
        },
        .exit_count = 2,
    },
};

void _init_rooms(void)
{
    memcpy(rooms, _room_templates, sizeof(_room_templates));
}

const room_t *room_get_by_id(int id)
{
    if (id >= 0 && id < MAX_ROOMS) {
        return &rooms[id];
    }
    return NULL;
}

int room_find_item(int room_id, int item_id)
{
    const room_t *room = room_get_by_id(room_id);
    if (!room) return -1;

    for (int i = 0; i < room->item_count; i++) {
        if (room->items[i] == item_id) {
            return i;
        }
    }
    return -1;
}

bool room_has_exit(int room_id, const char *direction)
{
    const room_t *room = room_get_by_id(room_id);
    if (!room) return false;

    for (int i = 0; i < room->exit_count; i++) {
        if (strcasecmp(room->exits[i].direction, direction) == 0) {
            return true;
        }
    }
    return false;
}

int room_get_exit_room(int room_id, const char *direction)
{
    const room_t *room = room_get_by_id(room_id);
    if (!room) return -1;

    for (int i = 0; i < room->exit_count; i++) {
        if (strcasecmp(room->exits[i].direction, direction) == 0) {
            return room->exits[i].to_room;
        }
    }
    return -1;
}

const char *room_get_exit_name(int room_id, int exit_index)
{
    const room_t *room = room_get_by_id(room_id);
    if (!room || exit_index < 0 || exit_index >= room->exit_count) {
        return NULL;
    }
    return room->exits[exit_index].direction;
}
