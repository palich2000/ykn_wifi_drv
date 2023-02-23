/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wfm200test, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>


static struct net_mgmt_event_callback wifi_shell_mgmt_cb;
static struct net_mgmt_event_callback  mgmt_if_cb;

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb, struct net_if *iface)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status->status) {
		LOG_ERR("Connection request failed (%d)", status->status);
	} else {
		LOG_INF("WIFI Connected");
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
	case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb, iface);
		break;
	default:
		break;
	}
}

static void my_if_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				     struct net_if *iface)
{
	int a;
	LOG_INF("[%s]Event %s 0x%X", __FUNCTION__, iface->if_dev->dev->name, mgmt_event);
	switch (mgmt_event) {
	case NET_EVENT_IF_ADMIN_UP:
	case NET_EVENT_IF_UP:
		LOG_INF("Interface up %s", iface->if_dev->dev->name);
		//net_dhcpv4_start(iface);
		break;
	case NET_EVENT_IF_ADMIN_DOWN:
	case NET_EVENT_IF_DOWN:
		LOG_INF("Interface down %s", iface->if_dev->dev->name);
		//net_dhcpv4_stop(iface);
		break;
	default:
		break;
	}
}

void main(void)
{
	LOG_INF("********************************** --- start ***********************************");
	struct net_if * iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (iface) {
		net_dhcpv4_start(iface);
	}

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT|NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&mgmt_if_cb, my_if_mgmt_event_handler, _NET_EVENT_IF_BASE);
	net_mgmt_add_event_callback(&mgmt_if_cb);

	while (1) {
		k_msleep(1000);
	}
}

// my comment