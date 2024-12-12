#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
    // must be defined before including <netinet/in.h>
    #define __APPLE_USE_RFC_3542
#endif
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <netinet/ip6.h>
    #include <arpa/inet.h>
    #include <errno.h>
#endif

#ifdef __linux__
    typedef int sock_optval_t;
    #define IPV4_MTU_DISCOVER_OPTNAME IP_MTU_DISCOVER
    #define IPV4_MTU_DISCOVER_OPTVAL IP_PMTUDISC_PROBE
    #define IPV6_MTU_DISCOVER_OPTNAME IPV6_MTU_DISCOVER
    #define IPV6_MTU_DISCOVER_OPTVAL IPV6_PMTUDISC_PROBE
    #define IPV6_DONTFRAG_OPTNAME IPV6_DONTFRAG
    #define IPV6_DONTFRAG_OPTVAL 1
#elif defined(__APPLE__)
    typedef int sock_optval_t;
    #define IPV4_DONTFRAG_OPTNAME IP_DONTFRAG
    #define IPV4_DONTFRAG_OPTVAL 1
    #define IPV6_DONTFRAG_OPTNAME IPV6_DONTFRAG
    #define IPV6_DONTFRAG_OPTVAL 1
#elif defined(_WIN32)
    typedef char sock_optval_t;
    #define IPV4_DONTFRAG_OPTNAME IP_DONTFRAGMENT
    #define IPV4_DONTFRAG_OPTVAL 1
    #define IPV6_DONTFRAG_OPTNAME IPV6_DONTFRAG
    #define IPV6_DONTFRAG_OPTVAL 1
    #define usleep(x) Sleep((x)/1000)
    #define close(x) closesocket(x)
    #define perror(x) printf("%s: %d\n", x, WSAGetLastError())
#else
    #error "Unsupported platform"
#endif

#define SERVER_IPV4 "8.8.8.8"
#define SERVER_IPV6 "2001:4860:4860::8888"
#define SERVER_PORT 12345
#define PACKET_COUNT 5
#define PACKET_INTERVAL 0.5 // seconds
#define PACKET_SIZE 1000

typedef enum {
    IPV4,
    IPV6,
    DUAL
} network_type;

int main(int argc, char *argv[]) {
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup failed\n");
            return 1;
        }
    #endif

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
    
    // Linux: used for IPv4-only and dual-stack
    // macOS and Windows: only used for IPv4-only, not for dual-stack
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4);
    server_addr.sin_port = htons(SERVER_PORT);

    // on macOS and Windows we need to use an IPv4-mapped IPv6 address
    #ifndef __linux__
        struct sockaddr_in6 server_addr_v6mapped;
        memset(&server_addr_v6mapped, 0, sizeof(server_addr_v6mapped));
        server_addr_v6mapped.sin6_family = AF_INET6;
        server_addr_v6mapped.sin6_port = htons(SERVER_PORT);
        inet_pton(AF_INET6, "::ffff:" SERVER_IPV4, &server_addr_v6mapped.sin6_addr);
    #endif

    // IPv6
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

    // on macOS and Windows, we need to set the IPV6_V6ONLY option to 0 to allow dual-stack sockets
    #ifndef __linux__
        if(net_type == DUAL) {
            sock_optval_t opt = 0;
            if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
                perror("setsockopt IPV6_V6ONLY");
                close(sock);
                return 1;
            }
        }
    #endif

    // macOS dual-stack sockets are configured using IPv6 sockopts only
    int should_set_ipv4_df = 0;
    #if defined(__APPLE__)
        should_set_ipv4_df = (net_type == IPV4);
    #else
        should_set_ipv4_df = (net_type == IPV4 || net_type == DUAL);
    #endif

    if(should_set_ipv4_df) {
        #ifdef __linux__
        sock_optval_t opt_val = IPV4_MTU_DISCOVER_OPTVAL;
        if (setsockopt(sock, IPPROTO_IP, IPV4_MTU_DISCOVER_OPTNAME, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt ipv4 mtu discover");
            close(sock);
            return 1;
        }
        #else
        sock_optval_t opt_val = IPV4_DONTFRAG_OPTVAL;
        if (setsockopt(sock, IPPROTO_IP, IPV4_DONTFRAG_OPTNAME, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt ipv4 dontfrag");
            close(sock);
            return 1;
        }
        #endif
    }

    if(net_type == IPV6 || net_type == DUAL) {
        sock_optval_t opt_val = IPV6_DONTFRAG_OPTVAL;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_DONTFRAG_OPTNAME, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt");
            close(sock);
            return 1;
        }
        #ifdef __linux__
        opt_val = IPV6_MTU_DISCOVER_OPTVAL;
        if (setsockopt(sock, IPPROTO_IP, IPV6_MTU_DISCOVER_OPTNAME, &opt_val, sizeof(opt_val)) < 0) {
            perror("setsockopt ipv6 mtu discover");
            close(sock);
            return 1;
        }
        #endif
    }

    for (int i = 0; i < PACKET_COUNT; i++) {
        char packet[PACKET_SIZE];
        memset(packet, 42, PACKET_SIZE);

        if(net_type == IPV4) {
            printf("Sending IPv4 packet #%d\n", i+1);
            if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                #ifdef _WIN32
                    printf("sendto v4: %d\n", WSAGetLastError());
                #else
                    printf("sendto v4: %s\n", strerror(errno));
                #endif
            }
        }

        if(net_type == DUAL) {
            printf("Sending IPv4 packet #%d\n", i+1);
            // on macOS and Windows, we need to use an IPv4-mapped IPv6 address
            #ifndef __linux__
                if(sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr_v6mapped, sizeof(server_addr_v6mapped)) < 0) {
                    #ifdef _WIN32
                        printf("sendto v4: %d\n", WSAGetLastError());
                    #else
                        printf("sendto v4: %s\n", strerror(errno));
                    #endif
                }
            #else
                if(sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                   printf("sendto v4: %s\n", strerror(errno));
                }
            #endif
        }

        if(net_type == IPV6 || net_type == DUAL) {
            printf("Sending IPv6 packet #%d\n", i+1);
            if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addrv6, sizeof(server_addrv6)) < 0) {
                #ifdef _WIN32
                    printf("sendto v6: %d\n", WSAGetLastError());
                #else
                    printf("sendto v6: %s\n", strerror(errno));
                #endif
            }
        }
        usleep(PACKET_INTERVAL * 1000000);
    }

    close(sock);
    #ifdef _WIN32
        WSACleanup();
    #endif
    return 0;
}
