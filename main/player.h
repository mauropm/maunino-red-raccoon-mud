#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>
#include "items.h"

#define MAX_PLAYERS       8
#define MAX_NAME_LEN      16
#define MAX_INPUT_LEN     128
#define MAX_INVENTORY     16
#define SESSION_TOKEN_LEN 33

typedef enum {
    PLAYER_STATE_CONNECTING,
    PLAYER_STATE_LOGIN,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_QUITTING,
} player_state_t;

typedef enum {
    TRANSPORT_TELNET,
    TRANSPORT_WEB,
} transport_type_t;

typedef struct player player_t;

typedef void (*transport_send_fn)(player_t *player, const char *msg);
typedef void (*transport_close_fn)(player_t *player);

struct player {
    int sockfd;
    char name[MAX_NAME_LEN];
    player_state_t state;
    int current_room;
    int inventory[MAX_INVENTORY];
    int inventory_count;
    int score;
    int trash_collected;
    bool active;
    bool use_color;
    char input_buf[MAX_INPUT_LEN];
    int input_len;

    transport_type_t transport;
    transport_send_fn send_fn;
    transport_close_fn close_fn;
    char session_token[SESSION_TOKEN_LEN];
};

void player_init(void);
int player_add(int sockfd);
void player_remove(int sockfd);
player_t *player_get_by_socket(int sockfd);
player_t *player_get_by_name(const char *name);
player_t *player_get_by_index(int index);
player_t *player_get_by_token(const char *token);
bool player_name_exists(const char *name);
bool player_is_valid_name(const char *name);
void player_add_item(player_t *player, int item_id);
void player_remove_item(player_t *player, int item_index);
int player_has_item(player_t *player, int item_id);
void player_add_score(player_t *player, int points);
int player_get_active_count(void);

void player_send(player_t *player, const char *msg);
void player_broadcast_room(int room_id, const char *msg, player_t *exclude);
void player_broadcast_all(const char *msg, player_t *exclude);
void player_disconnect(player_t *player);

#endif /* PLAYER_H */
