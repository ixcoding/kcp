#include "ikcp.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

struct {
    char *buf;
    int len;
} dst[10];

static int index = 0;

int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    printf("send index: %d, len:%d\n", index, len);
     dst[index].buf = malloc(len);
     dst[index].len = len;
     memcpy(dst[index].buf, buf, len);
     
     index++;
}


/* get clock in millisecond 64 */
static inline long long current()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    
    return time.tv_sec*1000 + time.tv_usec/1000;
}


int main(int argc, char *argv[])
{
    for (int i=0; i<10; i++) {
         dst[i].buf = NULL;
         dst[i].len = 0;
    }
    
    ikcpcb *kcp = ikcp_create(0x11223344, (void*)0);

    
    ikcp_setoutput(kcp, udp_output);
     ikcp_nodelay(kcp, 0, 10, 0, 0);
    
    char src[3000];
    for (int i=0; i<1000; i++) {
        src[i] = 0;        
    }
    for (int i=0; i<1000; i++) {
        src[1000+i] = 1;        
    }
    for (int i=0; i<1000; i++) {
        src[2000+i] = 2;        
    }
    ikcp_update(kcp, current());
    ikcp_send(kcp, src, 3000, 1);
    
    kcp->nocwnd = 32;
    ikcp_flush(kcp);

    if (dst[0].len) 
        ikcp_input(kcp, dst[0].buf, dst[0].len);     
    if (dst[2].len) 
        ikcp_input(kcp, dst[2].buf, dst[2].len);    
    if (dst[1].len) 
        ikcp_input(kcp, dst[1].buf, dst[1].len);
    
    
    int size = ikcp_peeksize(kcp);
    
    printf("recv size: %d\n", size);
    
    char *rcv = malloc(size);
    
    ikcp_recv(kcp, rcv, size);
    
    assert(size==3000);
     
    for (int i=0; i<size; i++)     
        assert(rcv[i]==src[i]);
   
    printf("test succ\n");
    
}