/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>

#include "lwip/opt.h"

#include "lwip/init.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/dns.h"

#include "lwip/stats.h"

#include "lwip/tcp_impl.h"
#include "lwip/inet_chksum.h"

#include "lwip/tcpip.h"
#include "lwip/sockets.h"

#include "netif/tapif.h"
#include "netif/tunif.h"

#include "netif/unixif.h"
#include "netif/dropif.h"
#include "netif/pcapif.h"

#include "netif/tcpdump.h"

#if PPP_SUPPORT
#include "netif/ppp/pppos.h"
#define PPP_PTY_TEST 1
#include <termios.h>
#endif

#include "lwip/ip_addr.h"
#include "arch/perf.h"

#include "httpd.h"
#include "udpecho.h"
#include "tcpecho.h"
#include "shell.h"

#if LWIP_RAW
#include "lwip/icmp.h"
#include "lwip/raw.h"
#endif

/* (manual) host IP configuration */
static ip4_addr_t ipaddr, netmask, gw;

/* ping out destination cmd option */
static unsigned char ping_flag;
static ip4_addr_t ping_addr;

/* nonstatic debug cmd option, exported in lwipopts.h */
unsigned char debug_flags;

/** @todo add options for selecting netif, starting DHCP client etc */
static struct option longopts[] = {
  /* turn on debugging output (if build with LWIP_DEBUG) */
  {"debug", no_argument,        NULL, 'd'},
  /* help */
  {"help", no_argument, NULL, 'h'},
  /* gateway address */
  {"gateway", required_argument, NULL, 'g'},
  /* ip address */
  {"ipaddr", required_argument, NULL, 'i'},
  /* netmask */
  {"netmask", required_argument, NULL, 'm'},
  /* ping destination */
  {"ping",   required_argument, NULL, 'p'},
  /* new command line options go here! */
  {NULL,   0,                 NULL,  0}
};
#define NUM_OPTS ((sizeof(longopts) / sizeof(struct option)) - 1)

static void init_netifs(void);

static void usage(void)
{
  unsigned char i;

  printf("options:\n");
  for (i = 0; i < NUM_OPTS; i++) {
    printf("-%c --%s\n",longopts[i].val, longopts[i].name);
  }
}

#if 0
static void
tcp_debug_timeout(void *data)
{
  LWIP_UNUSED_ARG(data);
#if TCP_DEBUG
  tcp_debug_print_pcbs();
#endif /* TCP_DEBUG */
  sys_timeout(5000, tcp_debug_timeout, NULL);
}
#endif

static void
tcpip_init_done(void *arg)
{
  sys_sem_t *sem;
  sem = (sys_sem_t *)arg;

  init_netifs();

  sys_sem_signal(sem);
}

#if PPP_SUPPORT
static void
ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
{
    struct netif *pppif = ppp_netif(pcb);
    LWIP_UNUSED_ARG(ctx);

    switch(err_code) {
    case PPPERR_NONE:               /* No error. */
        {
#if LWIP_DNS
        ip_addr_t ns;
#endif /* LWIP_DNS */
        fprintf(stderr, "ppp_link_status_cb: PPPERR_NONE\n\r");
        fprintf(stderr, "   our_ipaddr  = %s\n\r", ipaddr_ntoa(&pppif->ip_addr));
        fprintf(stderr, "   his_ipaddr  = %s\n\r", ipaddr_ntoa(&pppif->gw));
        fprintf(stderr, "   netmask     = %s\n\r", ipaddr_ntoa(&pppif->netmask));
#if LWIP_DNS
        ns = dns_getserver(0);
        fprintf(stderr, "   dns1        = %s\n\r", ipaddr_ntoa(&ns));
        ns = dns_getserver(1);
        fprintf(stderr, "   dns2        = %s\n\r", ipaddr_ntoa(&ns));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
        fprintf(stderr, "   our6_ipaddr = %s\n\r", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
        }
        break;

    case PPPERR_PARAM:             /* Invalid parameter. */
        printf("ppp_link_status_cb: PPPERR_PARAM\n");
        break;

    case PPPERR_OPEN:              /* Unable to open PPP session. */
        printf("ppp_link_status_cb: PPPERR_OPEN\n");
        break;

    case PPPERR_DEVICE:            /* Invalid I/O device for PPP. */
        printf("ppp_link_status_cb: PPPERR_DEVICE\n");
        break;

    case PPPERR_ALLOC:             /* Unable to allocate resources. */
        printf("ppp_link_status_cb: PPPERR_ALLOC\n");
        break;

    case PPPERR_USER:              /* User interrupt. */
        printf("ppp_link_status_cb: PPPERR_USER\n");
        break;

    case PPPERR_CONNECT:           /* Connection lost. */
        printf("ppp_link_status_cb: PPPERR_CONNECT\n");
        break;

    case PPPERR_AUTHFAIL:          /* Failed authentication challenge. */
        printf("ppp_link_status_cb: PPPERR_AUTHFAIL\n");
        break;

    case PPPERR_PROTOCOL:          /* Failed to meet protocol. */
        printf("ppp_link_status_cb: PPPERR_PROTOCOL\n");
        break;

    case PPPERR_PEERDEAD:          /* Connection timeout. */
        printf("ppp_link_status_cb: PPPERR_PEERDEAD\n");
        break;

    case PPPERR_IDLETIMEOUT:       /* Idle Timeout. */
        printf("ppp_link_status_cb: PPPERR_IDLETIMEOUT\n");
        break;

    case PPPERR_CONNECTTIME:       /* PPPERR_CONNECTTIME. */
        printf("ppp_link_status_cb: PPPERR_CONNECTTIME\n");
        break;

    case PPPERR_LOOPBACK:          /* Connection timeout. */
        printf("ppp_link_status_cb: PPPERR_LOOPBACK\n");
        break;

    default:
        printf("ppp_link_status_cb: unknown errCode %d\n", err_code);
        break;
    }
}
#endif

/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
#if LWIP_RAW

static int seq_num;

#if 0
/* Ping using the raw api */
static int
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, ip4_addr_t *addr)
{
  printf("ping recv\n");
  return 1; /* eat the event */
}

static void
ping_send(struct raw_pcb *raw, ip4_addr_t *addr)
{
  struct pbuf *p;
  struct icmp_echo_hdr *iecho;

  p = pbuf_alloc(PBUF_IP,sizeof(struct icmp_echo_hdr),PBUF_RAM);
  if (!p) return;

  iecho = p->payload;
  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->seqno = htons(seq_num);

  iecho->chksum = inet_chksum(iecho, p->len);
  raw_send_to(raw,p,addr);

  pbuf_free(p);

  seq_num++;
}

static void
ping_thread(void *arg)
{
  struct raw_pcb *raw;

  if (!(raw = raw_new(IP_PROTO_ICMP))) return;

  raw_recv(raw,ping_recv,NULL);

  while (1)
  {
    printf("ping send\n");
    ping_send(raw,&ping_addr);
    sleep(1);
  }
  /* Never reaches this */
  raw_remove(raw);
}
#else
/* Ping using the socket api */

static void
ping_send(int s, ip4_addr_t *addr)
{
  struct icmp_echo_hdr *iecho;
  struct sockaddr_in to;

  if (!(iecho = (struct icmp_echo_hdr *)malloc(sizeof(struct icmp_echo_hdr))))
    return;

  ICMPH_TYPE_SET(iecho,ICMP_ECHO);
  iecho->chksum = 0;
  iecho->seqno = htons(seq_num);
  iecho->chksum = inet_chksum(iecho, sizeof(*iecho));

  to.sin_len = sizeof(to);
  to.sin_family = AF_INET;
  to.sin_addr.s_addr = addr->addr;

  lwip_sendto(s,iecho,sizeof(*iecho),0,(struct sockaddr*)&to,sizeof(to));

  free(iecho);
  seq_num++;
}

static void
ping_recv(int s, ip4_addr_t *addr)
{
  char buf[200];
  socklen_t fromlen;
  int len;
  struct sockaddr_in from;
  LWIP_UNUSED_ARG(addr);

  len = lwip_recvfrom(s, buf,sizeof(buf),0,(struct sockaddr*)&from,&fromlen);

  printf("Received %d bytes from %x\n",len,ntohl(from.sin_addr.s_addr));
}

static void
ping_thread(void *arg)
{
  int s;
  LWIP_UNUSED_ARG(arg);

  if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
    return;
  }

  while (1) {
    printf("sending ping\n");
    ping_send(s,&ping_addr);
    ping_recv(s,&ping_addr);
    sleep(1);
  }
}
#endif

#endif

struct netif netif;
#if PPP_SUPPORT
struct netif pppos_netif;
#endif

static void
init_netifs(void)
{
#if PPP_SUPPORT
  sio_fd_t ppp_sio;
  ppp_pcb *ppp;
#if PPP_PTY_TEST
  ppp_sio = sio_open(2);
#else
  ppp_sio = sio_open(0);
#endif
  if(!ppp_sio)
  {
      perror("Error opening device: ");
      exit(1);
  }

  ppp = pppos_create(&pppos_netif, ppp_sio, ppp_link_status_cb, NULL);
  if (!ppp)
  {
      printf("Could not create PPP control interface");
      exit(1);
  }
#ifdef LWIP_PPP_CHAP_TEST
  ppp_set_auth(ppp, PPPAUTHTYPE_CHAP, "lwip", "mysecret");
#endif

  ppp_connect(ppp, 0);
#endif /* PPP_SUPPORT */
  
#if LWIP_DHCP
  IP4_ADDR(&gw, 0,0,0,0);
  IP4_ADDR(&ipaddr, 0,0,0,0);
  IP4_ADDR(&netmask, 0,0,0,0);
#endif
  
  netif_set_default(netif_add(&netif,&ipaddr, &netmask, &gw, NULL, tapif_init,
                  tcpip_input));
  netif_set_up(&netif);
#if LWIP_DHCP
  dhcp_start(&netif);
#endif
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&netif, 1);
#endif

#if 0
  /* Only used for testing purposes: */
  netif_add(&ipaddr, &netmask, &gw, NULL, pcapif_init, tcpip_input);
#endif
  
#if LWIP_TCP  
  tcpecho_init();
  shell_init();
  httpd_init();
#endif
#if LWIP_UDP  
  udpecho_init();
#endif  
  /*  sys_timeout(5000, tcp_debug_timeout, NULL);*/
}

/*-----------------------------------------------------------------------------------*/
static void
main_thread(void *arg)
{
  sys_sem_t sem;
  LWIP_UNUSED_ARG(arg);

  if(sys_sem_new(&sem, 0) != ERR_OK) {
    LWIP_ASSERT("Failed to create semaphore", 0);
  }
  tcpip_init(tcpip_init_done, &sem);
  sys_sem_wait(&sem);
  printf("TCP/IP initialized.\n");

#if LWIP_RAW
  /** @todo remove dependency on RAW PCB support */
  if(ping_flag) {
    sys_thread_new("ping_thread", ping_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
  }
#endif

  printf("Applications started.\n");


#ifdef MEM_PERF
  mem_perf_init("/tmp/memstats.client");
#endif /* MEM_PERF */
#if 0
    stats_display();
#endif
  /* Block forever. */
  sys_sem_wait(&sem);
}
/*-----------------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  int ch;
  char ip_str[16] = {0}, nm_str[16] = {0}, gw_str[16] = {0};

  /* startup defaults (may be overridden by one or more opts) */
  IP4_ADDR(&gw, 192,168,0,1);
  IP4_ADDR(&netmask, 255,255,255,0);
  IP4_ADDR(&ipaddr, 192,168,0,2);
  
  ping_flag = 0;
  /* use debug flags defined by debug.h */
  debug_flags = LWIP_DBG_OFF;
  
  while ((ch = getopt_long(argc, argv, "dhg:i:m:p:", longopts, NULL)) != -1) {
    switch (ch) {
      case 'd':
        debug_flags |= (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
        break;
      case 'h':
        usage();
        exit(0);
        break;
      case 'g':
        ip4addr_aton(optarg, &gw);
        break;
      case 'i':
        ip4addr_aton(optarg, &ipaddr);
        break;
      case 'm':
        ip4addr_aton(optarg, &netmask);
        break;
      case 'p':
        ping_flag = !0;
        ip4addr_aton(optarg, &ping_addr);
        strncpy(ip_str,ip4addr_ntoa(&ping_addr),sizeof(ip_str));
        printf("Using %s to ping\n", ip_str);
        break;
      default:
        usage();
        break;
    }
  }
  argc -= optind;
  argv += optind;

  strncpy(ip_str,ip4addr_ntoa(&ipaddr),sizeof(ip_str));
  strncpy(nm_str,ip4addr_ntoa(&netmask),sizeof(nm_str));
  strncpy(gw_str,ip4addr_ntoa(&gw),sizeof(gw_str));
  printf("Host at %s mask %s gateway %s\n", ip_str, nm_str, gw_str);

#ifdef PERF
  perf_init("/tmp/simhost.perf");
#endif /* PERF */

  printf("System initialized.\n");
    
  sys_thread_new("main_thread", main_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
  pause();
  return 0;
}
/*-----------------------------------------------------------------------------------*/








