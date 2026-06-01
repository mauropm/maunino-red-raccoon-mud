#include "commands.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include "player.h"
#include "world.h"
#include "rooms.h"
#include "items.h"
#include "esp_log.h"

#define ANSI_RESET    "\033[0m"
#define ANSI_BOLD     "\033[1m"
#define ANSI_GREEN    "\033[32m"
#define ANSI_YELLOW   "\033[33m"
#define ANSI_BLUE     "\033[34m"
#define ANSI_CYAN     "\033[36m"
#define ANSI_RED      "\033[31m"
#define ANSI_MAGENTA  "\033[35m"

static void send_fmt(player_t *player, const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    player_send(player, buf);
}

static void parse_command(const char *input, char *cmd, char *arg, int arg_size)
{
    const char *p = input;
    while (*p == ' ') p++;

    int i = 0;
    while (*p && *p != ' ' && i < 31) {
        cmd[i++] = *p++;
    }
    cmd[i] = '\0';

    while (*p == ' ') p++;

    i = 0;
    while (*p && i < arg_size - 1) {
        arg[i++] = *p++;
    }
    arg[i] = '\0';

    for (int j = 0; j < (int)strlen(cmd); j++) {
        cmd[j] = tolower((unsigned char)cmd[j]);
    }
}

static void cmd_help(player_t *player)
{
    send_fmt(player, "\r\n%sCommands:%s\r\n", ANSI_BOLD, ANSI_RESET);
    send_fmt(player, "  %shelp%s       - Show this help\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %slook%s       - Look around the room\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %swho%s        - See who is online\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %ssay <msg>%s  - Talk to room occupants\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %sshout <msg>%s- Talk to everyone\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %snorth%s      - Go north (or %sn%s)\r\n", ANSI_CYAN, ANSI_RESET, ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %ssouth%s      - Go south (or %ss%s)\r\n", ANSI_CYAN, ANSI_RESET, ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %seast%s       - Go east (or %se%s)\r\n", ANSI_CYAN, ANSI_RESET, ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %swest%s       - Go west (or %sw%s)\r\n", ANSI_CYAN, ANSI_RESET, ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %sinventory%s  - Check your stuff\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %stake <item>%s- Pick up an item\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %sdrop <item>%s- Drop an item\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %sexamine%s    - Examine something\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %sscore%s      - Check your score\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "  %squit%s       - Leave the game\r\n", ANSI_CYAN, ANSI_RESET);
    send_fmt(player, "\r\n");
}

static void cmd_who(player_t *player)
{
    send_fmt(player, "\r\n%sPlayers online:%s\r\n", ANSI_BOLD, ANSI_RESET);
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        player_t *p = player_get_by_index(i);
        if (p && p->state == PLAYER_STATE_PLAYING) {
            const room_t *room = room_get_by_id(p->current_room);
            send_fmt(player, "  %s - %s%s\r\n", p->name,
                     room ? room->name : "Unknown", ANSI_RESET);
            count++;
        }
    }
    if (count == 0) {
        send_fmt(player, "  (nobody, how depressing)\r\n");
    }
    send_fmt(player, "\r\n");
}

void command_look(player_t *player)
{
    if (!player) return;

    const room_t *room = room_get_by_id(player->current_room);
    if (!room) return;

    send_fmt(player, "\r\n%s%s%s\r\n", ANSI_BOLD, ANSI_GREEN, room->name, ANSI_RESET);
    send_fmt(player, "%s%s%s\r\n", ANSI_YELLOW, room->description, ANSI_RESET);

    send_fmt(player, "\r\n%sExits:%s ", ANSI_BOLD, ANSI_RESET);
    for (int i = 0; i < room->exit_count; i++) {
        if (i > 0) send_fmt(player, ", ");
        send_fmt(player, "%s", room->exits[i].direction);
    }
    send_fmt(player, "\r\n");

    if (room->item_count > 0) {
        send_fmt(player, "\r\n%sYou see:%s\r\n", ANSI_BOLD, ANSI_RESET);
        for (int i = 0; i < room->item_count; i++) {
            const item_def_t *item = item_get_by_id(room->items[i]);
            if (item) {
                send_fmt(player, "  - %s%s%s\r\n", ANSI_CYAN, item->name, ANSI_RESET);
            }
        }
    }

    int others[MAX_PLAYERS];
    int count = world_get_players_in_room(player->current_room, others, MAX_PLAYERS);
    if (count > 0) {
        send_fmt(player, "\r\n%sAlso here:%s\r\n", ANSI_BOLD, ANSI_RESET);
        for (int i = 0; i < count; i++) {
            player_t *other = player_get_by_index(others[i]);
            if (other && other != player) {
                send_fmt(player, "  - %s%s%s\r\n", ANSI_MAGENTA, other->name, ANSI_RESET);
            }
        }
    }

    send_fmt(player, "\r\n");
}

static void cmd_say(player_t *player, const char *arg)
{
    if (!player || strlen(arg) == 0) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "%s%s%s says: %s%s%s\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET,
             ANSI_YELLOW, arg, ANSI_RESET);
    player_broadcast_room(player->current_room, msg, player);
}

static void cmd_shout(player_t *player, const char *arg)
{
    if (!player || strlen(arg) == 0) return;

    char msg[256];
    snprintf(msg, sizeof(msg), "%s[SHOUT] %s%s%s shouts: %s%s%s\r\n",
             ANSI_RED, ANSI_MAGENTA, player->name, ANSI_RED,
             ANSI_YELLOW, arg, ANSI_RESET);
    player_broadcast_all(msg, player);
}

static void cmd_move(player_t *player, const char *direction)
{
    if (!player) return;

    int new_room = room_get_exit_room(player->current_room, direction);
    if (new_room < 0) {
        send_fmt(player, "You can't go %s.\r\n", direction);
        return;
    }

    const room_t *old_room = room_get_by_id(player->current_room);
    const char *dir_name = NULL;
    for (int i = 0; i < old_room->exit_count; i++) {
        if (strcasecmp(old_room->exits[i].direction, direction) == 0) {
            dir_name = old_room->exits[i].direction;
            break;
        }
    }
    if (!dir_name) dir_name = direction;

    char msg[128];
    snprintf(msg, sizeof(msg), "%s%s%s leaves %s.\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET, dir_name);
    player_broadcast_room(player->current_room, msg, player);

    player->current_room = new_room;

    snprintf(msg, sizeof(msg), "%s%s%s enters from the %s.\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET,
             world_get_direction_opposite(dir_name));
    player_broadcast_room(new_room, msg, player);

    command_look(player);
}

static void cmd_inventory(player_t *player)
{
    if (!player) return;

    send_fmt(player, "\r\n%sYou are carrying:%s\r\n", ANSI_BOLD, ANSI_RESET);
    if (player->inventory_count == 0) {
        send_fmt(player, "  Nothing. Your paws are empty. Sad.\r\n");
    } else {
        for (int i = 0; i < player->inventory_count; i++) {
            const item_def_t *item = item_get_by_id(player->inventory[i]);
            if (item) {
                send_fmt(player, "  - %s%s%s\r\n", ANSI_CYAN, item->name, ANSI_RESET);
            }
        }
    }
    send_fmt(player, "\r\n");
}

static void cmd_take(player_t *player, const char *arg)
{
    if (!player || strlen(arg) == 0) {
        send_fmt(player, "Take what?\r\n");
        return;
    }

    const room_t *room = room_get_by_id(player->current_room);
    if (!room) return;

    int item_idx = -1;
    for (int i = 0; i < room->item_count; i++) {
        const item_def_t *item = item_get_by_id(room->items[i]);
        if (item && strcasecmp(item->name, arg) == 0) {
            item_idx = i;
            break;
        }
    }

    if (item_idx < 0) {
        send_fmt(player, "You don't see that here.\r\n");
        return;
    }

    int item_id = room->items[item_idx];
    const item_def_t *item = item_get_by_id(item_id);

    player_add_item(player, item_id);
    world_remove_item_from_room(player->current_room, item_idx);

    if (item->is_trash) {
        player_add_score(player, item->points);
        send_fmt(player, "You pick up the %s%s%s. (+%d points, trash #%d)\r\n",
                 ANSI_CYAN, item->name, ANSI_RESET, item->points,
                 player->trash_collected);
    } else {
        send_fmt(player, "You pick up the %s%s%s.\r\n",
                 ANSI_CYAN, item->name, ANSI_RESET);
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "%s%s%s picks up the %s.\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET, item->name);
    player_broadcast_room(player->current_room, msg, player);
}

static void cmd_drop(player_t *player, const char *arg)
{
    if (!player || strlen(arg) == 0) {
        send_fmt(player, "Drop what?\r\n");
        return;
    }

    int inv_idx = -1;
    for (int i = 0; i < player->inventory_count; i++) {
        const item_def_t *item = item_get_by_id(player->inventory[i]);
        if (item && strcasecmp(item->name, arg) == 0) {
            inv_idx = i;
            break;
        }
    }

    if (inv_idx < 0) {
        send_fmt(player, "You don't have that.\r\n");
        return;
    }

    int item_id = player->inventory[inv_idx];
    const item_def_t *item = item_get_by_id(item_id);

    player_remove_item(player, inv_idx);
    world_add_item_to_room(player->current_room, item_id);

    send_fmt(player, "You drop the %s%s%s.\r\n", ANSI_CYAN, item->name, ANSI_RESET);

    char msg[128];
    snprintf(msg, sizeof(msg), "%s%s%s drops the %s.\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET, item->name);
    player_broadcast_room(player->current_room, msg, player);
}

static void cmd_examine(player_t *player, const char *arg)
{
    if (!player || strlen(arg) == 0) {
        send_fmt(player, "Examine what?\r\n");
        return;
    }

    for (int i = 0; i < player->inventory_count; i++) {
        const item_def_t *item = item_get_by_id(player->inventory[i]);
        if (item && strcasecmp(item->name, arg) == 0) {
            send_fmt(player, "\r\n%s%s:%s %s\r\n\r\n",
                     ANSI_BOLD, item->name, ANSI_RESET, item->description);
            return;
        }
    }

    const room_t *room = room_get_by_id(player->current_room);
    if (room) {
        for (int i = 0; i < room->item_count; i++) {
            const item_def_t *item = item_get_by_id(room->items[i]);
            if (item && strcasecmp(item->name, arg) == 0) {
                send_fmt(player, "\r\n%s%s:%s %s\r\n\r\n",
                         ANSI_BOLD, item->name, ANSI_RESET, item->description);
                return;
            }
        }
    }

    send_fmt(player, "You don't see that here.\r\n");
}

static void cmd_score(player_t *player)
{
    if (!player) return;

    send_fmt(player, "\r\n%s=== Score ===%s\r\n", ANSI_BOLD, ANSI_RESET);
    send_fmt(player, "Player: %s%s%s\r\n", ANSI_MAGENTA, player->name, ANSI_RESET);
    send_fmt(player, "Score:  %s%d%s points\r\n", ANSI_GREEN, player->score, ANSI_RESET);
    send_fmt(player, "Trash:  %s%d%s items collected\r\n", ANSI_CYAN, player->trash_collected, ANSI_RESET);

    if (player->score >= 500) {
        send_fmt(player, "\r\n%sRank: Master Trash Panda%s\r\n", ANSI_YELLOW, ANSI_RESET);
    } else if (player->score >= 200) {
        send_fmt(player, "\r\n%sRank: Senior Raccoon%s\r\n", ANSI_YELLOW, ANSI_RESET);
    } else if (player->score >= 50) {
        send_fmt(player, "\r\n%sRank: Trash Enthusiast%s\r\n", ANSI_YELLOW, ANSI_RESET);
    } else {
        send_fmt(player, "\r\n%sRank: Novice Dumpster Diver%s\r\n", ANSI_YELLOW, ANSI_RESET);
    }
    send_fmt(player, "\r\n");
}

static void cmd_quit(player_t *player)
{
    if (!player) return;

    char msg[128];
    snprintf(msg, sizeof(msg), "%s%s%s quits the game.\r\n",
             ANSI_MAGENTA, player->name, ANSI_RESET);
    player_broadcast_room(player->current_room, msg, player);

    send_fmt(player, "\r\nThanks for playing! The raccoons will remember you.\r\n");
    send_fmt(player, "Your final score: %d points.\r\n", player->score);

    player_disconnect(player);
}

void command_process(player_t *player, const char *input)
{
    if (!player) return;

    char cmd[32];
    char arg[128];

    parse_command(input, cmd, arg, sizeof(arg));

    if (strlen(cmd) == 0) return;

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        cmd_help(player);
    } else if (strcmp(cmd, "look") == 0 || strcmp(cmd, "l") == 0) {
        command_look(player);
    } else if (strcmp(cmd, "who") == 0) {
        cmd_who(player);
    } else if (strcmp(cmd, "say") == 0) {
        cmd_say(player, arg);
    } else if (strcmp(cmd, "shout") == 0) {
        cmd_shout(player, arg);
    } else if (strcmp(cmd, "north") == 0 || strcmp(cmd, "n") == 0) {
        cmd_move(player, "north");
    } else if (strcmp(cmd, "south") == 0 || strcmp(cmd, "s") == 0) {
        cmd_move(player, "south");
    } else if (strcmp(cmd, "east") == 0 || strcmp(cmd, "e") == 0) {
        cmd_move(player, "east");
    } else if (strcmp(cmd, "west") == 0 || strcmp(cmd, "w") == 0) {
        cmd_move(player, "west");
    } else if (strcmp(cmd, "inventory") == 0 || strcmp(cmd, "i") == 0) {
        cmd_inventory(player);
    } else if (strcmp(cmd, "take") == 0 || strcmp(cmd, "get") == 0) {
        cmd_take(player, arg);
    } else if (strcmp(cmd, "drop") == 0) {
        cmd_drop(player, arg);
    } else if (strcmp(cmd, "examine") == 0 || strcmp(cmd, "x") == 0) {
        cmd_examine(player, arg);
    } else if (strcmp(cmd, "score") == 0) {
        cmd_score(player);
    } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
        cmd_quit(player);
    } else {
        send_fmt(player, "Unknown command: %s. Type 'help' for commands.\r\n", cmd);
    }
}
