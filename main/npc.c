#include "npc.h"
#include <string.h>
#include "rooms.h"

static npc_t npcs[MAX_NPCS];
static char pending_msg[MAX_NPC_MSG];
static int pending_room = -1;
static bool has_pending = false;

static const char *raccoon_messages[] = {
    "The Little Red Raccoon whispers: I'm not stealing trash, I'm liberating it.",
    "The Little Red Raccoon mutters: The Wolf's trash is the BEST trash. Don't tell him.",
    "The Little Red Raccoon says: Hood? It's a fashion statement AND a disguise.",
    "The Little Red Raccoon sighs: Being a raccoon is hard work. Someone has to do it.",
    "The Little Red Raccoon giggles: I left the Wolf a thank-you note. In his trash can.",
    "The Little Red Raccoon says: The council voted. More snacks. Always more snacks.",
};

static const char *wolf_messages[] = {
    "The Wolf sighs: I wish the raccoon would stop stealing my garbage.",
    "The Wolf mutters: I bought those trash bags for a reason. A REASON.",
    "The Wolf says: My sailor is coming today. I should cook something. Again.",
    "The Wolf sighs: Why does everyone think I'm evil? I just like gardening.",
    "The Wolf says: The raccoon can have the trash. I'll just buy more.",
    "The Wolf mutters: I left the fancy trash bag out. She'll love it.",
};

static const char *sailor_messages[] = {
    "The Oregon Sailor says: The sea is calmer than my dating life.",
    "The Oregon Sailor shouts: I brought snacks from Oregon! They're mostly seaweed.",
    "The Oregon Sailor says: This boat needs a name. I'm thinking 'SS Trash Panda'.",
    "The Oregon Sailor mutters: The Wolf makes great cookies. The raccoon eats them all.",
    "The Oregon Sailor says: Oregon is nice. But this creek has better trash.",
    "The Oregon Sailor shouts: Anyone want to go sailing? I accept payment in sandwiches.",
};

void npc_init(void)
{
    memset(npcs, 0, sizeof(npcs));

    npcs[0].id = 0;
    npcs[0].name = "Little Red Raccoon";
    npcs[0].current_room = ROOM_RACCOON_COUNCIL;
    npcs[0].message_count = sizeof(raccoon_messages) / sizeof(raccoon_messages[0]);
    for (int i = 0; i < npcs[0].message_count; i++) {
        npcs[0].messages[i] = raccoon_messages[i];
    }
    npcs[0].message_index = 0;
    npcs[0].move_timer = 30;
    npcs[0].move_interval = 60;
    npcs[0].say_timer = 45;
    npcs[0].say_interval = 90;
    npcs[0].active = true;

    npcs[1].id = 1;
    npcs[1].name = "The Wolf";
    npcs[1].current_room = ROOM_WOLF_KITCHEN;
    npcs[1].message_count = sizeof(wolf_messages) / sizeof(wolf_messages[0]);
    for (int i = 0; i < npcs[1].message_count; i++) {
        npcs[1].messages[i] = wolf_messages[i];
    }
    npcs[1].message_index = 0;
    npcs[1].move_timer = 25;
    npcs[1].move_interval = 45;
    npcs[1].say_timer = 60;
    npcs[1].say_interval = 120;
    npcs[1].active = true;

    npcs[2].id = 2;
    npcs[2].name = "Oregon Sailor";
    npcs[2].current_room = ROOM_SAILBOAT_DECK;
    npcs[2].message_count = sizeof(sailor_messages) / sizeof(sailor_messages[0]);
    for (int i = 0; i < npcs[2].message_count; i++) {
        npcs[2].messages[i] = sailor_messages[i];
    }
    npcs[2].message_index = 0;
    npcs[2].move_timer = 20;
    npcs[2].move_interval = 50;
    npcs[2].say_timer = 50;
    npcs[2].say_interval = 100;
    npcs[2].active = true;

    pending_room = -1;
    has_pending = false;
}

const npc_t *npc_get_by_id(int id)
{
    if (id >= 0 && id < MAX_NPCS && npcs[id].active) {
        return &npcs[id];
    }
    return NULL;
}

void npc_tick(void)
{
    for (int i = 0; i < MAX_NPCS; i++) {
        if (!npcs[i].active) continue;

        npcs[i].move_timer--;
        npcs[i].say_timer--;

        if (npcs[i].move_timer <= 0) {
            npcs[i].move_timer = npcs[i].move_interval;
            const room_t *room = room_get_by_id(npcs[i].current_room);
            if (room && room->exit_count > 0) {
                int exit_idx = npcs[i].move_timer % room->exit_count;
                npcs[i].current_room = room->exits[exit_idx].to_room;
            }
        }

        if (npcs[i].say_timer <= 0) {
            npcs[i].say_timer = npcs[i].say_interval;
            if (!has_pending) {
                strncpy(pending_msg, npcs[i].messages[npcs[i].message_index], MAX_NPC_MSG - 1);
                pending_msg[MAX_NPC_MSG - 1] = '\0';
                pending_room = npcs[i].current_room;
                has_pending = true;
                npcs[i].message_index = (npcs[i].message_index + 1) % npcs[i].message_count;
            }
        }
    }
}

const char *npc_get_pending_message(int *room_id)
{
    if (has_pending) {
        has_pending = false;
        *room_id = pending_room;
        return pending_msg;
    }
    return NULL;
}
