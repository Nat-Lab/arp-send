#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libnet.h>
#include "arp_send.h"

void *arp_send(void *req) {
    arpctx_t *ctx = (arpctx_t *) req;
    int ret;
    if((ret = arp_fill(ctx->lctx, ctx->arp, &ctx->ltags)) < 0) {
        fprintf(stderr, "[CRIT] arp_send: error creating %s: %s\n", ret == -1 ? "ARP pcaket" : "ethernet packet", libnet_geterror(ctx->lctx));
        return NULL;
    }
    while (1) {
        char *info = ctx_str(&ctx->arp);
        fprintf(stderr, "[INFO] send: %s", info);
        free(info);
        if (libnet_write(ctx->lctx) < 0) fprintf(stderr, "[WARN] arp_send: can't send: %s.\n", libnet_geterror(ctx->lctx));
        usleep(ctx->intv * 1000);
    }
    return NULL;
}

int arp_fill(libnet_t *lctx, const arpinfo_t arp, arptags_t *tags) {
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

    return 0;
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

int ctxlist_insertf(arpctx_list **ctxlist, char *errbuf, char *file) {
    FILE *fp = fopen(file, "r");
    if(!fp) {
        fprintf(stderr, "[CRIT] ctxlist_insertf: can not open file: \"%s\"", file);
        return -1;
    }

    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);

    char *buf = (char *) malloc(sz+1);

    fread(buf, sz, 1, fp);
    fclose(fp);

    char **argv = (char **) malloc(8192 * sizeof(char *)), *cur;
    int i = 0;
    while((cur = strsep(&buf, " \n\t")) != NULL) {
        if(i >= 8192) {
            fprintf(stderr, "[WARN] ctxlist_insertf: too many commands in file: \"%s\".", file);
            break;
        }
        argv[i] = (char *) malloc(strlen(cur) + 1);
        strcpy(argv[i++], cur);
    };

    free(buf);

    return cmd_parse(i, argv, errbuf, ctxlist);
}

arpctx_t* make_arpctx(u_int16_t op, char **argv, int argi, char *errbuf) {
    char dev[IFNAMSIZ];
    strncpy(dev, argv[argi + 1], IFNAMSIZ);

    arpctx_t *ctx = (arpctx_t *) malloc(sizeof(arpctx_t));
    memset(ctx, 0, sizeof(arpctx_t));
    ctx->arp.op = op;
    ctx->intv = atoi(argv[argi + 8]);
    ctx->lctx = libnet_init(LIBNET_LINK, dev, errbuf);

    if(ctx->lctx == NULL) {
        fprintf(stderr, "[CRIT] make_arpctx: libnet_init: %s", errbuf);
        return NULL;
    }

    inet_pton(AF_INET, argv[argi + 4], (void *) &ctx->arp.ip_src);
    inet_pton(AF_INET, argv[argi + 7], (void *) &ctx->arp.ip_dst);

    int errc = 0;
    errc += ether_parse(ctx->arp.eth_ether_src, argv[argi + 2]);
    errc += ether_parse(ctx->arp.arp_ether_src, argv[argi + 3]);
    errc += ether_parse(ctx->arp.eth_ether_dst, argv[argi + 5]);
    errc += ether_parse(ctx->arp.arp_ether_dst, argv[argi + 6]);

    if(errc != 0) {
        fprintf(stderr, "[CRIT] make_arpctx: ether_parse: Invalid argument.");
        return NULL;
    }

    return ctx;
}

int cmd_parse(int argc, char **argv, char *errbuf, arpctx_list **ctxlist) {
    for(int argi = 0; argi < argc; argi++) {
        if(!strcmp(argv[argi], "reply")) {
            if(argc < argi + 9) {
                fprintf(stderr, "[CRIT] cmd_parse: insufficient number of arguments after 'reply', expcet 8, saw %d.\n", argc - argi - 1);
                return -1;
            }
            arpctx_t* ctx = make_arpctx(ARPOP_REPLY, argv, argi, errbuf);
            if(ctx == NULL) {
                fprintf(stderr, "[CRIT] cmd_parse: make_arpctx: invalid argument.\n");
                return -1;
            }
            char *info = ctx_str(&ctx->arp);
            fprintf(stderr, "[INFO] cmd_parse: added %s.", info);
            free(info);
            ctxlist_insert(ctxlist, ctx);
            argi += 8;
            continue;
        }
        if(!strcmp(argv[argi], "request")) {
            if(argc < argi + 9) {
                fprintf(stderr, "[CRIT] cmd_parse: insufficient number of arguments after 'request', expcet 8, saw %d.\n", argc - argi - 1);
                return -1;
            }
            arpctx_t* ctx = make_arpctx(ARPOP_REQUEST, argv, argi, errbuf);
            if(ctx == NULL) {
                fprintf(stderr, "[CRIT] cmd_parse: make_arpctx: invalid argument.\n");
                return -1;
            }
            char *info = ctx_str(&ctx->arp);
            fprintf(stderr, "[INFO] cmd_parse: added %s.", info);
            free(info);
            ctxlist_insert(ctxlist, ctx);
            argi += 8;
            continue;
        }
        if(!strcmp(argv[argi], "source")) {
            if(argc < argi + 2) {
                fprintf(stderr, "[CRIT] cmd_parse: insufficient number of arguments after 'source', expcet 1, saw %d.\n", argc - argi - 1);
                return -1;
            }
            if(ctxlist_insertf(ctxlist, errbuf, argv[++argi]) < 0) {
                fprintf(stderr, "[CRIT] cmd_parse: ctxlist_insertf: invalid argument.\n");
                return -1;
            }
            continue;
        }
    }

    return 0;
}

char* ctx_str(arpinfo_t *ctx) {
    char *buf = (char *) malloc(256);

    sprintf(
        buf,
        "%s %02x:%02x:%02x:%02x:%02x:%02x/%d.%d.%d.%d @ %02x:%02x:%02x:%02x:%02x:%02x => %02x:%02x:%02x:%02x:%02x:%02x/%d.%d.%d.%d @ %02x:%02x:%02x:%02x:%02x:%02x\n",
        ctx->op == ARPOP_REPLY ? "REPLY" : "REQUEST",
        ctx->arp_ether_src[0], ctx->arp_ether_src[1], ctx->arp_ether_src[2], ctx->arp_ether_src[3], ctx->arp_ether_src[4], ctx->arp_ether_src[5],
        ctx->ip_src & 0xFF,(ctx->ip_src >> 8) & 0xFF, (ctx->ip_src >> 16) & 0xFF, (ctx->ip_src >> 24) & 0xFF,
        ctx->eth_ether_src[0], ctx->eth_ether_src[1], ctx->eth_ether_src[2], ctx->eth_ether_src[3], ctx->eth_ether_src[4], ctx->eth_ether_src[5],
        ctx->arp_ether_dst[0], ctx->arp_ether_dst[1], ctx->arp_ether_dst[2], ctx->arp_ether_dst[3], ctx->arp_ether_dst[4], ctx->arp_ether_dst[5],
        ctx->ip_dst & 0xFF,(ctx->ip_dst >> 8) & 0xFF, (ctx->ip_dst >> 16) & 0xFF, (ctx->ip_dst >> 24) & 0xFF,
        ctx->eth_ether_dst[0], ctx->eth_ether_dst[1], ctx->eth_ether_dst[2], ctx->eth_ether_dst[3], ctx->eth_ether_dst[4], ctx->eth_ether_dst[5]
    );

    return buf;
}

void print_help(void) {
    fprintf(stderr, "Usage: arp_send COMMAND [COMMAND...]\n");
    fprintf(stderr, "where COMMAND := { request IFN ETHER_MAC_SRC ARP_MAC_SRC IP_SRC ETHER_MAC_DST ARP_MAC_DST IP_DST INTV |\n");
    fprintf(stderr, "                   reply IFN ETHER_MAC_SRC ARP_MAC_SRC IP_SRC ETHER_MAC_DST ARP_MAC_DST IP_DST INTV |\n");
    fprintf(stderr, "                   source SOURCEFILE }\n");
    fprintf(stderr, "where IFN := Interface name.\n");
    fprintf(stderr, "      ETHER_MAC_SRC := Ethernet source MAC.\n");
    fprintf(stderr, "      ETHER_MAC_DST := Ethernet destination MAC.\n");
    fprintf(stderr, "      ARP_MAC_SRC := ARP source MAC.\n");
    fprintf(stderr, "      ARP_MAC_DST := ARP destination MAC.\n");
    fprintf(stderr, "      IP_SRC := ARP source IP.\n");
    fprintf(stderr, "      IP_DST := ARP destination IP.\n");
    fprintf(stderr, "      INTV := Time in milliseconds to wait between sends.\n");
    fprintf(stderr, "      SOURCEFILE := Name of source file.\n");
}

int main(int argc, char **argv) {
    char errbuf[LIBNET_ERRBUF_SIZE];

    arpctx_list *ctxlist = NULL;

    if (argc < 2) {
        print_help();
        return 0;
    }

    if(cmd_parse(argc, argv, errbuf, &ctxlist) < 0) {
        fprintf(stderr, "[CRIT] main: cmd_parse: invalid argument.\n");
        print_help();
        return 1;
    }

    arpctx_list *ctxptr = ctxlist;

    int i = 0;
    while (ctxptr != NULL) {
        ctxptr->ctx->id = i++;
        pthread_create(&ctxptr->tid, NULL, arp_send, (void *) ctxptr->ctx);
        ctxptr = ctxptr->next;
    }

    ctxptr = ctxlist;

    while (ctxptr != NULL) {
        pthread_join(ctxptr->tid, NULL);
        ctxptr = ctxptr->next;
    }

    if(ctxlist == NULL) print_help();

    return 0;
}

