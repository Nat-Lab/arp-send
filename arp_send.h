#include <stdint.h>

typedef struct {
    u_int8_t eth_ether_src[6];
    u_int8_t eth_ether_dst[6];
    u_int8_t arp_ether_src[6];
    u_int8_t arp_ether_dst[6];
    u_int32_t ip_src;
    u_int32_t ip_dst;
} arpinfo_t;

typedef struct {
    libnet_ptag_t ether;
    libnet_ptag_t arp;
} arptags_t;

int arp_send(const arpinfo_t arp, arptags_t *tags, libnet_t *lctx);
