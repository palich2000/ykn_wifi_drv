/**
 * @file dhcpmem.h
 * @author Tsaplay Yuriy (y.tsaplay@yukonww.com)
 *
 * @brief
 *
 */
#ifndef ZEPHYR_DHCPMEM_H
#define ZEPHYR_DHCPMEM_H
#include <stddef.h>

void *dhcp4_malloc(size_t size);
void *dhcp4_calloc(size_t nmemb, size_t size);

#define dhcp4_free(buffer) \
	while(true) {           \
        if (buffer) {    \
		_dhcp4_free(buffer);     \
		(buffer)=NULL;}                 \
        break;                 \
	}

void _dhcp4_free(void * ptr);
char *dhcp4_strdup(const char *str);

#endif // ZEPHYR_DHCPMEM_H