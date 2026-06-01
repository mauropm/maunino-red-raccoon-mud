#include "player.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "player";

static player_t players[MAX_PLAYERS];

void player_init(void)
{
    memset(players, 0, sizeof(players));
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].sockfd = -1;
        players[i].state = PLAYER_STATE_CONNECTING;
        players[i].current_room = 0;
        players[i].inventory_count = 0;
        players[i].score = 0;
        players[i].trash_collected = 0;
        players[i].active = false;
        players[i].use_color = false;
        players[i].input_len = 0;
        players[i].transport = TRANSPORT_TELNET;
        players[i].send_fn = NULL;
        players[i].close_fn = NULL;
        players[i].session_token[0] = '\0';
    }
}

int player_add(int sockfd)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) {
            players[i].sockfd = sockfd;
            players[i].active = true;
            players[i].state = PLAYER_STATE_LOGIN;
            players[i].name[0] = '\0';
            players[i].current_room = 0;
            players[i].inventory_count = 0;
            players[i].score = 0;
            players[i].trash_collected = 0;
            players[i].use_color = false;
            players[i].input_len = 0;
            players[i].transport = TRANSPORT_TELNET;
            players[i].send_fn = NULL;
            players[i].close_fn = NULL;
            players[i].session_token[0] = '\0';
            memset(players[i].input_buf, 0, sizeof(players[i].input_buf));
            ESP_LOGI(TAG, "Player connected on socket %d (slot %d)", sockfd, i);
            return i;
        }
    }
    ESP_LOGW(TAG, "No free player slots for socket %d", sockfd);
    return -1;
}

void player_remove(int sockfd)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active && players[i].sockfd == sockfd) {
            ESP_LOGI(TAG, "Player '%s' disconnected (slot %d)", players[i].name, i);
            players[i].active = false;
            players[i].sockfd = -1;
            players[i].state = PLAYER_STATE_CONNECTING;
            players[i].name[0] = '\0';
            players[i].inventory_count = 0;
            players[i].input_len = 0;
            players[i].send_fn = NULL;
            players[i].close_fn = NULL;
            players[i].session_token[0] = '\0';
            return;
        }
    }
}

player_t *player_get_by_socket(int sockfd)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active && players[i].sockfd == sockfd) {
            return &players[i];
        }
    }
    return NULL;
}

player_t *player_get_by_name(const char *name)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active && strcmp(players[i].name, name) == 0) {
            return &players[i];
        }
    }
    return NULL;
}

player_t *player_get_by_index(int index)
{
    if (index >= 0 && index < MAX_PLAYERS && players[index].active) {
        return &players[index];
    }
    return NULL;
}

player_t *player_get_by_token(const char *token)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active && players[i].session_token[0] &&
            strcmp(players[i].session_token, token) == 0) {
            return &players[i];
        }
    }
    return NULL;
}

bool player_name_exists(const char *name)
{
    return player_get_by_name(name) != NULL;
}

bool player_is_valid_name(const char *name)
{
    int len = strlen(name);
    if (len == 0 || len > MAX_NAME_LEN - 1) {
        return false;
    }
    for (int i = 0; i < len; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') {
            return false;
        }
    }
    return true;
}

void player_add_item(player_t *player, int item_id)
{
    if (player->inventory_count < MAX_INVENTORY) {
        player->inventory[player->inventory_count++] = item_id;
    }
}

void player_remove_item(player_t *player, int item_index)
{
    if (item_index >= 0 && item_index < player->inventory_count) {
        for (int i = item_index; i < player->inventory_count - 1; i++) {
            player->inventory[i] = player->inventory[i + 1];
        }
        player->inventory_count--;
    }
}

int player_has_item(player_t *player, int item_id)
{
    for (int i = 0; i < player->inventory_count; i++) {
        if (player->inventory[i] == item_id) {
            return i;
        }
    }
    return -1;
}

void player_add_score(player_t *player, int points)
{
    player->score += points;
    player->trash_collected++;
}

int player_get_active_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active) {
            count++;
        }
    }
    return count;
}

void player_send(player_t *player, const char *msg)
{
    if (!player || !msg) return;
    if (player->send_fn) {
        player->send_fn(player, msg);
    } else if (player->sockfd >= 0) {
        send(player->sockfd, msg, strlen(msg), MSG_NOSIGNAL);
    }
}

void player_broadcast_room(int room_id, const char *msg, player_t *exclude)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active || players[i].state != PLAYER_STATE_PLAYING) continue;
        if (players[i].current_room == room_id && &players[i] != exclude) {
            player_send(&players[i], msg);
        }
    }
}

void player_broadcast_all(const char *msg, player_t *exclude)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active || players[i].state != PLAYER_STATE_PLAYING) continue;
        if (&players[i] != exclude) {
            player_send(&players[i], msg);
        }
    }
}

void player_disconnect(player_t *player)
{
    if (!player) return;
    if (player->close_fn) {
        player->close_fn(player);
    } else if (player->sockfd >= 0) {
        close(player->sockfd);
    }
    player_remove(player->sockfd);
}
