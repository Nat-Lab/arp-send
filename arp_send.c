#include <stdio.h>
#include <stdlib.h>
#include <libnet.h>
#include "arp_send.h"

int arp_send(const arpinfo_t arp, arptags_t *tags, libnet_t *lctx) {
    tags->arp = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        6, 4,
        ARPOP_REPLY,
        arp.arp_ether_src,
        (u_int8_t *) &arp.ip_src,
        arp.arp_ether_dst,
        (u_int8_t *) &arp.ip_dst,
        NULL, 0, lctx, tags->arp
    );

    if(tags->arp == -1) return -1;

    tags->ether = libnet_build_ethernet(
        arp.eth_ether_dst,
        arp.eth_ether_src,
        ETHERTYPE_ARP,
        NULL, 0, lctx, tags->ether
    );

    if(tags->ether == -1) return -2;

    int bytes_written = libnet_write(lctx);

    if(bytes_written == -1) return -3;

    return bytes_written;
}

int main (void) {
    char *dev = "en0";
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_t *lctx = libnet_init(LIBNET_LINK, dev, errbuf);

    arpinfo_t *arp = (arpinfo_t *) malloc(sizeof(arpinfo_t));
    arptags_t *tags = (arptags_t *) malloc(sizeof(arptags_t));

    memset(arp->eth_ether_src, 0xff, 6 * sizeof (uint8_t));
    memset(arp->eth_ether_dst, 0xff, 6 * sizeof (uint8_t));
    memset(arp->arp_ether_src, 0xff, 6 * sizeof (uint8_t));
    memset(arp->arp_ether_dst, 0xff, 6 * sizeof (uint8_t));

    inet_pton(AF_INET, "1.0.0.1", (void *) &arp->ip_src);
    inet_pton(AF_INET, "1.0.0.2", (void *) &arp->ip_dst);

    printf("Bytes written: %d\n", arp_send(*arp, tags, lctx));
}

