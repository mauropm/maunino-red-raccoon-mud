#ifndef NPC_H
#define NPC_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_NPCS        8
#define MAX_NPC_NAME    32
#define MAX_NPC_MSG     128
#define MAX_NPC_MSGS    8

typedef struct {
    int id;
    const char *name;
    int current_room;
    const char *messages[MAX_NPC_MSGS];
    int message_count;
    int message_index;
    int move_timer;
    int move_interval;
    int say_timer;
    int say_interval;
    bool active;
} npc_t;

void npc_init(void);
const npc_t *npc_get_by_id(int id);
void npc_tick(void);
const char *npc_get_pending_message(int *room_id);

#endif /* NPC_H */
