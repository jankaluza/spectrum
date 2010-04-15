/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include "dnsresolver.h"
#include "dnsquery.h"
#include "network.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "string.h"
#include "log.h"

extern LogClass Log_;

struct ResolverData {
	PurpleDnsQueryData *query_data;
	PurpleDnsQueryResolvedCallback resolved_cb;
	PurpleDnsQueryFailedCallback failed_cb;
	gchar *error_message;
	GSList *hosts;
	bool destroyed;
};

static GHashTable *query_data_cache = NULL;

static gboolean resolve_ip(ResolverData *data) {
	PurpleDnsQueryData *query_data = (PurpleDnsQueryData *) data->query_data;
	struct sockaddr_in sin;
	if (inet_aton(purple_dnsquery_get_host(query_data), &sin.sin_addr)) {
		// The given "hostname" is actually an IP address, so we don't need to do anything.
		Log("DNSResolver", "Resolving " << purple_dnsquery_get_host(query_data) << ": It's IP, don't do anything.");
		GSList *hosts = NULL;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(purple_dnsquery_get_port(query_data));
		hosts = g_slist_append(hosts, GINT_TO_POINTER(sizeof(sin)));
		hosts = g_slist_append(hosts, g_memdup(&sin, sizeof(sin)));
		data->resolved_cb(query_data, hosts);
		delete data;
		return TRUE;
	}

	return FALSE;
}

static gboolean
dns_main_thread_cb(gpointer d)
{
	ResolverData *data = (ResolverData *) d;
	if (data->destroyed) {
		delete data;
		return FALSE;
	}
	PurpleDnsQueryData *query_data = data->query_data;
	g_hash_table_remove(query_data_cache, query_data);

	/* We're done, so purple_dnsquery_destroy() shouldn't think it is canceling an in-progress lookup */
// 	query_data->resolver = NULL;

	if (data->error_message != NULL) {
		Log("DNSResolver", "Resolving " << purple_dnsquery_get_host(query_data) << ": Resolving failed: " << data->error_message);
		data->failed_cb(query_data, data->error_message);
	}
	else
	{
		Log("DNSResolver", "Resolving " << purple_dnsquery_get_host(query_data) << ": Successfully resolved");
		GSList *hosts;
		/* We don't want purple_dns_query_resolved() to free(hosts) */
		hosts = data->hosts;
		data->hosts = NULL;
		data->resolved_cb(query_data, hosts);
	}
	delete data;

	return FALSE;
}

static gpointer dns_thread(gpointer d) {
	ResolverData *data = (ResolverData *) d;
	PurpleDnsQueryData *query_data;
#ifdef HAVE_GETADDRINFO
	int rc;
	struct addrinfo hints, *res, *tmp;
	char servname[20];
#else
	struct sockaddr_in sin;
	struct hostent *hp;
#endif
	char *hostname;

	query_data = data->query_data;

// Currently we don't support IDN

// #ifdef USE_IDN
// 	if (!dns_str_is_ascii(purple_dnsquery_get_host(query_data))) {
// 		rc = purple_network_convert_idn_to_ascii(purple_dnsquery_get_host(query_data), &hostname);
// 		if (rc != 0) {
// 			data->error_message = g_strdup_printf(_("Error converting %s "
// 					"to punycode: %d"), purple_dnsquery_get_host(query_data), rc);
// 			/* back to main thread */
// 			purple_timeout_add(0, dns_main_thread_cb, query_data);
// 			return 0;
// 		}
// 	} else /* intentional fallthru */
// #endif
	hostname = g_strdup(purple_dnsquery_get_host(query_data));

#ifdef HAVE_GETADDRINFO
	g_snprintf(servname, sizeof(servname), "%d", purple_dnsquery_get_port(query_data));
	memset(&hints,0,sizeof(hints));

	/*
	 * This is only used to convert a service
	 * name to a port number. As we know we are
	 * passing a number already, we know this
	 * value will not be really used by the C
	 * library.
	 */
	hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif /* AI_ADDRCONFIG */
	if ((rc = getaddrinfo(hostname, servname, &hints, &res)) == 0) {
		tmp = res;
		while(res) {
			data->hosts = g_slist_append(data->hosts,
				GSIZE_TO_POINTER(res->ai_addrlen));
			data->hosts = g_slist_append(data->hosts,
				g_memdup(res->ai_addr, res->ai_addrlen));
			res = res->ai_next;
		}
		freeaddrinfo(tmp);
	} else {
		data->error_message = g_strdup_printf(_("Error resolving %s:\n%s"), purple_dnsquery_get_host(query_data), purple_gai_strerror(rc));
	}
#else
	if ((hp = gethostbyname(hostname))) {
		memset(&sin, 0, sizeof(struct sockaddr_in));
		memcpy(&sin.sin_addr.s_addr, hp->h_addr, hp->h_length);
		sin.sin_family = hp->h_addrtype;
		sin.sin_port = htons(purple_dnsquery_get_port(query_data));

		data->hosts = g_slist_append(data->hosts,
				GSIZE_TO_POINTER(sizeof(sin)));
		data->hosts = g_slist_append(data->hosts,
				g_memdup(&sin, sizeof(sin)));
	} else {
		data->error_message = g_strdup_printf("Error resolving %s: %d", purple_dnsquery_get_host(query_data), h_errno);
	}
#endif
	g_free(hostname);

	/* back to main thread */
	purple_timeout_add(0, dns_main_thread_cb, data);

	return 0;
}

static gboolean resolve_host(PurpleDnsQueryData *query_data, PurpleDnsQueryResolvedCallback resolved_cb, PurpleDnsQueryFailedCallback failed_cb) {
	GError *err = NULL;

	ResolverData *data = new ResolverData;
	data->query_data = query_data;
	data->resolved_cb = resolved_cb;
	data->failed_cb = failed_cb;
	data->error_message = NULL;
	data->hosts = NULL;
	data->destroyed = false;

	if (!resolve_ip(data)) {
		Log("DNSResolver", "Resolving " << purple_dnsquery_get_host(query_data) << ": Starting resolver thread.");
		if (g_thread_create(dns_thread, data, FALSE, &err) == NULL)
		{
			Log("DNSResolver", "Resolving " << purple_dnsquery_get_host(query_data) << ": Resolver thread couldn't been started.");
			char message[1024];
			g_snprintf(message, sizeof(message), "Thread creation failure: %s",
					(err && err->message) ? err->message : "Unknown reason");
			g_error_free(err);
			failed_cb(query_data, message);
			delete data;
			return FALSE;
		}
		else {
			g_hash_table_replace(query_data_cache, query_data, data);
		}
	}

	return TRUE;
}

static void destroy(PurpleDnsQueryData *query_data) {
	Log("DNSResolver", "Destroying " << purple_dnsquery_get_host(query_data));
	ResolverData *data = (ResolverData *) g_hash_table_lookup(query_data_cache, query_data);
	if (data) {
		Log("DNSResolver", "Destroying " << purple_dnsquery_get_host(query_data) << ": will be destroyed soon...");
		data->destroyed = true;
		g_hash_table_remove(query_data_cache, query_data);
	}
}

static PurpleDnsQueryUiOps dnsUiOps =
{
	resolve_host,
	destroy,
	NULL,
	NULL,
	NULL,
	NULL
};

PurpleDnsQueryUiOps * getDNSUiOps() {
	// TODO: this leaks, but there's not proper API to g_hash_table_destroy it somewhere...
	query_data_cache = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
	return &dnsUiOps;
}

