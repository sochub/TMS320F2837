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
#ifndef __LWIP_IP4_H__
#define __LWIP_IP4_H__

#include "opt.h"

#include "def.h"
#include "pbuf.h"
#include "ip_addr.h"
#include "ip6_addr.h"
#include "err.h"
#include "netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Currently, the function ip_output_if_opt() is only used with IGMP */
#define IP_OPTIONS_SEND   LWIP_IGMP

#define IP_HLEN 20

#define IP_PROTO_ICMP    1
#define IP_PROTO_IGMP    2
#define IP_PROTO_UDP     17
#define IP_PROTO_UDPLITE 136
#define IP_PROTO_TCP     6
#define IP_PROTO_GRE     47
#define IP_PROTO_ESP     50
#define IP_PROTO_AH      51

/** This operates on a void* by loading the first byte */
#define IP_HDR_GET_VERSION(ptr)   ((*(u8_t*)(ptr)) >> 4)

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "bpstruct.h"
#endif

#undef IP_RF
#undef IP_DF
#undef IP_MF
#undef IP_OFFMASK

#define IP_RF       0x8000U   /* reserved fragment flag */
#define IP_DF       0x4000U   /* dont fragment flag */
#define IP_MF       0x2000U   /* more fragments flag */
#define IP_OFFMASK  0x1fffU   /* mask for fragmenting bits */

struct ipsec_esp_hdr {
	u32_t 	spi;    	    /**< Security Parameters Index      */
	u32_t	sequence;       /**< Sequence number                */
};

PACK_STRUCT_BEGIN
struct ip_hdr {
  /* version / header length */
  PACK_STRUCT_FIELD(u8_t _v_hl);
  /* type of service */
  PACK_STRUCT_FIELD(u8_t _tos);
  /* total length */
  PACK_STRUCT_FIELD(u16_t _len);
  /* identification */
  PACK_STRUCT_FIELD(u16_t _id);
  /* fragment offset field */
  PACK_STRUCT_FIELD(u16_t _offset);
  /* time to live */
  PACK_STRUCT_FIELD(u8_t _ttl);
  /* protocol*/
  PACK_STRUCT_FIELD(u8_t _proto);
  /* checksum */
  PACK_STRUCT_FIELD(u16_t _chksum);
  /* source and destination IP addresses */
  PACK_STRUCT_FIELD(ip_addr_p_t src);
  PACK_STRUCT_FIELD(ip_addr_p_t dest);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "epstruct.h"
#endif

#define IPH_V(hdr)  ((hdr)->_v_hl >> 4)
#define IPH_HL(hdr) ((hdr)->_v_hl & 0x0f)
#define IPH_TOS(hdr) ((hdr)->_tos)
#define IPH_LEN(hdr) ((hdr)->_len)
#define IPH_ID(hdr) ((hdr)->_id)
#define IPH_OFFSET(hdr) ((hdr)->_offset)
#define IPH_TTL(hdr) ((hdr)->_ttl)
#define IPH_PROTO(hdr) ((hdr)->_proto)
#define IPH_CHKSUM(hdr) ((hdr)->_chksum)

#define IPH_VHL_SET(hdr, v, hl) (hdr)->_v_hl = (((v) << 4) | (hl))
#define IPH_TOS_SET(hdr, tos) (hdr)->_tos = (tos)
#define IPH_LEN_SET(hdr, len) (hdr)->_len = (len)
#define IPH_ID_SET(hdr, id) (hdr)->_id = (id)
#define IPH_OFFSET_SET(hdr, off) (hdr)->_offset = (off)
#define IPH_TTL_SET(hdr, ttl) (hdr)->_ttl = (u8_t)(ttl)
#define IPH_PROTO_SET(hdr, proto) (hdr)->_proto = (u8_t)(proto)
#define IPH_CHKSUM_SET(hdr, chksum) (hdr)->_chksum = (chksum)


#define ip_init() /* Compatibility define, no init needed. */
struct netif *ip_route(ip_addr_t *dest);
struct netif *ip_route_lc(struct pbuf *p, struct netif *inp);
struct netif *ip_route_from_app_fn(u32_t ip4_src, u32_t ip4_dest,  const char* file, const int line);
#define ip_route_from_app(ip4_src, ip4_dest) ip_route_from_app_fn((ip4_src), (ip4_dest), __FUNCTION__, __LINE__)

err_t ip_input(struct pbuf *p, struct netif *inp);
err_t ip4_input(struct pbuf *p, struct netif *inp);
err_t ip_output(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto);
err_t ip_output_if(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto,
       struct netif *netif);
#if LWIP_NETIF_HWADDRHINT
err_t ip_output_hinted(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto, u8_t *addr_hint);
#endif /* LWIP_NETIF_HWADDRHINT */
#if IP_OPTIONS_SEND
err_t ip_output_if_opt(struct pbuf *p, ip_addr_t *src, ip_addr_t *dest,
       u8_t ttl, u8_t tos, u8_t proto, struct netif *netif, void *ip_options,
       u16_t optlen);
#endif /* IP_OPTIONS_SEND */

#define ip_netif_get_local_ip(netif) (((netif) != NULL) ? (&((netif)->ip_addr)) : NULL)
#define ip_netif_get_local_ipX(netif) (((netif) != NULL) ? ip_2_ipX(&((netif)->ip_addr)) : NULL)

#if LWIP_DEBUG
void ip_debug_print(struct pbuf *p);
#else
#define ip_debug_print(p)
#endif
void ip_pbuf_log_input(struct netif *inp, struct ip_hdr *iphdr, u16_t payloadLen, struct pbuf *p);
void ip_pbuf_log_output_fn(struct netif *inp, struct ip_hdr *iphdr, u16_t payloadLen, err_t ret, struct pbuf *p, const char *func);
#define ip_pbuf_log_output(inp, iphdr, len, ret, p) ip_pbuf_log_output_fn(inp, iphdr, len, ret, p, __FUNCTION__)

#ifdef __cplusplus
}
#endif

#endif /* __LWIP_IP_H__ */


