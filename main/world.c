#include "world.h"
#include <string.h>

void world_init(void)
{
    _init_rooms();
}

void world_remove_item_from_room(int room_id, int item_index)
{
    if (room_id < 0 || room_id >= MAX_ROOMS) return;
    room_t *room = &rooms[room_id];
    if (item_index < 0 || item_index >= room->item_count) return;

    for (int i = item_index; i < room->item_count - 1; i++) {
        room->items[i] = room->items[i + 1];
    }
    room->item_count--;
}

void world_add_item_to_room(int room_id, int item_id)
{
    if (room_id < 0 || room_id >= MAX_ROOMS) return;
    room_t *room = &rooms[room_id];
    if (room->item_count >= MAX_ITEMS) return;

    room->items[room->item_count++] = item_id;
}

int world_get_players_in_room(int room_id, int *player_indices, int max_count)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS && count < max_count; i++) {
        player_t *p = player_get_by_index(i);
        if (p && p->state == PLAYER_STATE_PLAYING && p->current_room == room_id) {
            player_indices[count++] = i;
        }
    }
    return count;
}

const char *world_get_direction_opposite(const char *direction)
{
    if (strcasecmp(direction, "north") == 0) return "south";
    if (strcasecmp(direction, "south") == 0) return "north";
    if (strcasecmp(direction, "east") == 0) return "west";
    if (strcasecmp(direction, "west") == 0) return "east";
    if (strcasecmp(direction, "up") == 0) return "down";
    if (strcasecmp(direction, "down") == 0) return "up";
    return "somewhere";
}
