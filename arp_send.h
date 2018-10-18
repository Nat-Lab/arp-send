#include <stdint.h>
#include <pthread.h>

typedef struct {
    u_int16_t op;
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

typedef struct {
    arpinfo_t arp;
    arptags_t ltags;
    libnet_t *lctx;
    unsigned int intv;
    int id;
} arpctx_t;

typedef struct arpctx_list {
    arpctx_t *ctx;
    pthread_t tid;
    struct arpctx_list *next;
} arpctx_list;

void *arp_send(void *req);
void ctxlist_insert(arpctx_list **ctxlist, arpctx_t *ctx);
char* ctx_str(arpinfo_t *ctx);
int ctxlist_insertf(arpctx_list **ctxlist, char *errbuf, char *file);
int ether_parse(uint8_t *dst, char *src);
int arp_fill(libnet_t *lctx, const arpinfo_t arp, arptags_t *tags);
int cmd_parse(int argc, char **argv, char *errbuf, arpctx_list **ctxlist);
arpctx_t* make_arpctx(u_int16_t op, char **argv, int argi, char *errbuf);
