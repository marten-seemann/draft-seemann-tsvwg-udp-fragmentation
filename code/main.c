#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#define SERVER_IPV4 "8.8.8.8"
#define SERVER_IPV6 "2001:4860:4860::8888"
#define SERVER_PORT 12345
#define PACKET_COUNT 5
#define PACKET_INTERVAL 1 // seconds
#define PACKET_SIZE 2000

typedef enum {
    IPV4,
    IPV6,
    DUAL
} network_type;

int main(int argc, char *argv[]) {
    network_type net_type = -1;

    if(argc >= 2) {
        if (strcmp(argv[1], "ipv4") == 0) {
            net_type = IPV4;
        } else if (strcmp(argv[1], "ipv6") == 0) {
            net_type = IPV6;
        } else if (strcmp(argv[1], "dual") == 0) {
            net_type = DUAL;
        }
    }
    if(net_type == -1) {
        printf("usage: %s <ipv4 / ipv6 / dual> <set socket options (true/false)>\n", argv[0]);
        return 1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4);
    server_addr.sin_port = htons(SERVER_PORT);

    struct sockaddr_in6 server_addrv6;
    memset(&server_addrv6, 0, sizeof(server_addrv6));
    server_addrv6.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, SERVER_IPV6, &server_addrv6.sin6_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }
    server_addrv6.sin6_port = htons(SERVER_PORT);

    int sock, opt_val;
    if(net_type == IPV4) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("socket");
            return 1;
        }
    }
    if(net_type == IPV6 || net_type == DUAL) {
        sock = socket(AF_INET6, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("socket");
            return 1;
        }
    }

    if(net_type == IPV4 || net_type == DUAL) {
        opt_val = IP_PMTUDISC_DO;
        if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt");
            close(sock);
            return 1;
        }
    }
    if(net_type == IPV6 || net_type == DUAL) {
        opt_val = 1;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_DONTFRAG, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt");
            close(sock);
            return 1;
        }
    }

    for (int i = 0; i < PACKET_COUNT; i++) {
        char packet[PACKET_SIZE];
        memset(packet, 42, PACKET_SIZE);

        if(net_type == IPV4 || net_type == DUAL) {
            printf("Sending IPv4 packet #%d\n", i+1);
            if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                perror("sendto");
            }
        }
        if(net_type == IPV6 || net_type == DUAL) {
            printf("Sending IPv6 packet #%d\n", i+1);
            if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addrv6, sizeof(server_addrv6)) < 0) {
                perror("sendto");
            }
        }
        sleep(PACKET_INTERVAL);
    }

    close(sock);
    return 0;
}
