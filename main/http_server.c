#include "http_server.h"
#include "web_ui.h"
#include "player.h"
#include "commands.h"
#include "rooms.h"
#include "wifi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

static const char *TAG = "http";

#define HTTP_PORT 80
#define MAX_SSE_CLIENTS MAX_PLAYERS
#define HTTP_BUF_SIZE 2048
#define SSE_BUF_SIZE 1024
#define SSE_KEEPALIVE_MS 30000
#define SSE_CLEANUP_MS 10000

typedef struct {
    int sockfd;
    char token[SESSION_TOKEN_LEN];
    bool active;
} sse_client_t;

static sse_client_t sse_clients[MAX_SSE_CLIENTS];
static int listen_sock = -1;

static void set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    }
}

static void web_close_fn(player_t *player)
{
    if (!player) return;
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i].active &&
            strcmp(sse_clients[i].token, player->session_token) == 0) {
            close(sse_clients[i].sockfd);
            sse_clients[i].active = false;
            sse_clients[i].token[0] = '\0';
            break;
        }
    }
}

static void generate_token(char *buf, int len)
{
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < len - 1; i++) {
        buf[i] = hex[rand() % 16];
    }
    buf[len - 1] = '\0';
}

static int http_response(int sock, int status, const char *status_text,
                         const char *content_type, const char *body,
                         const char *extra_header)
{
    char hdr[512];
    int body_len = body ? (int)strlen(body) : 0;
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
        "%s"
        "\r\n",
        status, status_text,
        content_type,
        body_len,
        extra_header ? extra_header : "");
    send(sock, hdr, hlen, MSG_NOSIGNAL);
    if (body && body_len > 0) {
        send(sock, body, body_len, MSG_NOSIGNAL);
    }
    return body_len;
}

static int http_redirect(int sock, const char *location)
{
    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "HTTP/1.1 302 Found\r\n"
        "Location: %s\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n", location);
    return send(sock, buf, len, MSG_NOSIGNAL);
}

static void strip_ansi(const char *src, char *dst, int dst_size)
{
    int di = 0;
    for (int si = 0; src[si] && di < dst_size - 1; si++) {
        if (src[si] == '\033') {
            si++;
            if (src[si] == '[') {
                si++;
                while (src[si] && ((src[si] >= '0' && src[si] <= '9') || src[si] == ';')) si++;
                continue;
            }
            continue;
        }
        dst[di++] = src[si];
    }
    dst[di] = '\0';
}

void sse_send_to_player(player_t *player, const char *msg)
{
    if (!player || !msg) return;
    char clean[SSE_BUF_SIZE];
    strip_ansi(msg, clean, sizeof(clean));
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i].active &&
            strcmp(sse_clients[i].token, player->session_token) == 0) {
            char sse_msg[SSE_BUF_SIZE];
            int written = snprintf(sse_msg, sizeof(sse_msg), "data: %s\n\n", clean);
            if (written > 0) {
                send(sse_clients[i].sockfd, sse_msg, written, MSG_NOSIGNAL);
            }
            return;
        }
    }
}

static void sse_cleanup(void)
{
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (!sse_clients[i].active) continue;

        char c;
        int ret = recv(sse_clients[i].sockfd, &c, 1, MSG_PEEK);
        if (ret > 0) continue;
        if (ret < 0) {
            int err = errno;
            if (err == EAGAIN || err == EWOULDBLOCK) continue;
        }

        ESP_LOGI(TAG, "SSE client %d disconnected", sse_clients[i].sockfd);

        player_t *player = player_get_by_token(sse_clients[i].token);
        if (player && player->active && player->state == PLAYER_STATE_PLAYING) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%s vanishes into the forest.\r\n", player->name);
            player_broadcast_room(player->current_room, msg, NULL);
            player_remove(player->sockfd);
        }

        close(sse_clients[i].sockfd);
        sse_clients[i].active = false;
        sse_clients[i].token[0] = '\0';
    }
}

static void sse_keepalive(void)
{
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i].active) {
            send(sse_clients[i].sockfd, ": keepalive\n\n", 13, MSG_NOSIGNAL);
        }
    }
}

static void handle_get_root(int sock)
{
    http_response(sock, 200, "OK", "text/html", WEB_UI_HTML, NULL);
}

static void handle_generate_204(int sock)
{
    char resp[] = "HTTP/1.1 204 No Content\r\n"
                  "Content-Length: 0\r\n"
                  "Connection: close\r\n"
                  "\r\n";
    send(sock, resp, strlen(resp), MSG_NOSIGNAL);
}

static void handle_hotspot_detect(int sock)
{
    http_redirect(sock, "/");
}

static const char *get_param(const char *body, const char *key, char *val, int val_size)
{
    const char *p = strstr(body, key);
    if (!p) { val[0] = '\0'; return NULL; }
    p += strlen(key);
    int i = 0;
    while (*p && *p != '&' && i < val_size - 1) {
        if (*p == '+') { val[i++] = ' '; p++; }
        else if (*p == '%' && p[1] && p[2]) {
            char hex[3] = {p[1], p[2], 0};
            val[i++] = (char)strtol(hex, NULL, 16);
            p += 3;
        } else {
            val[i++] = *p++;
        }
    }
    val[i] = '\0';
    return val;
}

static void handle_post_login(int sock, const char *body)
{
    char name[MAX_NAME_LEN] = {0};
    get_param(body, "name=", name, sizeof(name));

    if (strlen(name) == 0 || !player_is_valid_name(name)) {
        http_response(sock, 400, "Bad Request", "application/json",
                      "{\"error\":\"invalid_name\"}", NULL);
        return;
    }

    if (player_name_exists(name)) {
        http_response(sock, 409, "Conflict", "application/json",
                      "{\"error\":\"name_taken\"}", NULL);
        return;
    }

    if (player_get_active_count() >= MAX_PLAYERS) {
        http_response(sock, 503, "Full", "application/json",
                      "{\"error\":\"server_full\"}", NULL);
        return;
    }

    int idx = player_add(-1);
    if (idx < 0) {
        http_response(sock, 500, "Error", "application/json",
                      "{\"error\":\"internal\"}", NULL);
        return;
    }

    player_t *player = player_get_by_index(idx);
    if (!player) {
        http_response(sock, 500, "Error", "application/json",
                      "{\"error\":\"internal\"}", NULL);
        return;
    }

    char token[SESSION_TOKEN_LEN];
    generate_token(token, sizeof(token));

    player->state = PLAYER_STATE_PLAYING;
    player->current_room = ROOM_FOREST_PATH;
    player->transport = TRANSPORT_WEB;
    player->close_fn = NULL;
    strncpy(player->name, name, MAX_NAME_LEN - 1);
    strncpy(player->session_token, token, SESSION_TOKEN_LEN - 1);

    char arrival_msg[128];
    snprintf(arrival_msg, sizeof(arrival_msg), "%s arrives at the Forest Path.\r\n", player->name);
    player_broadcast_room(player->current_room, arrival_msg, player);

    char response[1536];
    snprintf(response, sizeof(response),
        "{\"token\":\"%s\","
        "\"motd\":\"\\r\\n"
        "  | |    (*) | | | | |  \\r\\n"
        "  | |     *| |*| |*| | ___ \\r\\n"
        "  | |    | | __| __| |/ _ \\\\\\r\\n"
        "  | |__**| | |*| |*| |  __/\\r\\n"
        "  |******|*|_*|_*|*|_||   \\r\\n"
        "\\r\\n"
        "   LITTLE RED RACCOON MUD\\r\\n"
        "\\r\\n"
        "*** Message of the Day ***\\r\\n"
        "The Raccoon Council reminds you: stealing trash is a victimless crime.\\r\\n"
        "The Wolf has been notified. He does not care.\\r\\n"
        "******************************\"}", token);

    http_response(sock, 200, "OK", "application/json", response, NULL);
    ESP_LOGI(TAG, "Player '%s' logged in, token %s", name, token);
}

static void handle_post_command(int sock, const char *body)
{
    char token[SESSION_TOKEN_LEN] = {0};
    char cmd[256] = {0};
    get_param(body, "token=", token, sizeof(token));
    get_param(body, "command=", cmd, sizeof(cmd));

    if (strlen(token) == 0) {
        http_response(sock, 401, "Unauthorized", "application/json",
                      "{\"error\":\"no_token\"}", NULL);
        return;
    }

    player_t *player = player_get_by_token(token);
    if (!player) {
        http_response(sock, 401, "Unauthorized", "application/json",
                      "{\"error\":\"invalid_token\"}", NULL);
        return;
    }

    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
        command_process(player, cmd);
        http_response(sock, 200, "OK", "application/json",
                      "{\"status\":\"quit\"}", NULL);
        return;
    }

    command_process(player, cmd);
    http_response(sock, 200, "OK", "application/json",
                  "{\"status\":\"ok\"}", NULL);
}

static int handle_get_events(int sock, const char *query)
{
    char token[SESSION_TOKEN_LEN] = {0};
    if (query) {
        const char *t = strstr(query, "token=");
        if (t) get_param(t, "token=", token, sizeof(token));
    }

    if (strlen(token) == 0) {
        http_response(sock, 401, "Unauthorized", "application/json",
                      "{\"error\":\"no_token\"}", NULL);
        close(sock);
        return -1;
    }

    player_t *player = player_get_by_token(token);
    if (!player) {
        http_response(sock, 401, "Unauthorized", "application/json",
                      "{\"error\":\"invalid_token\"}", NULL);
        close(sock);
        return -1;
    }

    char hdr[512];
    int hlen = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n");
    send(sock, hdr, hlen, MSG_NOSIGNAL);

    int sse_idx = -1;
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (!sse_clients[i].active) {
            sse_idx = i;
            break;
        }
    }

    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i].active &&
            strcmp(sse_clients[i].token, token) == 0) {
            close(sse_clients[i].sockfd);
            sse_clients[i].sockfd = sock;
            player->sockfd = sock;
            player->send_fn = sse_send_to_player;
            player->close_fn = web_close_fn;

            char title_event[SSE_BUF_SIZE];
            snprintf(title_event, sizeof(title_event),
                "data: \\r\\n"
                "data:   | |    (*) | | | | |  \\r\\n"
                "data:   | |     *| |*| |*| | ___ \\r\\n"
                "data:   | |    | | __| __| |/ _ \\\\r\\n"
                "data:   | |__**| | |*| |*| |  __/\\r\\n"
                "data:   |******|*|_*|_*|*|_||   \\r\\n"
                "data: \\r\\n"
                "data:    RED RACCOON MUD\\r\\n"
                "data: \\r\\n\n\n");
            send(sock, title_event, strlen(title_event), MSG_NOSIGNAL);
            command_look(player);
            ESP_LOGI(TAG, "SSE reconnected for player '%s'", player->name);
            return 0;
        }
    }

    if (sse_idx < 0) {
        close(sock);
        return -1;
    }

    sse_clients[sse_idx].sockfd = sock;
    sse_clients[sse_idx].active = true;
    strncpy(sse_clients[sse_idx].token, token, SESSION_TOKEN_LEN - 1);

    set_nonblocking(sock);
    struct timeval zero_tv = {0, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &zero_tv, sizeof(zero_tv));
    player->sockfd = sock;
    player->send_fn = sse_send_to_player;
    player->close_fn = web_close_fn;
    player->transport = TRANSPORT_WEB;

    command_look(player);

    ESP_LOGI(TAG, "SSE connected for player '%s'", player->name);
    return 0;
}

static void http_serve(int client_sock)
{
    char buf[HTTP_BUF_SIZE];
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int total = 0;
    int content_length = -1;
    bool headers_done = false;

    while (total < HTTP_BUF_SIZE - 1) {
        int bytes = recv(client_sock, buf + total, HTTP_BUF_SIZE - 1 - total, 0);
        if (bytes <= 0) {
            if (total == 0) { close(client_sock); return; }
            break;
        }
        total += bytes;
        buf[total] = '\0';

        if (!headers_done) {
            char *end = strstr(buf, "\r\n\r\n");
            if (end) {
                headers_done = true;
                const char *cl = strstr(buf, "Content-Length: ");
                if (cl) content_length = atoi(cl + 16);
                int header_end = (end + 4) - buf;
                int body_received = total - header_end;
                if (content_length > 0 && body_received < content_length) continue;
                break;
            }
        } else {
            if (content_length > 0) {
                const char *end = strstr(buf, "\r\n\r\n");
                if (end) {
                    int body_offset = (end + 4) - buf;
                    if (total - body_offset >= content_length) break;
                }
            }
            break;
        }
    }

    if (total == 0) { close(client_sock); return; }

    buf[total] = '\0';
    char method[16] = {0}, path[256] = {0};
    sscanf(buf, "%15s %255s", method, path);

    const char *body = NULL;
    const char *end = strstr(buf, "\r\n\r\n");
    if (end) body = end + 4;

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            handle_get_root(client_sock);
            close(client_sock);
        } else if (strcmp(path, "/generate_204") == 0) {
            handle_generate_204(client_sock);
            close(client_sock);
        } else if (strcmp(path, "/hotspot-detect.html") == 0) {
            handle_hotspot_detect(client_sock);
            close(client_sock);
        } else if (strncmp(path, "/api/events", 11) == 0) {
            const char *query = strchr(path, '?');
            int ret = handle_get_events(client_sock, query);
            if (ret != 0) close(client_sock);
        } else if (strcmp(path, "/api/connectivity-check") == 0) {
            handle_hotspot_detect(client_sock);
            close(client_sock);
        } else if (strcmp(path, "/success.txt") == 0) {
            handle_generate_204(client_sock);
            close(client_sock);
        } else {
            handle_get_root(client_sock);
            close(client_sock);
        }
    } else if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/login") == 0) {
            handle_post_login(client_sock, body ? body : "");
        } else if (strcmp(path, "/api/command") == 0) {
            handle_post_command(client_sock, body ? body : "");
        } else {
            http_response(client_sock, 404, "Not Found", "text/plain", "Not Found", NULL);
        }
        close(client_sock);
    } else {
        http_response(client_sock, 405, "Method Not Allowed", "text/plain", "Method Not Allowed", NULL);
        close(client_sock);
    }
}

void http_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;

    vTaskDelay(pdMS_TO_TICKS(1500));

    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Failed to create HTTP socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(HTTP_PORT);

    int err = bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "HTTP bind failed on port %d: errno %d", HTTP_PORT, errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    err = listen(listen_sock, 8);
    if (err != 0) {
        ESP_LOGE(TAG, "HTTP listen failed: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "HTTP server listening on port %d", HTTP_PORT);

    memset(sse_clients, 0, sizeof(sse_clients));

    TickType_t last_keepalive = 0;
    TickType_t last_cleanup = 0;

    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);
        int maxfd = listen_sock;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            if (errno == EINTR) continue;
            ESP_LOGW(TAG, "select error: %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (FD_ISSET(listen_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_sock = accept(listen_sock,
                                     (struct sockaddr *)&client_addr, &client_len);
            if (client_sock >= 0) {
                http_serve(client_sock);
            }
        }

        TickType_t now = xTaskGetTickCount();

        if (now - last_keepalive >= pdMS_TO_TICKS(SSE_KEEPALIVE_MS)) {
            sse_keepalive();
            last_keepalive = now;
        }
        if (now - last_cleanup >= pdMS_TO_TICKS(SSE_CLEANUP_MS)) {
            sse_cleanup();
            last_cleanup = now;
        }
    }
}

esp_err_t http_server_init(void)
{
    ESP_LOGI(TAG, "HTTP server will start on port %d when task runs", HTTP_PORT);
    return ESP_OK;
}
