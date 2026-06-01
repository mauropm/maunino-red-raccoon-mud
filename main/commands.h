#ifndef COMMANDS_H
#define COMMANDS_H

#include "player.h"

void command_process(player_t *player, const char *input);
void command_look(player_t *player);

#endif /* COMMANDS_H */
