/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wfm200test, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
//#include <zephyr/device.h>
//#include <zephyr/devicetree.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>

/*
#include "sl_wfx.h"
#include "sl_wfx_host_init.h"
#include "sl_wfx_host.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
*/
// #if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
// #error"WM89xx series coder is not defined in DTS"
// #endif

// uint8_t buffer[4];

// sl_wfx_context_t   wifi;

// #define WIFI_NODE DT_NODELABEL(wifi)
//#define WIFI_NODE DT_ALIAS(wifi0)
//const struct device *const wifi_drv_0 = DEVICE_DT_GET(WIFI_NODE);

// extern void gpio_setup(void);

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
//	struct net_if *iface = net_if_get_default();

	LOG_INF("--- start ");

//	if (!device_is_ready(wifi_drv_0)) {
//		LOG_ERR("wfm200 devices not ready");
//		return;
//	}
//	LOG_INF("wfm200 devices ready");
	// k_sem_take(&wfx_init_sem,K_FOREVER);

//	LOG_INF("device name = %s\n", wifi_drv_0->name);
//
//	iface = net_if_lookup_by_dev(wifi_drv_0);

	net_mgmt_init_event_callback(&wifi_shell_mgmt_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT|NET_EVENT_WIFI_DISCONNECT_RESULT);

	net_mgmt_add_event_callback(&wifi_shell_mgmt_cb);

	net_mgmt_init_event_callback(&mgmt_if_cb, my_if_mgmt_event_handler, _NET_EVENT_IF_BASE);
	net_mgmt_add_event_callback(&mgmt_if_cb);

	// wfm200_scan();

	// wfm200_mgmt_scan();
	// wifi_scan();

	// net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	// wifi_scan();
	// printk("addr net_mgmt = %x\n");

	/*
	net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	k_msleep(6000);

	net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
	*/

	while (1) {
		k_msleep(100);
	}
	k_msleep(15000);
	printk("--- end \n");

	/**
	if (!device_is_ready(wifi_drv)) {
	    printk("wfm200 devices not ready\n");
	    return;
	}
	*/

	// TODO
	//  net_mgmt_event_wait
}

// my comment