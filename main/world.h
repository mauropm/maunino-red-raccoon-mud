#ifndef WORLD_H
#define WORLD_H

#include "rooms.h"
#include "player.h"

void world_init(void);
void world_remove_item_from_room(int room_id, int item_index);
void world_add_item_to_room(int room_id, int item_id);
int world_get_players_in_room(int room_id, int *player_indices, int max_count);
const char *world_get_direction_opposite(const char *direction);

#endif /* WORLD_H */
