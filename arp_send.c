#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libnet.h>
#include "arp_send.h"

void *arp_sender(void *req) {
    arpctx_t *ctx = (arpctx_t *) req;
    while (1) {
        printf("Bytes Wrote: %d\n", arp_send(ctx->arp, &ctx->ltags, ctx->lctx));
        sleep(ctx->intv);
    }
    return NULL;
}

int arp_send(const arpinfo_t arp, arptags_t *tags, libnet_t *lctx) {
    tags->arp = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        6, 4,
        arp.op,
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

int ether_parse(u_int8_t *dst, char *src) {
    int mac[6];
    if(6 == sscanf(src, "%x:%x:%x:%x:%x:%x%*c", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])) {
         for(int i = 0; i < 6; ++i) dst[i] = (u_int8_t) mac[i];
         return 0;
    }
    return -1;
}

void ctxlist_insert(arpctx_list **ctxlist, arpctx_t *ctx) {
    if(*ctxlist== NULL) {
        *ctxlist = (arpctx_list*) malloc(sizeof(arpctx_list));
        (*ctxlist)->ctx = ctx;
        (*ctxlist)->next = NULL;
        return;
    }
    arpctx_list *ctxptr = *ctxlist;
    while(ctxptr->next != NULL) ctxptr = ctxptr->next;
    ctxptr->next = (arpctx_list*) malloc(sizeof(arpctx_list));
    ctxptr->next->ctx = ctx;
    ctxptr->next->next = NULL;
}

void ctxlist_insertf(arpctx_list *ctxlist, char *file) {
}

arpctx_t* make_arpctx(u_int16_t op, char **argv, int argi, char *errbuf) {
    char dev[IFNAMSIZ];
    strncpy(dev, argv[argi + 1], IFNAMSIZ);

    arpctx_t *ctx = (arpctx_t *) malloc(sizeof(arpctx_t));
    memset(ctx, 0, sizeof(arpctx_t));
    ctx->arp.op = op;
    ctx->intv = atoi(argv[argi + 8]);
    ctx->lctx = libnet_init(LIBNET_LINK, dev, errbuf);

    inet_pton(AF_INET, argv[argi + 4], (void *) &ctx->arp.ip_src);
    inet_pton(AF_INET, argv[argi + 7], (void *) &ctx->arp.ip_dst);

    ether_parse(ctx->arp.eth_ether_src, argv[argi + 2]);
    ether_parse(ctx->arp.arp_ether_src, argv[argi + 3]);
    ether_parse(ctx->arp.eth_ether_dst, argv[argi + 5]);
    ether_parse(ctx->arp.arp_ether_dst, argv[argi + 6]);

    return ctx;
}

int main (int argc, char **argv) {
    char errbuf[LIBNET_ERRBUF_SIZE];

    arpctx_list *ctxlist = NULL;

    for(int argi = 1; argi < argc; argi++) {
        if(!strcmp(argv[argi], "reply")) {
            if(argc < argi + 9) {
                fprintf(stderr, "Insufficient number of arguments after 'reply', expcet 8, saw %d.\n", argc - argi - 1);
                return 1;
            }
            arpctx_t* ctx = make_arpctx(ARPOP_REPLY, argv, argi, errbuf);
            ctxlist_insert(&ctxlist, ctx);
            argi += 8;
        }
        if(!strcmp(argv[argi], "request")) {
            if(argc < argi + 9) {
                fprintf(stderr, "Insufficient number of arguments after 'request', expcet 8, saw %d.\n", argc - argi - 1);
                return 1;
            }
            arpctx_t* ctx = make_arpctx(ARPOP_REQUEST, argv, argi, errbuf);
            ctxlist_insert(&ctxlist, ctx);
            argi += 8;
        }
        if(!strcmp(argv[argi], "source")) {
            if(argc < argi + 2) {
                fprintf(stderr, "Insufficient number of arguments after 'source', expcet 1, saw %d.\n", argc - argi - 1);
                return 1;
            }
            ctxlist_insertf(ctxlist, argv[++argi]);
        }
    }

    arpctx_list *ctxptr = ctxlist;

    while (ctxptr != NULL) {
        pthread_create(&ctxptr->tid, NULL, arp_sender, (void *) ctxptr->ctx);
        ctxptr = ctxptr->next;
    }

    pthread_join(ctxlist->tid, NULL);
}

