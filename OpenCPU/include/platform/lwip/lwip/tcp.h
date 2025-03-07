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
#ifndef __LWIP_TCP_H__
#define __LWIP_TCP_H__

#include "opt.h"

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */

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

struct tcp_pcb;

/** Function prototype for tcp accept callback functions. Called when a new
 * connection can be accepted on a listening pcb.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param newpcb The new connection pcb
 * @param err An error code if there has been an error accepting.
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_accept_fn)(void *arg, struct tcp_pcb *newpcb, err_t err);

/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_recv_fn)(void *arg, struct tcp_pcb *tpcb,
                             struct pbuf *p, err_t err);

/** Function prototype for tcp sent callback functions. Called when sent data has
 * been acknowledged by the remote side. Use it to free corresponding resources.
 * This also means that the pcb has now space available to send new data.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb for which data has been acknowledged
 * @param len The amount of bytes acknowledged
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_sent_fn)(void *arg, struct tcp_pcb *tpcb,
                              u16_t len);

/** Function prototype for tcp poll callback functions. Called periodically as
 * specified by @see tcp_poll.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb tcp pcb
 * @return ERR_OK: try to send some data by calling tcp_output
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
typedef err_t (*tcp_poll_fn)(void *arg, struct tcp_pcb *tpcb);

/** Function prototype for tcp error callback functions. Called when the pcb
 * receives a RST or is unexpectedly closed for any other reason.
 *
 * @note The corresponding pcb is already freed when this callback is called!
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param err Error code to indicate why the pcb has been closed
 *            ERR_ABRT: aborted through tcp_abort or by a TCP timer
 *            ERR_RST: the connection was reset by the remote host
 */
typedef void  (*tcp_err_fn)(void *arg, err_t err);

/** Function prototype for tcp connected callback functions. Called when a pcb
 * is connected to the remote side after initiating a connection attempt by
 * calling tcp_connect().
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which is connected
 * @param err An unused error code, always ERR_OK currently ;-) TODO!
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 *
 * @note When a connection attempt fails, the error callback is currently called!
 */
typedef err_t (*tcp_connected_fn)(void *arg, struct tcp_pcb *tpcb, err_t err);

enum tcp_state {
  CLOSED      = 0,
  LISTEN      = 1,
  SYN_SENT    = 2,
  SYN_RCVD    = 3,
  ESTABLISHED = 4,
  FIN_WAIT_1  = 5,
  FIN_WAIT_2  = 6,
  CLOSE_WAIT  = 7,
  CLOSING     = 8,
  LAST_ACK    = 9,
  TIME_WAIT   = 10
};

#if LWIP_WND_SCALE
typedef u32_t tcpwnd_size_t;
#define RCV_WND_SCALE(pcb, wnd) (((wnd) >> (pcb)->rcv_scale))
#define SND_WND_SCALE(pcb, wnd) (((wnd) << (pcb)->snd_scale))
#define TCPWND16(x)             ((u16_t)LWIP_MIN((x), 0xFFFF))
#define TCP_WND_MAX(pcb)        ((tcpwnd_size_t)(((pcb)->flags & TF_WND_SCALE) ? TCP_WND : TCPWND16(TCP_WND)))
#else
typedef u16_t tcpwnd_size_t;
#define RCV_WND_SCALE(pcb, wnd) (wnd)
#define SND_WND_SCALE(pcb, wnd) (wnd)
#define TCPWND16(x)             (x)
#define TCP_WND_MAX(pcb)        TCP_WND
#endif
/* Increments a tcpwnd_size_t and holds at max value rather than rollover */
#define TCP_WND_INC(wnd, inc)   do { \
                                  if ((tcpwnd_size_t)(wnd + inc) >= wnd) { \
                                    wnd = (tcpwnd_size_t)(wnd + inc); \
                                  } else { \
                                    wnd = (tcpwnd_size_t)-1; \
                                  } \
                                } while(0)

#if LWIP_CALLBACK_API
  /* Function to call when a listener has been connected.
   * @param arg user-supplied argument (tcp_pcb.callback_arg)
   * @param pcb a new tcp_pcb that now is connected
   * @param err an error argument (TODO: that is current always ERR_OK?)
   * @return ERR_OK: accept the new connection,
   *                 any other err_t abortsthe new connection
   */
#define DEF_ACCEPT_CALLBACK  tcp_accept_fn accept;
#else /* LWIP_CALLBACK_API */
#define DEF_ACCEPT_CALLBACK
#endif /* LWIP_CALLBACK_API */

#if LWIP_TCP_SACK_OUT
/** SACK ranges to include in ACK packets.
 * SACK entry is invalid if left==right. */
struct tcp_sack_range {
  /** Left edge of the SACK: the first acknowledged sequence number. */
  u32_t left;
  /** Right edge of the SACK: the last acknowledged sequence number +1 (so first NOT acknowledged). */
  u32_t right;
};
#endif /* LWIP_TCP_SACK_OUT */

typedef u16_t tcpflags_t;
#define TCP_ALLFLAGS 0xffffU

/**
 * members common to struct tcp_pcb and struct tcp_listen_pcb
 */
#define TCP_PCB_COMMON(type) \
  type *next; /* for the linked list */ \
  void *callback_arg; \
  enum tcp_state state_prev; /* TCP state */ \
  enum tcp_state state; /* TCP state */ \
  u8_t prio; \
  /* ports are in host byte order */ \
  u16_t local_port


/* the TCP protocol control block */
struct tcp_pcb {
/** common PCB members */
  IP_PCB;
/** protocol specific PCB members */
  TCP_PCB_COMMON(struct tcp_pcb);

  /* ports are in host byte order */
  u16_t remote_port;

  tcpflags_t flags;
#define TF_ACK_DELAY   0x01U   /* Delayed ACK. */
#define TF_ACK_NOW     0x02U   /* Immediate ACK. */
#define TF_INFR        0x04U   /* In fast recovery. */
#define TF_CLOSEPEND   0x08U   /* If this is set, tcp_close failed to enqueue the FIN (retried in tcp_tmr) */
#define TF_RXCLOSED    0x10U   /* rx closed by tcp_shutdown */
#define TF_FIN         0x20U   /* Connection was closed locally (FIN segment enqueued). */
#define TF_NODELAY     0x40U   /* Disable Nagle algorithm */
#define TF_NAGLEMEMERR 0x80U   /* nagle enabled, memerr, try to output to prevent delayed ACK to happen */
#if LWIP_WND_SCALE
#define TF_WND_SCALE   0x0100U /* Window Scale option enabled */
#endif
#if TCP_LISTEN_BACKLOG
#define TF_BACKLOGPEND 0x0200U /* If this is set, a connection pcb has increased the backlog on its listener */
#endif
#if LWIP_TCP_TIMESTAMPS
#define TF_TIMESTAMP   0x0400U   /* Timestamp option enabled */
#endif
#define TF_RTO         0x0800U /* RTO timer has fired, in-flight data moved to unsent and being retransmitted */
#if LWIP_TCP_SACK_OUT
#define TF_SACK        0x1000U /* Selective ACKs enabled */
#endif

  /* the rest of the fields are in host byte order
     as we have to do some math with them */

  /* Timers */
  u8_t polltmr, pollinterval;
  u8_t last_timer;
  u32_t tmr;

  /* receiver variables */
  u32_t rcv_nxt;   /* next seqno expected */
  tcpwnd_size_t rcv_wnd;   /* receiver window available */
  tcpwnd_size_t rcv_ann_wnd; /* receiver window to announce */
  u32_t rcv_ann_right_edge; /* announced right edge of window */

#if LWIP_TCP_SACK_OUT
   /* SACK ranges to include in ACK packets (entry is invalid if left==right) */
   struct tcp_sack_range rcv_sacks[LWIP_TCP_MAX_SACK_NUM];
#define LWIP_TCP_SACK_VALID(pcb, idx) ((pcb)->rcv_sacks[idx].left != (pcb)->rcv_sacks[idx].right)
#endif /* LWIP_TCP_SACK_OUT */

  /* Retransmission timer. */
  s16_t rtime;

  u16_t mss;   /* maximum segment size */

  /* RTT (round trip time) estimation variables */
  u32_t rttest; /* RTT estimate in 500ms ticks */
  u32_t rtseq;  /* sequence number being timed */
  s16_t sa, sv; /* @todo document this */

  s16_t rto;    /* retransmission time-out */
  u8_t nrtx;    /* number of retransmissions */

  /* fast retransmit/recovery */
  u8_t dupacks;
  u32_t lastack; /* Highest acknowledged seqno. */
  u32_t acked_sum; /* accumulation of acked number */
  u32_t close_timeout; /*timerout set for tcp close */

  /* congestion avoidance/control variables */
  tcpwnd_size_t cwnd;
  tcpwnd_size_t ssthresh;

  /* first byte following last rto byte */
  u32_t rto_end;

  /* sender variables */
  u32_t snd_nxt;   /* next new seqno to be sent */
  u32_t snd_wl1, snd_wl2; /* Sequence and acknowledgement numbers of last
                             window update. */
  u32_t snd_lbb;       /* Sequence number of next byte to be buffered. */
  tcpwnd_size_t snd_wnd;   /* sender window */
  tcpwnd_size_t snd_wnd_max; /* the maximum sender window announced by the remote host */
  tcpwnd_size_t snd_buf;   /* Available buffer space for sending (in bytes). */
  tcpwnd_size_t bytes_acked;

#define TCP_SNDQUEUELEN_OVERFLOW (0xffffU-3)
  u16_t snd_queuelen; /* Available buffer space for sending (in tcp_segs). */

#if TCP_OVERSIZE
  /* Extra bytes available at the end of the last pbuf in unsent. */
  u16_t unsent_oversize;
#endif /* TCP_OVERSIZE */

  /* These are ordered by sequence number: */
  struct tcp_seg *unsent;   /* Unsent (queued) segments. */
  struct tcp_seg *unacked;  /* Sent but unacknowledged segments. */
#if TCP_QUEUE_OOSEQ
  struct tcp_seg *ooseq;    /* Received out of sequence segments. */
#endif /* TCP_QUEUE_OOSEQ */

  struct pbuf *refused_data; /* Data previously received but not yet taken by upper layer */

#if LWIP_CALLBACK_API || TCP_LISTEN_BACKLOG
  struct tcp_pcb_listen* listener;
#endif

#if LWIP_CALLBACK_API
  /* Function to be called when more send buffer space is available. */
  tcp_sent_fn sent;
  /* Function to be called when (in-sequence) data has arrived. */
  tcp_recv_fn recv;
  /* Function to be called when a connection has been set up. */
  tcp_connected_fn connected;
  /* Function which is called periodically. */
  tcp_poll_fn poll;
  /* Function to be called whenever a fatal error occurs. */
  tcp_err_fn errf;
#endif /* LWIP_CALLBACK_API */

#if LWIP_TCP_TIMESTAMPS
  u32_t ts_lastacksent;
  u32_t ts_recent;
#endif /* LWIP_TCP_TIMESTAMPS */

  /* idle time before KEEPALIVE is sent */
  u32_t keep_idle;
#if LWIP_TCP_KEEPALIVE
  u32_t keep_intvl;
  u32_t keep_cnt;
#endif /* LWIP_TCP_KEEPALIVE */

  /* Persist timer counter */
  u8_t persist_cnt;
  /* Persist timer back-off */
  u8_t persist_backoff;
  /* Number of persist probes */
  u8_t persist_probe;
  /* KEEPALIVE counter */
  u8_t keep_cnt_sent;

#if LWIP_WND_SCALE
  u8_t snd_scale;
  u8_t rcv_scale;
#endif

  /* Timer out counter */
  u32_t timer_out_cnt;
  u16_t redirect_port;
  u16_t rst_loop_cnt;

#if (LWIP_TCP_WRITE_STAGE_OPT)
  char* stage_buf;    /*stage buf, store write data for combine to a big one*/
  u16_t stage_index;  /*record stage index for stage buf*/
  u8_t stage_ttl;    /*ttl set for clean staging buf, if no write more*/
  u8_t stage_rsvd;
#endif

  u8_t nrtx_syn_max; /*max nrtx setting for tcp sync*/
  u8_t nrtx_max;     /*max nrtx setting for tcp*/
  u16_t nrtx_rsvd;
};

struct tcp_pcb_listen {
/* Common members of all PCB types */
  IP_PCB;
/* Protocol specific PCB members */
  TCP_PCB_COMMON(struct tcp_pcb_listen);

#if LWIP_CALLBACK_API
  tcp_accept_fn accept;
#endif

#if TCP_LISTEN_BACKLOG
  u8_t backlog;
  u8_t accepts_pending;
#endif /* TCP_LISTEN_BACKLOG */
#if LWIP_IPV6
  u8_t accept_any_ip_version;
#endif /* LWIP_IPV6 */
};

#if LWIP_EVENT_API

enum lwip_event {
  LWIP_EVENT_ACCEPT,
  LWIP_EVENT_SENT,
  LWIP_EVENT_RECV,
  LWIP_EVENT_CONNECTED,
  LWIP_EVENT_POLL,
  LWIP_EVENT_ERR
};

err_t lwip_tcp_event(void *arg, struct tcp_pcb *pcb,
         enum lwip_event,
         struct pbuf *p,
         u16_t size,
         err_t err);

#endif /* LWIP_EVENT_API */

/* Application program's interface: */
struct tcp_pcb * tcp_new     (void);

void             tcp_arg     (struct tcp_pcb *pcb, void *arg);
void             tcp_accept  (struct tcp_pcb *pcb, tcp_accept_fn accept);
void             tcp_recv    (struct tcp_pcb *pcb, tcp_recv_fn recv);
void             tcp_sent    (struct tcp_pcb *pcb, tcp_sent_fn sent);
void             tcp_poll    (struct tcp_pcb *pcb, tcp_poll_fn poll, u8_t interval);
void             tcp_err     (struct tcp_pcb *pcb, tcp_err_fn err);

#define          tcp_set_flags(pcb, set_flags)     do { (pcb)->flags = (tcpflags_t)((pcb)->flags |  (set_flags)); } while(0)
#define          tcp_clear_flags(pcb, clr_flags)   do { (pcb)->flags = (tcpflags_t)((pcb)->flags & (tcpflags_t)(~(clr_flags) & TCP_ALLFLAGS)); } while(0)
#define          tcp_is_flag_set(pcb, flag)        (((pcb)->flags & (flag)) != 0)

#if LWIP_TCP_TIMESTAMPS
#define          tcp_mss(pcb)             (((pcb)->flags & TF_TIMESTAMP) ? ((pcb)->mss - 12)  : (pcb)->mss)
#else /* LWIP_TCP_TIMESTAMPS */
/** @ingroup tcp_raw */
#define          tcp_mss(pcb)             ((pcb)->mss)
#endif /* LWIP_TCP_TIMESTAMPS */
/** @ingroup tcp_raw */
#define          tcp_sndbuf(pcb)          (TCPWND16((pcb)->snd_buf))
/** @ingroup tcp_raw */
#define          tcp_sndqueuelen(pcb)     ((pcb)->snd_queuelen)
/** @ingroup tcp_raw */
#define          tcp_nagle_disable(pcb)   tcp_set_flags(pcb, TF_NODELAY)
/** @ingroup tcp_raw */
#define          tcp_nagle_enable(pcb)    tcp_clear_flags(pcb, TF_NODELAY)
/** @ingroup tcp_raw */
#define          tcp_nagle_disabled(pcb)  tcp_is_flag_set(pcb, TF_NODELAY)

#if TCP_LISTEN_BACKLOG
#define          tcp_backlog_set(pcb, new_backlog) do { \
  LWIP_ASSERT_NOW((pcb)->state == LISTEN); \
  ((struct tcp_pcb_listen *)(pcb))->backlog = ((new_backlog) ? (new_backlog) : 1); } while(0)
void             tcp_backlog_delayed(struct tcp_pcb* pcb);
void             tcp_backlog_accepted(struct tcp_pcb* pcb);
#else  /* TCP_LISTEN_BACKLOG */
#define          tcp_backlog_set(pcb, new_backlog)
#define          tcp_backlog_delayed(pcb)
#define          tcp_backlog_accepted(pcb)
#endif /* TCP_LISTEN_BACKLOG */
#define          tcp_accepted(pcb) do { LWIP_UNUSED_ARG(pcb); } while(0) /* compatibility define, not needed any more */

void             tcp_recved  (struct tcp_pcb *pcb, u16_t len);
err_t            tcp_bind    (struct tcp_pcb *pcb, ip_addr_t *ipaddr,
                              u16_t port);
err_t            tcp_connect (struct tcp_pcb *pcb, ip_addr_t *ipaddr,
                              u16_t port, tcp_connected_fn connected);

struct tcp_pcb * tcp_listen_with_backlog(struct tcp_pcb *pcb, u8_t backlog);
#define          tcp_listen(pcb) tcp_listen_with_backlog(pcb, TCP_DEFAULT_LISTEN_BACKLOG)

void             tcp_abort_fn (struct tcp_pcb *pcb, const char *func, const int line);
#define          tcp_abort(pcb)  tcp_abort_fn(pcb, __FUNCTION__, __LINE__)

err_t            tcp_close_fn(struct tcp_pcb *pcb, const char *func, const int line);
#define          tcp_close(pcb)  tcp_close_fn(pcb, __FUNCTION__, __LINE__)

err_t            tcp_shutdown(struct tcp_pcb *pcb, int shut_rx, int shut_tx);

/* Flags for "apiflags" parameter in tcp_write */
#define TCP_WRITE_FLAG_COPY 0x01
#define TCP_WRITE_FLAG_MORE 0x02

#if (LWIP_TCP_WRITE_STAGE_OPT)
err_t            tcp_write_stage(struct tcp_pcb *pcb, const void *arg, u16_t len, err_t err_in);

err_t            tcp_write_checks(struct tcp_pcb *pcb, u16_t len);
#endif

err_t            tcp_write   (struct tcp_pcb *pcb, const void *dataptr, u16_t len,
                              u8_t apiflags);

void             tcp_setprio (struct tcp_pcb *pcb, u8_t prio);

#define TCP_PRIO_MIN    1
#define TCP_PRIO_NORMAL 64
#define TCP_PRIO_MAX    127

err_t            tcp_output_fn  (struct tcp_pcb *pcb, const char *func, const int line);
#define          tcp_output(pcb)  tcp_output_fn(pcb, __FUNCTION__, __LINE__)

const char* tcp_debug_state_str(enum tcp_state s);

#if LWIP_IPV6
struct tcp_pcb * tcp_new_ip6 (void);
#define          tcp_bind_ip6(pcb, ip6addr, port) \
                   tcp_bind(pcb, ip6_2_ip(ip6addr), port)
#define          tcp_connect_ip6(pcb, ip6addr, port, connected) \
                   tcp_connect(pcb, ip6_2_ip(ip6addr), port, connected)
struct tcp_pcb * tcp_listen_dual_with_backlog(struct tcp_pcb *pcb, u8_t backlog);
#define          tcp_listen_dual(pcb) tcp_listen_dual_with_backlog(pcb, TCP_DEFAULT_LISTEN_BACKLOG)
#else /* LWIP_IPV6 */
#define          tcp_listen_dual_with_backlog(pcb, backlog) tcp_listen_with_backlog(pcb, backlog)
#define          tcp_listen_dual(pcb) tcp_listen(pcb)
#endif /* LWIP_IPV6 */

int tcp_in_process(void);
int tcp_port_in_listen(u16_t port);
int tcp_process_pcb_exist(void);
int tcp_listen_pcb_exist(void);
void tcp_set_max_rtx(struct tcp_pcb *pcb, int max_rtx);
void tcp_set_max_syn_rtx(struct tcp_pcb *pcb, int max_rtx);
void tcp_reset_man_trigger(struct tcp_pcb *pcb);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_TCP */

#endif /* __LWIP_TCP_H__ */
