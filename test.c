/*  gcc -o test test.c ikcp.c -std=c99 */

#include "ikcp.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

struct {
    char *buf;
    int len;
} dst[100];
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

static IUINT64 gusn = 0;
int on_ack(struct IKCPCB *kcp, IUINT64 usn)
{
    printf("on_ack usn:%llx\n", usn);
    gusn = usn;
    return 0;
}

void test_on_ack()
{
    index = 0;
    ikcpcb *kcp = ikcp_create(0x11223344, (void*)0);
    ikcpcb *kcp2 = ikcp_create(0x11223344, (void*)1);

    ikcp_setoutput(kcp, udp_output);
    ikcp_setoutput(kcp2, udp_output);
    ikcp_setack(kcp, on_ack);
    ikcp_setack(kcp2, on_ack);
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
    ikcp_update(kcp2, current());
    IUINT64 usn = 0x1001;
    ikcp_send(kcp, src, 3000, usn);

    kcp->nocwnd = 32;
    ikcp_flush(kcp);

    if (dst[0].len) 
        ikcp_input(kcp2, dst[0].buf, dst[0].len);
    assert(gusn!=usn);
    if (dst[2].len) 
        ikcp_input(kcp2, dst[2].buf, dst[2].len); 
    assert(gusn!=usn);   
    if (dst[1].len) 
        ikcp_input(kcp2, dst[1].buf, dst[1].len);    
    
    int size = ikcp_peeksize(kcp2);
    
    printf("recv size: %d\n", size);

    assert(size==3000);
    
    char *rcv = malloc(size);
    
    ikcp_recv(kcp2, rcv, size);

    for (int i=0; i<size; i++)     
        assert(rcv[i]==src[i]);

    printf("kcp2 send\n");
    ikcp_send(kcp2, "0", 1, usn+2);

    ikcp_flush(kcp2);

    assert(gusn!=usn);

    if (dst[3].len) {
        ikcp_input(kcp, dst[3].buf, dst[3].len);
    }
    assert(gusn==usn);
    ikcp_send(kcp, "1", 1, usn+3);
    ikcp_flush(kcp);

    if (dst[4].len)
        ikcp_input(kcp2, dst[4].buf, dst[4].len);
    assert(gusn==usn+2);

}

void test_unorded_recv()
{
        for (int i=0; i<10; i++) {
         dst[i].buf = NULL;
         dst[i].len = 0;
    }
    
    ikcpcb *kcp = ikcp_create(0x11223344, (void*)0);

    
    ikcp_setoutput(kcp, udp_output);
    ikcp_setack(kcp, on_ack);
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
    ikcp_send(kcp, src, 3000, 0x1001);
    //ikcp_send(kcp, &src[1000], 1000, 0x1002);
    //ikcp_send(kcp, &src[2000], 1000, 0x1003);

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


int main(int argc, char *argv[])
{
    test_unorded_recv();
    test_on_ack();

    return 0;
    
}