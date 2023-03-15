/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wfm200test, LOG_LEVEL_DBG);

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/timing/timing.h>
#include <zephyr/shell/shell.h>

#include <mbedtls/ecdh.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/ccm.h>
#include "arpa/inet.h"
#include "dhcpserver.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused"

static struct net_mgmt_event_callback wifi_mgmt_cb;
static struct net_mgmt_event_callback  mgmt_if_cb;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb, struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Connection request failed (%d)", status->status);
	} else {
		LOG_INF("WIFI Connected");
		net_if_carrier_on(iface);
		net_dhcpv4_start(iface);
		LOG_INF("Dhcp started");
	}
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb, struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Disconnection request failed (%d)", status->status);
	} else {
		LOG_INF("WIFI Disconnected");
		net_dhcpv4_stop(iface);
		net_if_config_ipv4_put(iface);
		net_if_carrier_off(iface);
		LOG_INF("Dhcp stopped");
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb, iface);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb, iface);
		break;
	default:
		break;
	}
}

static void my_if_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				     struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);
	LOG_INF("[%s]Event interface:%s event:0x%X", __FUNCTION__, iface->if_dev->dev->name, mgmt_event);
	switch (mgmt_event) {
	case NET_EVENT_IF_ADMIN_UP:
	case NET_EVENT_IF_UP:
		LOG_INF("Interface up %s", iface->if_dev->dev->name);
		if (strstr(iface->if_dev->dev->name,"eth@") == iface->if_dev->dev->name) {
			//LOG_INF("dhcp4client for %s started", iface->if_dev->dev->name);
			//net_dhcpv4_start(iface);
			if (net_if_config_ipv4_put(iface) ) {
				LOG_ERR("net_if_config_ipv4_put error");
			}

			memset(&iface->config.dhcpv4,0,sizeof(iface->config.dhcpv4));
			if (iface->config.ip.ipv4) {
				memset(&iface->config.ip.ipv4->gw, 0, sizeof(iface->config.ip.ipv4->gw));
			}


			struct in_addr addr;
			if (net_addr_pton(AF_INET, "192.168.2.1", &addr)) {
				LOG_ERR("Invalid address: %s", "192.168.2.1");
				return;
			}

			if (net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0)==NULL){
				LOG_ERR("net_if_ipv4_addr_add failed");
				return;
			}

			if (net_addr_pton(AF_INET, "255.255.255.0", &addr)) {
				LOG_ERR("Invalid netmask: %s", "255.255.255.0");
				return;
			}

			net_if_ipv4_set_netmask(iface, &addr);

			dhcpd4_start(iface);
		}
		break;
	case NET_EVENT_IF_ADMIN_DOWN:
	case NET_EVENT_IF_DOWN:
		LOG_INF("Interface down %s", iface->if_dev->dev->name);
		if (strstr(iface->if_dev->dev->name,"eth@") == iface->if_dev->dev->name) {
			LOG_INF("dhcp4client for %s stopped", iface->if_dev->dev->name);
			//net_dhcpv4_stop(iface);
			dhcpd4_stop(iface);
		}
		break;
	default:
		break;
	}
}

#define PR(fmt, ...)						\
	shell_fprintf(sh, SHELL_NORMAL, fmt, ##__VA_ARGS__)

static int cmd_test_iface_put(const struct shell *sh, size_t argc, char *argv[]){
	ARG_UNUSED(sh);
	PR("argc=%d\n", argc);
 if (argc==2) {
		int if_index = atoi(argv[1]);
		struct net_if * iface = net_if_get_by_index(if_index);
		if (net_if_config_ipv4_put(iface)!=0){
			PR("net_if_config_ipv4_put failed\n");
		} else {
			PR("net_if_config_ipv4_put ok\n");
			net_dhcpv4_stop(iface);
			memset(&iface->config.dhcpv4,0, sizeof(iface->config.dhcpv4));
		}
 }
 return 0;
}

static int cmd_test_ccm(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	unsigned char key[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
				 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};
	unsigned char nonce[12] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
				   0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};
	//unsigned char header[10] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11, 0x22, 0x33, 0x44};

	unsigned char input[20] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc,
				   0xba, 0x98, 0x76, 0x54, 0x32, 0x10, 0xff, 0xee, 0xdd, 0xcc};
	//unsigned char output[20] = {0};
	unsigned char tag[16] = {0};

	size_t input_len = 20;
	//size_t header_len = 10;
	size_t tag_len = 16;

	mbedtls_ccm_context ctx;
	mbedtls_ccm_init(&ctx);
	mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);

	LOG_HEXDUMP_INF(input, input_len,"input");

	timing_t start_time, end_time;
	uint64_t total_cycles;
	uint64_t total_ns;

	timing_init();
	timing_start();

	start_time = timing_counter_get();

	mbedtls_ccm_encrypt_and_tag(&ctx, input_len,
				    nonce, sizeof(nonce),
				    NULL, 0,
				    input, input,
				    tag, tag_len);

	end_time = timing_counter_get();

	total_cycles = timing_cycles_get(&start_time, &end_time);
	total_ns = timing_cycles_to_ns(total_cycles);

	timing_stop();
	mbedtls_ccm_free(&ctx);
	LOG_HEXDUMP_INF(input, input_len,"output");
	LOG_HEXDUMP_INF(tag, tag_len,"tag");
	LOG_INF("exec time: %lld microseconds", total_ns/1000);
	return 0;
}

extern void arp_update(struct net_if *iface,
		struct in_addr *src,
		struct net_eth_addr *hwaddr,
		bool gratuitous,
		bool force);

static int cmd_test_arp(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	struct net_if * iface = net_if_get_default();
	struct in_addr ip_addr;
	inet_pton(AF_INET,"192.168.2.2", &ip_addr);
	struct net_eth_addr mac_addr={.addr={0,1,2,3,4,5}};
	arp_update(iface,&ip_addr, &mac_addr,false,true);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(test_commands,
	SHELL_CMD(ccm, NULL, "Test ccm alg",cmd_test_ccm),
			       SHELL_CMD(iput, NULL, "Test iface_put",cmd_test_iface_put),
			       SHELL_CMD(arp, NULL, "Test arp_update",cmd_test_arp),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(test, &test_commands, "Testing commands", NULL);

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT |  \
				NET_EVENT_WIFI_TWT)

void main(void)
{
	LOG_INF("********************************** --- start ***********************************");

	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler,
				     _NET_WIFI_EVENT);
	net_mgmt_add_event_callback(&wifi_mgmt_cb);

	net_mgmt_init_event_callback(&mgmt_if_cb, my_if_mgmt_event_handler, _NET_EVENT_IF_BASE);
	net_mgmt_add_event_callback(&mgmt_if_cb);

	while (1) {
		k_msleep(1000);
	}
}

// my comment