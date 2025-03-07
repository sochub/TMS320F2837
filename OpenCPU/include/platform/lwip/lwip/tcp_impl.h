/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
#ifndef __LWIP_TCP_IMPL_H__
#define __LWIP_TCP_IMPL_H__

#include "opt.h"

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */

#include "tcp.h"
#include "sys.h"
#include "mem.h"
#include "pbuf.h"
#include "ip.h"
#include "icmp.h"
#include "err.h"
#include "ip6.h"
#include "ip6_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

extern err_t tcp_err_store;

/* Functions for interfacing with TCP: */

/* Lower layer interface to TCP: */
void             tcp_init    (void);  /* Initialize this module. */
/* Must be called every TCP_TMR_INTERVAL ms. (Typically 250 ms). */
void             tcp_tmr     (u32_t tick_diff);
/* It is also possible to call these two functions at the right
   intervals (instead of calling tcp_tmr()). */
void             tcp_slowtmr (void);
void             tcp_fasttmr (void);


/* Only used by IP to pass a TCP segment to TCP: */
void             tcp_input   (struct pbuf *p, struct netif *inp);
/* Used within the TCP code only: */
struct tcp_pcb * tcp_alloc   (u8_t prio);
void             tcp_abandon (struct tcp_pcb *pcb, int reset);
err_t            tcp_send_empty_ack(struct tcp_pcb *pcb);
void             tcp_rexmit  (struct tcp_pcb *pcb);
void             tcp_rexmit_rto  (struct tcp_pcb *pcb);
void             tcp_rexmit_fast (struct tcp_pcb *pcb);
u32_t            tcp_update_rcv_ann_wnd(struct tcp_pcb *pcb);
err_t            tcp_process_refused_data(struct tcp_pcb *pcb);

/**
 * This is the Nagle algorithm: try to combine user data to send as few TCP
 * segments as possible. Only send if
 * - no previously transmitted data on the connection remains unacknowledged or
 * - the TF_NODELAY flag is set (nagle algorithm turned off for this pcb) or
 * - the only unsent segment is at least pcb->mss bytes long (or there is more
 *   than one unsent segment - with lwIP, this can happen although unsent->len < mss)
 * - or if we are in fast-retransmit (TF_INFR)
 */
#define tcp_do_output_nagle(tpcb) ((((tpcb)->unacked == NULL) || \
                            ((tpcb)->flags & (TF_NODELAY | TF_INFR)) || \
                            (((tpcb)->unsent != NULL) && (((tpcb)->unsent->next != NULL) || \
                              ((tpcb)->unsent->len >= (tpcb)->mss))) || \
                            ((tcp_sndbuf(tpcb) == 0) || (tcp_sndqueuelen(tpcb) >= TCP_SND_QUEUELEN)) \
                            ) ? 1 : 0)
#define tcp_output_nagle(tpcb) (tcp_do_output_nagle(tpcb) ? tcp_output(tpcb) : ERR_OK)


#define TCP_SEQ_LT(a,b)     ((s32_t)((u32_t)(a) - (u32_t)(b)) < 0)
#define TCP_SEQ_LEQ(a,b)    ((s32_t)((u32_t)(a) - (u32_t)(b)) <= 0)
#define TCP_SEQ_GT(a,b)     ((s32_t)((u32_t)(a) - (u32_t)(b)) > 0)
#define TCP_SEQ_GEQ(a,b)    ((s32_t)((u32_t)(a) - (u32_t)(b)) >= 0)
/* is b<=a<=c? */
#if 0 /* see bug #10548 */
#define TCP_SEQ_BETWEEN(a,b,c) ((c)-(b) >= (a)-(b))
#endif
#define TCP_SEQ_BETWEEN(a,b,c) (TCP_SEQ_GEQ(a,b) && TCP_SEQ_LEQ(a,c))
#define TCP_FIN 0x01U
#define TCP_SYN 0x02U
#define TCP_RST 0x04U
#define TCP_PSH 0x08U
#define TCP_ACK 0x10U
#define TCP_URG 0x20U
#define TCP_ECE 0x40U
#define TCP_CWR 0x80U
#define TCP_FIN_RST     (TCP_FIN | TCP_RST)
#define TCP_SYN_FIN_RST (TCP_SYN | TCP_FIN | TCP_RST)

/* Valid TCP header flags */
#define TCP_FLAGS 0x3fU

/* Length of the TCP header, excluding options. */
#define TCP_HLEN 20
#define IPSEC_OFF_HLEN  48

#ifndef TCP_TMR_INTERVAL
#define TCP_TMR_INTERVAL       250  /* The TCP timer interval in milliseconds. */
#endif /* TCP_TMR_INTERVAL */

#ifndef TCP_FAST_INTERVAL
#define TCP_FAST_INTERVAL      TCP_TMR_INTERVAL /* the fine grained timeout in milliseconds */
#endif /* TCP_FAST_INTERVAL */

#ifndef TCP_SLOW_INTERVAL
#define TCP_SLOW_INTERVAL      (2*TCP_TMR_INTERVAL)  /* the coarse grained timeout in milliseconds */
#endif /* TCP_SLOW_INTERVAL */

#define TCP_FIN_WAIT_TIMEOUT 20000 /* milliseconds */
#define TCP_SYN_RCVD_TIMEOUT 20000 /* milliseconds */

#define TCP_OOSEQ_TIMEOUT        6U /* x RTO */

#ifndef TCP_MSL
#define TCP_MSL 60000UL /* The maximum segment lifetime in milliseconds */
#endif

/* Keepalive values, compliant with RFC 1122. Don't change this unless you know what you're doing */
#ifndef  TCP_KEEPIDLE_DEFAULT
#define  TCP_KEEPIDLE_DEFAULT     7200000UL /* Default KEEPALIVE timer in milliseconds */
#endif

#ifndef  TCP_KEEPINTVL_DEFAULT
#define  TCP_KEEPINTVL_DEFAULT    75000UL   /* Default Time between KEEPALIVE probes in milliseconds */
#endif

#ifndef  TCP_KEEPCNT_DEFAULT
#define  TCP_KEEPCNT_DEFAULT      9U        /* Default Counter for KEEPALIVE probes */
#endif

#define  TCP_MAXIDLE              TCP_KEEPCNT_DEFAULT * TCP_KEEPINTVL_DEFAULT  /* Maximum KEEPALIVE probe time */

/* Fields are (of course) in network byte order.
 * Some fields are converted to host byte order in tcp_input().
 */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct tcp_hdr {
  PACK_STRUCT_FIELD(u16_t src);
  PACK_STRUCT_FIELD(u16_t dest);
  PACK_STRUCT_FIELD(u32_t seqno);
  PACK_STRUCT_FIELD(u32_t ackno);
  PACK_STRUCT_FIELD(u16_t _hdrlen_rsvd_flags);
  PACK_STRUCT_FIELD(u16_t wnd);
  PACK_STRUCT_FIELD(u16_t chksum);
  PACK_STRUCT_FIELD(u16_t urgp);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "epstruct.h"
#endif

#define TCP_MAX_OPTION_BYTES 40

#define TCPH_HDRLEN(phdr) (ntohs((phdr)->_hdrlen_rsvd_flags) >> 12)
#define TCPH_HDRLEN_BYTES(phdr) ((u8_t)(TCPH_HDRLEN(phdr) << 2))
#define TCPH_FLAGS(phdr)  (ntohs((phdr)->_hdrlen_rsvd_flags) & TCP_FLAGS)

#define TCPH_HDRLEN_SET(phdr, len) (phdr)->_hdrlen_rsvd_flags = htons(((len) << 12) | TCPH_FLAGS(phdr))
#define TCPH_FLAGS_SET(phdr, flags) (phdr)->_hdrlen_rsvd_flags = (((phdr)->_hdrlen_rsvd_flags & PP_HTONS((u16_t)(~(u16_t)(TCP_FLAGS)))) | htons(flags))
#define TCPH_HDRLEN_FLAGS_SET(phdr, len, flags) (phdr)->_hdrlen_rsvd_flags = htons(((len) << 12) | (flags))

#define TCPH_SET_FLAG(phdr, flags ) (phdr)->_hdrlen_rsvd_flags = ((phdr)->_hdrlen_rsvd_flags | htons(flags))
#define TCPH_UNSET_FLAG(phdr, flags) (phdr)->_hdrlen_rsvd_flags = htons(ntohs((phdr)->_hdrlen_rsvd_flags) | (TCPH_FLAGS(phdr) & ~(flags)) )

#define TCP_TCPLEN(seg) ((seg)->len + ((TCPH_FLAGS((seg)->tcphdr) & (TCP_FIN | TCP_SYN)) != 0))

/** Flags used on input processing, not on pcb->flags
*/
#define TF_RESET     (u8_t)0x08U   /* Connection was reset. */
#define TF_CLOSED    (u8_t)0x10U   /* Connection was sucessfully closed. */
#define TF_GOT_FIN   (u8_t)0x20U   /* Connection was closed by the remote end. */


#if LWIP_EVENT_API

#define TCP_EVENT_ACCEPT(pcb,err,ret)    ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                LWIP_EVENT_ACCEPT, NULL, 0, err)
#define TCP_EVENT_SENT(pcb,space,ret) ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                   LWIP_EVENT_SENT, NULL, space, ERR_OK)
#define TCP_EVENT_RECV(pcb,p,err,ret) ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                LWIP_EVENT_RECV, (p), 0, (err))
#define TCP_EVENT_CLOSED(pcb,ret) ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                LWIP_EVENT_RECV, NULL, 0, ERR_OK)
#define TCP_EVENT_CONNECTED(pcb,err,ret) ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                LWIP_EVENT_CONNECTED, NULL, 0, (err))
#define TCP_EVENT_POLL(pcb,ret)       ret = lwip_tcp_event((pcb)->callback_arg, (pcb),\
                LWIP_EVENT_POLL, NULL, 0, ERR_OK)
#define TCP_EVENT_ERR(errf,arg,err)  lwip_tcp_event((arg), NULL, \
                LWIP_EVENT_ERR, NULL, 0, (err))

#else /* LWIP_EVENT_API */

#define TCP_EVENT_ACCEPT(lpcb,pcb,err,ret,cat3)                          \
  do {                                                         \
    if((lpcb)->accept != NULL) {                                  \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_ACCEPT, %s, line=%d, lpcb=%lx, pcb=%lx, err=%d\n", __FUNCTION__, __LINE__, lpcb, pcb, err); \
      (ret) = (lpcb)->accept((lpcb)->callback_arg,(pcb),(err));  \
    } else {                                                    \
      (ret) = ERR_ARG;                                          \
    }                                                           \
  } while (0)

#define TCP_EVENT_SENT(pcb,space,ret,cat3)                          \
  do {                                                         \
    if((pcb)->sent != NULL) {                                    \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_SENT, %s, line=%d, pcb=%lx, space=%d\n", __FUNCTION__, __LINE__, pcb, space); \
      (ret) = (pcb)->sent((pcb)->callback_arg,(pcb),(space));  \
    } else {                                                  \
      (ret) = ERR_OK;                                         \
    }                                                         \
  } while (0)

#define TCP_EVENT_RECV(pcb,p,err,ret,cat3)                          \
  do {                                                         \
    if((pcb)->recv != NULL) {                                  \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_RECV, %s, line=%d, pcb=%lx, pbuf=%lx, err=%d\n", __FUNCTION__, __LINE__, pcb, p, err); \
      (ret) = (pcb)->recv((pcb)->callback_arg,(pcb),(p),(err));\
    } else {                                                   \
      (ret) = tcp_recv_null(NULL, (pcb), (p), (err));          \
    }                                                          \
  } while (0)

#define TCP_EVENT_CLOSED(pcb,ret,cat3)                                \
  do {                                                           \
    if(((pcb)->recv != NULL)) {                                  \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_CLOSED, %s, line=%d, pcb=%lx\n", __FUNCTION__, __LINE__, pcb); \
      (ret) = (pcb)->recv((pcb)->callback_arg,(pcb),NULL,ERR_OK);\
    } else {                                                     \
      (ret) = ERR_OK;                                            \
    }                                                            \
  } while (0)

#define TCP_EVENT_CONNECTED(pcb,err,ret,cat3)                         \
  do {                                                           \
    if((pcb)->connected != NULL) {                                \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_CONNECTED, %s, line=%d, pcb=%lx, err=%d\n", __FUNCTION__, __LINE__, pcb, err); \
      (ret) = (pcb)->connected((pcb)->callback_arg,(pcb),(err)); \
    } else {                                                    \
      (ret) = ERR_OK;                                           \
    }                                                           \
  } while (0)

#define TCP_EVENT_POLL(pcb,ret,cat3)                                 \
  do {                                                          \
    if((pcb)->poll != NULL) {                                   \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_POLL, %s, line=%d, pcb=%lx\n", __FUNCTION__, __LINE__, pcb); \
      (ret) = (pcb)->poll((pcb)->callback_arg,(pcb));           \
    } else {                                                    \
      (ret) = ERR_OK;                                           \
    }                                                           \
  } while (0)

#define TCP_EVENT_ERR(errf,arg,err,cat3)                            \
  do {                                                         \
    tcp_err_store = err;                                       \
    if((errf) != NULL) {                                        \
      LWIP_DIAG(TCP_DEBUG, cat3, "TCP_EVENT_ERR, %s, line=%d, arg=%lx, err=%d\n", __FUNCTION__, __LINE__, (void*)arg, err); \
      (errf)((arg),(err));                                      \
    } else {                                                   \
      LWIP_DEBUGF(TCP_DEBUG, ("TCP_EVENT_ERR, errf is null, %s, line=%d, arg=%lx, err=%d\n", __FUNCTION__, __LINE__, (void*)arg, err)); \
    }                                                           \
  } while (0)

#endif /* LWIP_EVENT_API */

/** Enabled extra-check for TCP_OVERSIZE if LWIP_DEBUG is enabled */
#if TCP_OVERSIZE && defined(LWIP_DEBUG)
#define TCP_OVERSIZE_DBGCHECK 1
#else
#define TCP_OVERSIZE_DBGCHECK 0
#endif

/** Don't generate checksum on copy if CHECKSUM_GEN_TCP is disabled */
#define TCP_CHECKSUM_ON_COPY  (LWIP_CHECKSUM_ON_COPY && CHECKSUM_GEN_TCP)

/* This structure represents a TCP segment on the unsent, unacked and ooseq queues */
struct tcp_seg {
  struct tcp_seg *next;    /* used when putting segements on a queue */
  struct pbuf *p;          /* buffer containing data + TCP header */
  u16_t len;               /* the TCP length of this segment */
#if TCP_OVERSIZE_DBGCHECK
  u16_t oversize_left;     /* Extra bytes available at the end of the last
                              pbuf in unsent (used for asserting vs.
                              tcp_pcb.unsent_oversized only) */
#endif /* TCP_OVERSIZE_DBGCHECK */
#if TCP_CHECKSUM_ON_COPY
  u16_t chksum;
  u8_t  chksum_swapped;
#endif /* TCP_CHECKSUM_ON_COPY */
  u8_t  flags;
#define TF_SEG_OPTS_MSS         (u8_t)0x01U /* Include MSS option. */
#define TF_SEG_OPTS_TS          (u8_t)0x02U /* Include timestamp option. */
#define TF_SEG_DATA_CHECKSUMMED (u8_t)0x04U /* ALL data (not the header) is checksummed into 'chksum' */
#define TF_SEG_OPTS_WND_SCALE   (u8_t)0x08U /* Include WND SCALE option (only used in SYN segments)*/
#define TF_SEG_OPTS_SACK_PERM   (u8_t)0x10U /* Include WND SACK Permitted option (only used in SYN segments)*/

  struct tcp_hdr *tcphdr;  /* the TCP header */
};

#define LWIP_TCP_OPT_EOL                0
#define LWIP_TCP_OPT_NOP                1
#define LWIP_TCP_OPT_MSS                2
#define LWIP_TCP_OPT_WS                 3
#define LWIP_TCP_OPT_SACK_PERM          4
#define LWIP_TCP_OPT_TS                 8

#define LWIP_TCP_OPT_LEN_MSS            4

#if LWIP_TCP_TIMESTAMPS
#define LWIP_TCP_OPT_LEN_TS             10
#define LWIP_TCP_OPT_LEN_TS_OUT         12 /*aligned for output(includes NOP padding)*/
#else
#define LWIP_TCP_OPT_LEN_TS_OUT         0
#endif

#if LWIP_WND_SCALE
#define LWIP_TCP_OPT_LEN_WS             3
#define LWIP_TCP_OPT_LEN_WS_OUT         4   /*aligned for output(includes NOP padding)*/
#else
#define LWIP_TCP_OPT_LEN_WS_OUT         0
#endif

#if LWIP_TCP_SACK_OUT
#define LWIP_TCP_OPT_LEN_SACK_PERM      2
#define LWIP_TCP_OPT_LEN_SACK_PERM_OUT  4   /*aligned for output(includes NOP padding)*/
#else
#define LWIP_TCP_OPT_LEN_SACK_PERM_OUT  0
#endif

#define LWIP_TCP_OPT_LENGTH(flags)              \
  ((flags) & TF_SEG_OPTS_MSS        ? LWIP_TCP_OPT_LEN_MSS           : 0) + \
  ((flags) & TF_SEG_OPTS_TS         ? LWIP_TCP_OPT_LEN_TS_OUT        : 0) + \
  ((flags) & TF_SEG_OPTS_WND_SCALE  ? LWIP_TCP_OPT_LEN_WS_OUT        : 0) + \
  ((flags) & TF_SEG_OPTS_SACK_PERM  ? LWIP_TCP_OPT_LEN_SACK_PERM_OUT : 0)

/** This returns a TCP header option for MSS in an u32_t */
#define TCP_BUILD_MSS_OPTION(mss) htonl(0x02040000 | ((mss) & 0xFFFF))

#if LWIP_WND_SCALE
#define TCPWNDSIZE_F       U32_F
#define TCPWND_MAX         0xFFFFFFFFU
#define TCPWND_CHECK16(x)  LWIP_ASSERT_NOW((x) <= 0xFFFF)
#define TCPWND_MIN16(x)    ((u16_t)LWIP_MIN((x), 0xFFFF))
#else /* LWIP_WND_SCALE */
#define TCPWNDSIZE_F       U16_F
#define TCPWND_MAX         0xFFFFU
#define TCPWND_CHECK16(x)
#define TCPWND_MIN16(x)    x
#endif /* LWIP_WND_SCALE */

/* Global variables: */
extern struct tcp_pcb *tcp_input_pcb;
extern u32_t tcp_ticks;
extern u8_t tcp_active_pcbs_changed;

/* The TCP PCB lists. */
union tcp_listen_pcbs_t { /* List of all TCP PCBs in LISTEN state. */
  struct tcp_pcb_listen *listen_pcbs;
  struct tcp_pcb *pcbs;
};
extern struct tcp_pcb *tcp_bound_pcbs;
extern union tcp_listen_pcbs_t tcp_listen_pcbs;
extern struct tcp_pcb *tcp_active_pcbs;  /* List of all TCP PCBs that are in a
                                          state in which they accept or send data. */
extern struct tcp_pcb *tcp_tw_pcbs;      /* List of all TCP PCBs in TIME-WAIT. */
extern struct tcp_pcb *tcp_tmp_pcb;      /* Only used for temporary storage. */

/* Axioms about the above lists:
   1) Every TCP PCB that is not CLOSED is in one of the lists.
   2) A PCB is only in one of the lists.
   3) All PCBs in the tcp_listen_pcbs list is in LISTEN state.
   4) All PCBs in the tcp_tw_pcbs list is in TIME-WAIT state.
*/
/* Define two macros, TCP_REG and TCP_RMV that registers a TCP PCB
   with a PCB list or removes a PCB from a list, respectively. */
#define TCP_DEBUG_PCB_LISTS 1
#ifndef TCP_DEBUG_PCB_LISTS
#define TCP_DEBUG_PCB_LISTS 0
#endif
#if TCP_DEBUG_PCB_LISTS
#define TCP_REG(pcbs, npcb) do {\
                            LWIP_DIAG(TCP_DEBUG | LWIP_DBG_LEVEL_WARNING, lwip_tcpimpl_410, "TCP_REG %lx to pcbs %lx, called by %s-%d\n", \
                                                                                (npcb), *(pcbs), __FUNCTION__, __LINE__); \
                            for(tcp_tmp_pcb = *(pcbs); \
                              tcp_tmp_pcb != NULL; \
                              tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                              LWIP_ASSERT_NOW(tcp_tmp_pcb != (npcb)); \
                            } \
                            LWIP_ASSERT_NOW(((pcbs) == &tcp_bound_pcbs) || ((npcb)->state != CLOSED)); \
                            (npcb)->next = *(pcbs); \
                            LWIP_ASSERT_NOW((npcb)->next != (npcb)); \
                            *(pcbs) = (npcb); \
                            LWIP_ASSERT_NOW(tcp_pcbs_sane()); \
                            tcp_timer_needed(); \
                            } while(0)
#define TCP_RMV(pcbs, npcb) do { \
                            LWIP_ASSERT_NOW(*(pcbs) != NULL); \
                            LWIP_DIAG(TCP_DEBUG | LWIP_DBG_LEVEL_WARNING, lwip_tcpimpl_426, "TCP_RMV: removing %lx from %lx, called by %s-%d\n", \
                                                                                        (npcb), *(pcbs), __FUNCTION__, __LINE__); \
                            if(*(pcbs) == (npcb)) { \
                               *(pcbs) = (*pcbs)->next; \
                            } else for(tcp_tmp_pcb = *(pcbs); tcp_tmp_pcb != NULL; tcp_tmp_pcb = tcp_tmp_pcb->next) { \
                               if(tcp_tmp_pcb->next == (npcb)) { \
                                  tcp_tmp_pcb->next = (npcb)->next; \
                                  break; \
                               } \
                            } \
                            (npcb)->next = NULL; \
                            LWIP_ASSERT_NOW(tcp_pcbs_sane()); \
                            } while(0)

#else /* LWIP_DEBUG */

#define TCP_REG(pcbs, npcb)                        \
  do {                                             \
    (npcb)->next = *pcbs;                          \
    *(pcbs) = (npcb);                              \
    tcp_timer_needed();                            \
  } while (0)

#define TCP_RMV(pcbs, npcb)                        \
  do {                                             \
    if(*(pcbs) == (npcb)) {                        \
      (*(pcbs)) = (*pcbs)->next;                   \
    }                                              \
    else {                                         \
      for(tcp_tmp_pcb = *pcbs;                     \
          tcp_tmp_pcb != NULL;                     \
          tcp_tmp_pcb = tcp_tmp_pcb->next) {       \
        if(tcp_tmp_pcb->next == (npcb)) {          \
          tcp_tmp_pcb->next = (npcb)->next;        \
          break;                                   \
        }                                          \
      }                                            \
    }                                              \
    (npcb)->next = NULL;                           \
  } while(0)

#endif /* LWIP_DEBUG */

#define TCP_REG_ACTIVE(npcb)                       \
  do {                                             \
    TCP_REG(&tcp_active_pcbs, npcb);               \
    tcp_active_pcbs_changed = 1;                   \
  } while (0)

#define TCP_RMV_ACTIVE(npcb)                       \
  do {                                             \
    TCP_RMV(&tcp_active_pcbs, npcb);               \
    tcp_active_pcbs_changed = 1;                   \
  } while (0)

#define TCP_PCB_REMOVE_ACTIVE(pcb)                 \
  do {                                             \
    tcp_pcb_remove(&tcp_active_pcbs, pcb);         \
    tcp_active_pcbs_changed = 1;                   \
  } while (0)


/* Internal functions: */
struct tcp_pcb *tcp_pcb_copy(struct tcp_pcb *pcb);
void tcp_pcb_purge(struct tcp_pcb *pcb);
void tcp_pcb_remove(struct tcp_pcb **pcblist, struct tcp_pcb *pcb);
void tcp_pcb_free(struct tcp_pcb *pcb);
void tcp_segs_free(struct tcp_seg *seg);
void tcp_seg_free(struct tcp_seg *seg);
struct tcp_seg *tcp_seg_copy(struct tcp_seg *seg);
#if TCP_QUEUE_OOSEQ
void tcp_free_ooseq(struct tcp_pcb *pcb);
#endif

#define tcp_ack(pcb)                               \
  do {                                             \
    if((pcb)->flags & TF_ACK_DELAY) {              \
      (pcb)->flags &= ~TF_ACK_DELAY;               \
      (pcb)->flags |= TF_ACK_NOW;                  \
    }                                              \
    else {                                         \
      (pcb)->flags |= TF_ACK_DELAY;                \
    }                                              \
  } while (0)

#define tcp_ack_now(pcb)                           \
  do {                                             \
    (pcb)->flags |= TF_ACK_NOW;                    \
  } while (0)

err_t tcp_send_fin(struct tcp_pcb *pcb);
err_t tcp_enqueue_flags(struct tcp_pcb *pcb, u8_t flags);

void tcp_rexmit_seg(struct tcp_pcb *pcb, struct tcp_seg *seg);

void tcp_rst_impl(struct tcp_pcb *pcb, u32_t seqno, u32_t ackno,
       ipX_addr_t *local_ip, ipX_addr_t *remote_ip,
       u16_t local_port, u16_t remote_port, 
       char* func, int line);
#define tcp_rst(pcb, seqno, ackno, local_ip, remote_ip, local_port, remote_port) \
  tcp_rst_impl(pcb, seqno, ackno, local_ip, remote_ip, local_port, remote_port, (char *)__FUNCTION__, __LINE__)

u32_t tcp_next_iss(void);

void tcp_keepalive(struct tcp_pcb *pcb);
void tcp_zero_window_probe(struct tcp_pcb *pcb);
void tcp_trigger_input_pcb_close(void);

#if TCP_CALCULATE_EFF_SEND_MSS
u16_t tcp_eff_send_mss_impl(u16_t sendmss, struct tcp_pcb *pcb, struct netif *netif);
#define tcp_eff_send_mss(sendmss, pcb, netif) tcp_eff_send_mss_impl(sendmss, pcb, netif)
#endif /* TCP_CALCULATE_EFF_SEND_MSS */


#if LWIP_CALLBACK_API
err_t tcp_recv_null(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
#endif /* LWIP_CALLBACK_API */

#if LWIP_DEBUG
void tcp_debug_print(struct tcp_hdr *tcphdr);
void tcp_debug_print_flags(u8_t flags);
void tcp_debug_print_state_fn(struct tcp_pcb *pcb, const char* file, const int line);
#define tcp_debug_print_state(pcb)   tcp_debug_print_state_fn((pcb), __FUNCTION__, __LINE__)
void tcp_debug_print_pcbs(void);
s16_t tcp_pcbs_sane(void);
void tcp_listen_pcbs_dump(void);
#else
#define tcp_debug_print(tcphdr)
#define tcp_debug_print_flags(flags)
#define tcp_debug_print_state(s)
#define tcp_debug_print_pcbs()
#define tcp_pcbs_sane() 1
#define tcp_listen_pcbs_dump()
#endif /* TCP_DEBUG */

/** External function (implemented in timers.c), called when TCP detects
 * that a timer is needed (i.e. active- or time-wait-pcb found). */
void tcp_timer_needed(void);
#if TCP_MSS_ADJUST_EN
void tcp_mangle_mss(struct netif *netif, struct pbuf *p);
#endif
#if TCP_DNS_TURN_EN
void tcp_mangle_dns_turn(struct pbuf* p, u32_t direction);
#endif
#ifdef __cplusplus
}
#endif

#endif /* LWIP_TCP */

#endif /* __LWIP_TCP_H__ */
