#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dhcp4server, LOG_LEVEL_DBG);

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "args.h"
#include "options.h"
#include <zephyr/shell/shell.h>
#include "dhcpmem.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic error "-Wunused"
#pragma GCC diagnostic error "-Wint-conversion"

#define PR(sh, level, fmt, ...)					\
	do {								\
		if (sh) {						\
			shell_fprintf(sh, level, fmt, ##__VA_ARGS__); \
		} else {						\
			printk(fmt, ##__VA_ARGS__);			\
		}							\
	} while (false)

void usage(const struct shell *sh,char *msg)
{
	shell_warn(sh,"%s", USAGE_TXT);
    
    if (msg) {
	    shell_error(sh,"%s", msg);
    }
    
}

int parse_and_add_option(address_pool *pool, char * name, char * value)
{

    dhcp_option *option = dhcp4_calloc(1, sizeof(*option));
    if (!option) {
	    return -1;
    }
    uint8_t id = parse_option(option, name, value);
    if (id == 0) {
	    LOG_ERR("error: invalid dhcp option specified: %s,%s", name, value);
	    option_free(&option);
	    return -1;
    }
    return append_option(&pool->options, option);
}

int parse_args(const struct shell *sh, int argc, char *argv[], address_pool *pool)
{
    int c;

    opterr = 0;

    struct net_if * iface = net_if_get_default();
    if (iface) {
	pool->device_index=net_if_get_by_iface(iface);
    } else {
	pool->device_index=-1;
    }

    while ((c = getopt (argc, argv, "a:d:o:p:s:")) != -1)
	switch (c) {

	case 'a': // parse IP address pool
	    {
		char *opt    = dhcp4_strdup(optarg);
		char *sfirst = opt;
		char *slast  = strchr(opt, ',');
	    
		if (slast == NULL) {
			usage(sh,"error: comma not present in option -a.");
			return -1;
		}
		*slast = '\0';
		slast++;
	    
		uint32_t *first, *last;
		
		if (parse_ip(sfirst, (void **)&first) != 4) {
			usage(sh,"error: invalid first ip in address pool.");
			return -1;
		}

		if (parse_ip(slast, (void **)&last) != 4) {
			usage(sh, "error: invalid last ip in address pool.");
			return -1;
		}

		pool->indexes.first   = *first;
		pool->indexes.last    = *last;
		pool->indexes.current = *first;
		
		dhcp4_free(first);
		dhcp4_free(last);
		dhcp4_free(opt);
		
		break;
	    }

	case 'd': // network device to use
	    {
		uint32_t * di;
		if(parse_long(optarg, (void **)&di) != 4) {
			usage(sh, "error: invalid device index.");
			return -1;
		}
		struct net_if *iface = net_if_get_by_index(*di);
		if (!iface) {
			dhcp4_free(di);
			pool->device_index=-1;
			usage(sh, "error: invalid device index. device not found");
			return -1;
		}
		pool->device_index=net_if_get_by_iface(iface);
		dhcp4_free(di);
		break;
	    }
	    
	case 'o': // parse dhcp option
	    {
		uint8_t id;

		char *opt   = dhcp4_strdup(optarg);
		char *name  = opt;
		char *value = strchr(opt, ',');
		
		if (value == NULL) {
			usage(sh, "error: comma not present in option -o.");
			return -1;
		}
		*value = '\0';
		value++;
		
		dhcp_option *option = dhcp4_calloc(1, sizeof(*option));
		
		if((id = parse_option(option, name, value)) == 0) {
		    shell_error(sh,"error: invalid dhcp option specified: %s,%s",name, value);
		    return -1;
		}
		
		append_option(&pool->options, option);

		if(option->id == IP_ADDRESS_LEASE_TIME)
		    pool->lease_time = ntohl(*((uint32_t *)option->data));

		dhcp4_free(option);
		dhcp4_free(opt);
		break;
	    }

	case 'p': // parse pending time
	    {
		time_t *t;

		if(parse_long(optarg, (void **)&t) != 4) {
		    usage(sh, "error: invalid pending time.");
		    return -1;
		}

		pool->pending_time = *t;
		dhcp4_free(t);
		break;
	    }

	case 's': // static binding
	    {
		char *opt = dhcp4_strdup(optarg);
		char *shw  = opt;
		char *sip  = strchr(opt, ',');
		
		if (sip == NULL) {
		    usage(sh, "error: comma not present in option -s.");
		    return -1;
		}
		*sip = '\0';
		    sip++;
		
		uint32_t *ip;
		uint8_t  *hw;
		
		if (parse_mac(shw, (void **)&hw) != 6) {
		    usage(sh, "error: invalid mac address in static binding.");
		    return -1;
		}
		
		if (parse_ip(sip, (void **)&ip) != 4) {
		    usage(sh, "error: invalid ip in static binding.");
		    return -1;
		}
		
		add_binding(&pool->bindings, *ip, hw, 6, 1);
		
		dhcp4_free(ip);
		dhcp4_free(hw);
		dhcp4_free(opt);
	    }
	    break;
	case '?':
	    usage(sh, NULL);
	    break;
	default:
	    usage(sh, NULL);
	}

    if(optind >= argc) {
	usage(sh, "error: server address not provided.");
	return -1;
    }

    uint32_t *ip;

    if (parse_ip(argv[optind], (void **)&ip) != 4) {
	usage(sh, "error: invalid server address.");
	return -1;
    }

    pool->server_id = *ip;
    
    dhcp4_free(ip);
    return 0;

}
int dhcpd4_start(struct net_if *iface);
static int cmd_dhcpd_start(const struct shell *sh, size_t argc, char *argv[]) {
    address_pool * pool = get_pool();
    memset(pool, 0, sizeof(*pool));
    init_binding_list(&pool->bindings);
    init_option_list(&pool->options);

    if (parse_args(sh, argc, argv, pool)!=0) {
	return 1;
    }

    dhcpd4_start(NULL);
    return 0;
}

static int cmd_dhcpd_stop(const struct shell *sh, size_t argc, char *argv[]) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	PR(sh, SHELL_ERROR, "unimplemented yet");
	shell_help(sh);
	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(dhcpd_commands,
	SHELL_CMD(start, NULL, "dhcpd4 start", cmd_dhcpd_start),
	SHELL_CMD(stop, NULL,  "dhcpd4 stop", cmd_dhcpd_stop),
	SHELL_SUBCMD_SET_END
);


SHELL_CMD_REGISTER(dhcpd4, &dhcpd_commands, "dhcpd4 commands", NULL);


static int dhcpd_shell_init(const struct device *unused)
{
    ARG_UNUSED(unused);
    return 0;
}

SYS_INIT(dhcpd_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);