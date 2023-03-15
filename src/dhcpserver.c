#define _GNU_SOURCE 1

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dhcp4server, LOG_LEVEL_DBG);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <zephyr/net/net_if.h>
#include "dhcpserver.h"
#include "bindings.h"
#include "args.h"
#include "dhcp.h"
#include "options.h"
#include "logging.h"
#include "arpa/inet.h"
#include "zephyr/net/ethernet.h"
#include "dhcpmem.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused"
#pragma GCC diagnostic error "-Wint-conversion"
#pragma GCC diagnostic error "-Wincompatible-pointer-types"
/*
 * Global pool
 */

static address_pool pool;
address_pool *get_pool(void) {
	return &pool;
}
/*
 * Helper functions
 */

static char *
str_ip (uint32_t ip)
{
    static char addrstr[INET_ADDRSTRLEN];
    return inet_ntop(AF_INET, &ip, addrstr, sizeof(addrstr));
}

static char *
str_mac (uint8_t *mac)
{
    static char str[128];

    sprintf(str, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
	    mac[0], mac[1], mac[2],
	    mac[3], mac[4], mac[5]);

    return str;
}

char *
str_status (int status)
{
    switch(status) {
    case B_EMPTY:
	return "empty";
    case PENDING:
	return "pending";
    case ASSOCIATED:
	return "associated";
    case RELEASED:
	return "released";
    case EXPIRED:
	return "expired";
    default:
	return NULL;
    }
}

struct net_pkt *dhcpdv4_create_message(struct net_if *iface,
				       const struct in_addr *src,
				       const struct in_addr *dst,
				       uint8_t * data,
				       size_t size);

/*
 * Network related routines
 */

extern void arp_update(struct net_if *iface,
		       struct in_addr *src,
		       struct net_eth_addr *hwaddr,
		       bool gratuitous,
		       bool force);

void
add_arp_entry (int s, uint8_t *mac, uint32_t ip) //TODO
{
    ARG_UNUSED(s);
    ARG_UNUSED(mac);
    ARG_UNUSED(ip);

    struct net_if * iface = net_if_get_default();
    struct in_addr ip_addr;
    //inet_pton(AF_INET,"192.168.2.2", &ip_addr);
    struct net_eth_addr mac_addr;
    memmove(mac_addr.addr,mac,sizeof(mac_addr.addr));
    ip_addr.s_addr=ip;
    LOG_INF("arp add entry: %s %s",str_ip(ip), str_mac(mac));
    arp_update(iface,&ip_addr, &mac_addr,false,true);

    //    struct arpreq ar;
//    struct sockaddr_in *sock;
//
//    memset(&ar, 0, sizeof(ar));
//
//    /* add a proxy ARP entry for given pair */
//
//    sock = (struct sockaddr_in *) &ar.arp_pa;
//    sock->sin_family = AF_INET;
//    sock->sin_addr.s_addr = ip;
//
//    memcpy(ar.arp_ha.sa_data, mac, 6);
//    ar.arp_flags = ATF_COM; //(ATF_PUBL | ATF_COM);
//
//    strncpy(ar.arp_dev, pool.device, sizeof(ar.arp_dev));
//
//    if (ioctl(s, SIOCSARP, (char *) &ar) < 0)  {
//	perror("error adding entry to arp table");
//    };
}

void
delete_arp_entry (int s, uint8_t *mac, uint32_t ip) //TODO
{
    ARG_UNUSED(s);
    ARG_UNUSED(mac);
    ARG_UNUSED(ip);
//    struct arpreq ar;
//    struct sockaddr_in *sock;
//
//    memset(&ar, 0, sizeof(ar));
//
//    sock = (struct sockaddr_in *) &ar.arp_pa;
//    sock->sin_family = AF_INET;
//    sock->sin_addr.s_addr = ip;
//
//    strncpy(ar.arp_dev, pool.device, sizeof(ar.arp_dev));
//
//    if(ioctl(s, SIOCGARP, (char *) &ar) < 0)  {
//	if (errno != ENXIO) {
//	    perror("error getting arp entry");
//	    return;
//	}
//    };
//
//    if(ip == 0 || memcmp(mac, ar.arp_ha.sa_data, 6) == 0) {
//	if(ioctl(s, SIOCDARP, (char *) &ar) < 0) {
//	    perror("error removing arp table entry");
//	}
//    }
}

int
send_dhcp_reply	(int s, struct sockaddr_in *client_sock, dhcpd_msg *reply)
{
    ARG_UNUSED(s);
    ARG_UNUSED(client_sock);

     size_t len = serialize_option_list(&reply->opts, reply->hdr.options,
				sizeof(reply->hdr) - DHCP_HEADER_SIZE);

    len += DHCP_HEADER_SIZE;

//    int broadcast = 1;
//    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
//	LOG_ERR("setsockopt");
//	return -1;
//    }
//    inet_pton(AF_INET, "255.255.255.255", &client_sock->sin_addr.s_addr);
//    //client_sock->sin_addr.s_addr = reply->hdr.yiaddr; // use the address assigned by us
//
//    if(reply->hdr.yiaddr != 0) {
//	add_arp_entry(s, reply->hdr.chaddr, reply->hdr.yiaddr);
//    }
//
//    if ((ret = sendto(s, reply, len, 0, (struct sockaddr *)client_sock, sizeof(*client_sock))) < 0) {
//	LOG_ERR("sendto failed");
//	return -1;
//    }

    struct net_if * iface = net_if_get_by_index(pool.device_index);
    if (iface) {
	//iface->config.dhcpv4.xid++;
	struct in_addr src = iface->config.ip.ipv4->unicast[0].address.in_addr;
	struct net_pkt *pkt =
		dhcpdv4_create_message(iface, &src, net_ipv4_broadcast_address(), (uint8_t*)reply, len);
	if (!pkt) {
		goto fail;
	}

	if (net_send_data(pkt) < 0) {
		goto fail;
	} else {
		printk("dhcpd4 packet sent, size=%u\n", len);
	}

	return 0;
    } else {
	LOG_ERR("Invalid interface index %d", pool.device_index);
    }
fail:
    return -1;
}

/*
 * Message handling routines.
 */

uint8_t
expand_request (dhcpd_msg *request, size_t len)
{
    init_option_list(&request->opts);
    
    if (request->hdr.hlen < 1 || request->hdr.hlen > 16)
	return 0;

    if(parse_options_to_list(&request->opts, (dhcp_option *)request->hdr.options,
			     len - DHCP_HEADER_SIZE) == 0)
	return 0;
    
    dhcp_option *type_opt = search_option(&request->opts, DHCP_MESSAGE_TYPE);
    
    if (type_opt == NULL)
	return 0;

    uint8_t type = type_opt->data[0];
    
    return type;
}

int
init_reply (dhcpd_msg *request, dhcpd_msg *reply)
{
    memset(&reply->hdr, 0, sizeof(reply->hdr));

    init_option_list(&reply->opts);
    
    reply->hdr.op = BOOTREPLY;

    reply->hdr.htype = request->hdr.htype;
    reply->hdr.hlen  = request->hdr.hlen;

    reply->hdr.xid   = request->hdr.xid;
    reply->hdr.flags = request->hdr.flags;
     
    reply->hdr.giaddr = request->hdr.giaddr;
    
    memcpy(reply->hdr.chaddr, request->hdr.chaddr, request->hdr.hlen);

    return 1;
}

void
fill_requested_dhcp_options (dhcp_option *requested_opts, dhcp_option_list *reply_opts)
{
    uint8_t len = requested_opts->len;
    uint8_t *id = requested_opts->data;

    int i;
    for (i = 0; i < len; i++) {
	    
	if(id[i] != 0) {
	    dhcp_option *opt = search_option(&pool.options, id[i]);

	    if(opt != NULL)
		append_option(reply_opts, opt);
	}
	    
    }
}

int
fill_dhcp_reply (dhcpd_msg *request, dhcpd_msg *reply,
		 address_binding *binding, uint8_t type)
{
    static dhcp_option type_opt, server_id_opt;

    type_opt.id = DHCP_MESSAGE_TYPE;
    type_opt.len = 1;
    type_opt.data[0] = type;
    append_option(&reply->opts, &type_opt);

    server_id_opt.id = SERVER_IDENTIFIER;
    server_id_opt.len = 4;
    memcpy(server_id_opt.data, &pool.server_id, sizeof(pool.server_id));
    append_option(&reply->opts, &server_id_opt);
    
    if(binding != NULL) {
	reply->hdr.yiaddr = binding->address;
    }
    
    if (type != DHCP_NAK) {
	dhcp_option *requested_opts = search_option(&request->opts, PARAMETER_REQUEST_LIST);

	if (requested_opts)
	    fill_requested_dhcp_options(requested_opts, &reply->opts);
    }
    
    return type;
}

int
serve_dhcp_discover (dhcpd_msg *request, dhcpd_msg *reply)
{  
    address_binding *binding = search_binding(&pool.bindings, request->hdr.chaddr,
					      request->hdr.hlen, STATIC, B_EMPTY);

    if (binding) { // a static binding has been configured for this client

        log_info("Offer %s to %s (static), %s status %sexpired",
                 str_ip(binding->address), str_mac(request->hdr.chaddr),
                 str_status(binding->status),
                 binding->binding_time + binding->lease_time < time(NULL) ? "" : "not ");
            
        if (binding->binding_time + binding->lease_time < time(NULL)) {
	    binding->status = PENDING;
	    binding->binding_time = time(NULL);
	    binding->lease_time = pool.pending_time;
	}
            
        return fill_dhcp_reply(request, reply, binding, DHCP_OFFER);

    }

    else { // use dynamic pool

        /* If an address is available, the new address
           SHOULD be chosen as follows: */

	binding = search_binding(&pool.bindings, request->hdr.chaddr,
				 request->hdr.hlen, DYNAMIC, B_EMPTY);

        if (binding) {

            /* The client's current address as recorded in the client's current
               binding, ELSE */

            /* The client's previous address as recorded in the client's (now
               expired or released) binding, if that address is in the server's
               pool of available addresses and not already allocated, ELSE */

	    log_info("Offer %s to %s, %s status %sexpired",
		     str_ip(binding->address), str_mac(request->hdr.chaddr),
		     str_status(binding->status),
		     binding->binding_time + binding->lease_time < time(NULL) ? "" : "not ");

	    if (binding->binding_time + binding->lease_time < time(NULL)) {
		binding->status = PENDING;
		binding->binding_time = time(NULL);
		binding->lease_time = pool.pending_time;
	    }
	    
            return fill_dhcp_reply(request, reply, binding, DHCP_OFFER);

        } else {

	    /* The address requested in the 'Requested IP Address' option, if that
	       address is valid and not already allocated, ELSE */

	    /* A new address allocated from the server's pool of available
	       addresses; the address is selected based on the subnet from which
	       the message was received (if 'giaddr' is 0) or on the address of
	       the relay agent that forwarded the message ('giaddr' when not 0). */

	    // TODO: extract requested IP address
	    uint32_t address = 0;
	    dhcp_option *address_opt =
		search_option(&request->opts, REQUESTED_IP_ADDRESS);

	    if(address_opt != NULL)
		memcpy(&address, address_opt->data, sizeof(address));
	    
	    binding = new_dynamic_binding(&pool.bindings, &pool.indexes, address,
					  request->hdr.chaddr, request->hdr.hlen);

	    if (binding == NULL) {
		log_info("Can not offer an address to %s, no address available.",
			 str_mac(request->hdr.chaddr));
		
		return 0;
	    }

	    log_info("Offer %s to %s, %s status %sexpired",
		     str_ip(binding->address), str_mac(request->hdr.chaddr),
		     str_status(binding->status),
		     binding->binding_time + binding->lease_time < time(NULL) ? "" : "not ");
	    
	    if (binding->binding_time + binding->lease_time < time(NULL)) {
		binding->status = PENDING;
		binding->binding_time = time(NULL);
		binding->lease_time = pool.pending_time;
	    }

	    return fill_dhcp_reply(request, reply, binding, DHCP_OFFER);
	}

    }

    // should NOT reach here...
}

int
serve_dhcp_request (dhcpd_msg *request, dhcpd_msg *reply)
{
    address_binding *binding = search_binding(&pool.bindings, request->hdr.chaddr,
					      request->hdr.hlen, STATIC_OR_DYNAMIC, PENDING);

    uint32_t server_id = 0;
    dhcp_option *server_id_opt = search_option(&request->opts, SERVER_IDENTIFIER);

    if(server_id_opt != NULL)
	memcpy(&server_id, server_id_opt->data, sizeof(server_id));
    
    if (server_id == pool.server_id) { // this request is an answer to our offer

	if (binding != NULL) {

	    log_info("Ack %s to %s, associated",
		     str_ip(binding->address), str_mac(request->hdr.chaddr));

	    binding->status = ASSOCIATED;
	    binding->lease_time = pool.lease_time;
	    
	    return fill_dhcp_reply(request, reply, binding, DHCP_ACK);
	
	} else {

	    log_info("Nak to %s, not associated",
		     str_mac(request->hdr.chaddr));
		    
	    return fill_dhcp_reply(request, reply, NULL, DHCP_NAK);
	}

    } else if (server_id != 0) { // answer to the offer of another server

	log_info("Clearing %s of %s, accepted another server offer",
		 str_ip(binding->address), str_mac(request->hdr.chaddr));
		    
	binding->status = B_EMPTY;
	binding->lease_time = 0;
	
	return 0;

    }

    // malformed request...
    return 0;
}

int
serve_dhcp_decline (dhcpd_msg *request, dhcpd_msg *reply)
{
    ARG_UNUSED(reply);
    address_binding *binding = search_binding(&pool.bindings, request->hdr.chaddr,
					      request->hdr.hlen, STATIC_OR_DYNAMIC, PENDING);

    if(binding != NULL) {
	log_info("Declined %s by %s",
		 str_ip(binding->address), str_mac(request->hdr.chaddr));

	binding->status = B_EMPTY;
    }

    return 0;
}

int
serve_dhcp_release (dhcpd_msg *request, dhcpd_msg *reply)
{
    ARG_UNUSED(reply);
    address_binding *binding = search_binding(&pool.bindings, request->hdr.chaddr,
					      request->hdr.hlen, STATIC_OR_DYNAMIC, ASSOCIATED);

    if(binding != NULL) {
	log_info("Released %s by %s",
		 str_mac(request->hdr.chaddr), str_ip(binding->address));

	binding->status = RELEASED;
    }

    return 0;
}

int
serve_dhcp_inform (dhcpd_msg *request, dhcpd_msg *reply)
{
    log_info("Info to %s", str_mac(request->hdr.chaddr));
    return fill_dhcp_reply(request, reply, NULL, DHCP_ACK);
}

/*
 * Dispatch client DHCP messages to the correct handling routines
 */

void
message_dispatcher (int s, bool * stop)
{
    if (!stop)
	return ;

    while (!(*stop)) {
	struct sockaddr_in client_sock;
	socklen_t slen = sizeof(client_sock);
	size_t len;

	dhcpd_msg request;
	dhcpd_msg reply;

	uint8_t type;

	fd_set readfds;
	struct timeval timeout;

	timeout.tv_usec = 100000;
	timeout.tv_sec  = 0;

	FD_ZERO(&readfds);
	FD_SET(s, &readfds);

	int ready = select(s+1, &readfds, NULL, NULL, &timeout);

	if (ready == 0) {
	    continue ;
	} else if (ready == -1) {
	    LOG_ERR("%s: Error on select ()",__func__);
	}

	if((len = recvfrom(s, &request.hdr, sizeof(request.hdr), 0, (struct sockaddr *)&client_sock, &slen)) < DHCP_HEADER_SIZE + 5) {
	    continue; // TODO: check the magic number 300
	}

	if(request.hdr.op != BOOTREQUEST)
	    continue;
	
	printk("%s.%u: request received 1\n",str_ip(client_sock.sin_addr.s_addr), ntohs(client_sock.sin_port));
	if((type = expand_request(&request, len)) == 0) {
	    log_error("%s.%u: invalid request received",
		      str_ip(client_sock.sin_addr.s_addr), ntohs(client_sock.sin_port));
	    continue;
	}
	printk("%s.%u: request received 2\n",str_ip(client_sock.sin_addr.s_addr), ntohs(client_sock.sin_port));
	init_reply(&request, &reply);
	printk("type:%d\n",type);
	switch (type) {

	case DHCP_DISCOVER:
            type = serve_dhcp_discover(&request, &reply);
	    break;

	case DHCP_REQUEST:
	    type = serve_dhcp_request(&request, &reply);
	    break;
	    
	case DHCP_DECLINE:
	    type = serve_dhcp_decline(&request, &reply);
	    break;
	    
	case DHCP_RELEASE:
	    type = serve_dhcp_release(&request, &reply);
	    break;
	    
	case DHCP_INFORM:
	    type = serve_dhcp_inform(&request, &reply);
	    break;
	    
	default:
	    printk("%s.%u: request with invalid DHCP message type option 0x%02x\n",
		   str_ip(client_sock.sin_addr.s_addr), ntohs(client_sock.sin_port),type);
	    break;
	
	}

	if(type != 0) {
	    printk("AAA1\n");
	    send_dhcp_reply(s, &client_sock, &reply);
	    printk("AAA2\n");
	}
	delete_option_list(&request.opts);
	printk("AAA3\n");
	delete_option_list(&reply.opts);
	printk("AAA4\n");

    }

}
extern int net_ipv4_create_full(struct net_pkt *pkt,
				const struct in_addr *src,
				const struct in_addr *dst,
				uint8_t tos,
				uint16_t id,
				uint8_t flags,
				uint16_t offset,
				uint8_t ttl);

static void
dhcpd4_task (bool *stop, void *p2, void *p3)
{
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    LOG_INF("dpcpd4 started");
    int s;
    struct sockaddr_in server_sock;

     if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
          LOG_ERR("server: socket() error %s", strerror(errno));
	  return ;
     }

     server_sock.sin_family = AF_INET;
     server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
     server_sock.sin_port = htons(67);

     if (bind(s, (struct sockaddr *) &server_sock, sizeof(server_sock)) == -1) {
         LOG_ERR("server: bind() %s", strerror(errno));
         close(s);
	 return;
     }

     LOG_INF("dhcpd4 server: listening on %d", ntohs(server_sock.sin_port));

     /* Message processing loop */
     
     message_dispatcher(s, stop);

     close(s);
     LOG_INF("dpcpd4 finished");
}

#define DHCPD4_TASK_PRIO              21u
#define DHCPD4_TASK_STK_SIZE         2048u

struct k_thread dhcpd4_task_thread_data;
static K_THREAD_STACK_DEFINE(dhcpd4_task_stk, DHCPD4_TASK_STK_SIZE);
static k_tid_t dhcpd4_tid = NULL;
static bool dhcpd4_task_stop = false;
int dhcpd4_start(struct net_if *iface)
{

     address_pool * pool = get_pool();

     if (dhcpd4_tid) {
	 LOG_ERR("dhcpd4 already started.");
	 return -1;
     }

     memset(pool, 0, sizeof(*pool));
     init_binding_list(&pool->bindings);
     init_option_list(&pool->options);

     if (!iface) {
	 if (pool->device_index < 0) {
	    iface=net_if_get_default();
	 }
     }
     pool->device_index=net_if_get_by_iface(iface);

     if (TAILQ_EMPTY(&(pool->options))) {
	 int result = 0;
	 result += parse_and_add_option(pool, "BROADCAST_ADDRESS","192.168.2.255");
	 result += parse_and_add_option(pool, "SUBNET_MASK","255.255.255.0");
	 if (result){
	    LOG_ERR("dhcpd not started. parse_and_add_option error");
	    return 0;
	 }

	 uint32_t *first = NULL, *last=NULL;
	 parse_ip ("192.168.2.2", (void**)&first);
	 parse_ip ("192.168.2.254", (void**)&last);
	 if (first && last) {
	    pool->indexes.first = *first;
	    pool->indexes.last = *last;
	    pool->indexes.current = *first;
	 }
	 dhcp4_free(first);
	 dhcp4_free(last);
     }

     if (pool->device_index>0) {
	 pool->server_id=iface->config.ip.ipv4->unicast[0].address.in_addr.s_addr;
	 dhcpd4_task_stop = false;
	 dhcpd4_tid = k_thread_create(&dhcpd4_task_thread_data, dhcpd4_task_stk,
			 K_THREAD_STACK_SIZEOF(dhcpd4_task_stk), (k_thread_entry_t)dhcpd4_task,
			 &dhcpd4_task_stop, 0, 0, DHCPD4_TASK_PRIO, 0, K_NO_WAIT);
	 if (dhcpd4_tid) {
	    k_thread_name_set(&dhcpd4_task_thread_data, "dhcpd4 Task");
	    return 0;
	 }
	 return -1;

     }
     LOG_ERR("dhcpd not started. Invalid interface index");
     return -1;
}

int dhcpd4_stop(void) {
     dhcpd4_task_stop = true;
     k_thread_join(&dhcpd4_task_thread_data, K_FOREVER);
     dhcpd4_tid=NULL;
     LOG_ERR("dhcpd thread joined");
     return 0;
}


#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>


#define DHCPV4_SERVER_PORT	67
#define DHCPV4_CLIENT_PORT	68


int net_ipv4_create(struct net_pkt *pkt,
		    const struct in_addr *src,
		    const struct in_addr *dst);

int net_udp_create(struct net_pkt *pkt, uint16_t src_port, uint16_t dst_port);

int net_ipv4_finalize(struct net_pkt *pkt, uint8_t next_header_proto);

struct net_pkt *dhcpdv4_create_message(struct net_if *iface,
				       const struct in_addr *src,
				       const struct in_addr *dst,
				       uint8_t * data,
				       size_t size){
     LOG_HEXDUMP_INF(data, size, "dhcpd reply");
     NET_PKT_DATA_ACCESS_DEFINE(dhcp_access, struct dhcpd_msg);
     const struct in_addr *addr;
     struct net_pkt *pkt;

     if (src == NULL) {
	 addr = net_ipv4_unspecified_address();
     } else {
	 addr = src;
     }

     pkt = net_pkt_alloc_with_buffer(iface, size, AF_INET,
				     IPPROTO_UDP, K_FOREVER);

     net_pkt_set_ipv4_ttl(pkt, 0xFF);

     if (net_ipv4_create(pkt, addr, dst) ||
	 net_udp_create(pkt, htons(DHCPV4_SERVER_PORT),
			htons(DHCPV4_CLIENT_PORT))) {
	 goto fail;
     }

     dhcpd_msg *msg = net_pkt_get_data(pkt, &dhcp_access);

     (void)msg;
     //memmove(msg, data, size);


     net_pkt_write(pkt,data, size);

     net_pkt_cursor_init(pkt);

     net_ipv4_finalize(pkt, IPPROTO_UDP);

     return pkt;

fail:
     NET_DBG("Message creation failed");
     net_pkt_unref(pkt);

     return NULL;
}