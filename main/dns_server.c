#include "dns_server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/ip4_addr.h"

static const char *TAG = "dns";

#define DNS_PORT 53
#define DNS_MAX_MSG 512

typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

typedef struct __attribute__((packed)) {
    uint16_t type;
    uint16_t class;
} dns_question_tail_t;

typedef struct __attribute__((packed)) {
    uint16_t name_ptr;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
    uint32_t addr;
} dns_answer_t;

static uint32_t s_dns_ip = 0;

static int parse_dns_name(const uint8_t *data, int data_len, int offset, char *name, int name_len)
{
    int pos = offset;
    int written = 0;
    while (pos < data_len) {
        uint8_t len = data[pos];
        if (len == 0) {
            pos++;
            break;
        }
        if ((len & 0xC0) == 0xC0) {
            pos += 2;
            break;
        }
        pos++;
        if (written > 0 && written < name_len - 1) {
            name[written++] = '.';
        }
        for (int i = 0; i < len && pos < data_len && written < name_len - 1; i++) {
            name[written++] = data[pos++];
        }
    }
    if (written < name_len) name[written] = '\0';
    return pos - offset;
}

static void build_dns_response(const uint8_t *req, int req_len, uint8_t *resp, int *resp_len)
{
    if (req_len < (int)sizeof(dns_header_t)) {
        *resp_len = 0;
        return;
    }

    const dns_header_t *req_hdr = (const dns_header_t *)req;
    dns_header_t *resp_hdr = (dns_header_t *)resp;

    uint16_t qdcount = ntohs(req_hdr->qdcount);
    if (qdcount == 0) {
        *resp_len = 0;
        return;
    }

    resp_hdr->id = req_hdr->id;
    resp_hdr->flags = htons(0x8580);
    resp_hdr->qdcount = htons(1);
    resp_hdr->ancount = htons(1);
    resp_hdr->nscount = 0;
    resp_hdr->arcount = 0;

    int offset = sizeof(dns_header_t);
    char qname[256];
    int consumed = parse_dns_name(req, req_len, offset, qname, sizeof(qname));
    if (consumed <= 0) {
        *resp_len = 0;
        return;
    }

    int qname_len = 0;
    {
        int p = offset;
        while (p < req_len) {
            uint8_t len = req[p];
            if (len == 0) { p++; break; }
            if ((len & 0xC0) == 0xC0) { p += 2; break; }
            p += 1 + len;
        }
        qname_len = p - offset;
    }

    memcpy(resp + sizeof(dns_header_t), req + offset, qname_len);
    offset = sizeof(dns_header_t) + qname_len;

    if (offset + (int)sizeof(dns_question_tail_t) > req_len) {
        *resp_len = 0;
        return;
    }

    const dns_question_tail_t *qt = (const dns_question_tail_t *)(req + offset);
    uint16_t qtype = ntohs(qt->type);

    offset += sizeof(dns_question_tail_t);

    dns_answer_t *answer = (dns_answer_t *)(resp + offset);
    answer->name_ptr = htons(0xC000 | (sizeof(dns_header_t)));
    answer->type = htons(qtype);
    answer->class = htons(1);
    answer->ttl = htonl(300);
    answer->rdlength = htons(4);
    answer->addr = s_dns_ip;

    *resp_len = offset + sizeof(dns_answer_t);
}

static uint32_t get_esp_ip(void)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            return ip_info.ip.addr;
        }
    }
    return 0;
}

void dns_server_task(void *pvParameters)
{
    int sock = -1;
    struct sockaddr_in addr;
    uint8_t buf[DNS_MAX_MSG];
    uint8_t resp[DNS_MAX_MSG];

    vTaskDelay(pdMS_TO_TICKS(1000));

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create DNS socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DNS_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (err != 0) {
        ESP_LOGE(TAG, "DNS bind failed on port %d: errno %d", DNS_PORT, errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DNS server listening on port %d", DNS_PORT);

    while (1) {
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);

        s_dns_ip = get_esp_ip();
        if (s_dns_ip == 0) {
            ESP_LOGW(TAG, "No IP yet, waiting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        int bytes = recvfrom(sock, buf, sizeof(buf), 0,
                             (struct sockaddr *)&from, &from_len);
        if (bytes < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                ESP_LOGW(TAG, "DNS recv error: %d", errno);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        int resp_len = 0;
        build_dns_response(buf, bytes, resp, &resp_len);

        if (resp_len > 0) {
            sendto(sock, resp, resp_len, 0,
                   (struct sockaddr *)&from, from_len);
        }
    }
}

esp_err_t dns_server_init(void)
{
    ESP_LOGI(TAG, "DNS server will start on port %d when task runs", DNS_PORT);
    return ESP_OK;
}
