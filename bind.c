/*
 * Compile on Linux with:
 * gcc -nostartfiles -fpic -shared bind.c -o bind.so -ldl -D_GNU_SOURCE
 *
 * This works by writing first in a file a set of lines, each line being
 * an IPv6 or IPv4 address
 * Before doing the connec(), the preloaded library will attempt to bind
 * the socket to either the last v4 or v6 (depending on the socket's family)
 * address read from the file
 *
 * The file name is specified by the BIND_ADDR_FILE environment variable
*/



#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

#define INIT_GRACE_TIME 2

int (*real_bind)(int, const struct sockaddr *, socklen_t);
int (*real_connect)(int, const struct sockaddr *, socklen_t);

void _init(void)
{
	const char *err;
	real_bind = dlsym(RTLD_NEXT, "bind");
	if ((err = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(bind): %s\n", err);
	}

	real_connect = dlsym(RTLD_NEXT, "connect");
	if ((err = dlerror()) != NULL) {
		fprintf(stderr, "dlsym(connect): %s\n", err);
	}
}

/*
 * parseOneLine : lines read from the shared file have the following
 * format :
 * 1) ipv6 address (only the last one matters)
 * 2) ipv4 address (only the last one matters)
 * 3) <dest ipv6 address> <src ipv6 address>
 * 4) <*> <src ipv6 address> (only the last one matters)
 * 5) <dest ipv4 address> <src ipv4 address>
 * 6) <*> <src ipv4 address> (only the last one matters)
 */
static	void	parseOneLine(
			char *Line,
			struct sockaddr_in *v4,
			struct sockaddr_in6 *v6,
			struct sockaddr_in *peerv4,
			struct sockaddr_in6 *peerv6)
{
	struct sockaddr_in dstv4;
	struct sockaddr_in6 dstv6;
	char	*ptSrcAddr = strchr(Line, ' ');
	char	*pt = strchr(Line, ':');
	char	str[64];

	if (ptSrcAddr != NULL) {
		*ptSrcAddr++ = '\0';

		/*
		 * Format 3, 4, 5 or 6
		 */
		if (pt != NULL) {
			if (peerv6) {
				if (Line[0] != '*') {
					inet_pton(AF_INET6, Line, &dstv6.sin6_addr);
					if (memcmp(&dstv6.sin6_addr,
						   &peerv6->sin6_addr,
						   sizeof(dstv6.sin6_addr)) != 0) {
						/*
						 * destination address does not match
						 */
						return;
					}
				}
			}
		}
		else {
			if (peerv4) {
				if (Line[0] != '*') {
					dstv4.sin_addr.s_addr = inet_addr(Line);
					if (memcmp(&dstv4.sin_addr.s_addr,
						   &peerv4->sin_addr.s_addr,
						   sizeof(dstv4.sin_addr.s_addr)) != 0) {
						/*
						 * destination address does not match
						 */
						return;
					}
				}
			}
		}
	}
	else {
		ptSrcAddr = Line;
	}

	if (pt != NULL) {
		if (v6) {
			memset(v6, 0, sizeof(*v6));
			v6->sin6_family = AF_INET6;
			inet_pton(AF_INET6, ptSrcAddr, &v6->sin6_addr);
			inet_ntop(AF_INET6, &v6->sin6_addr, str, 64);
			printf("(v6 = %s) ", str);
		}
	}
	else {
		if (v4) {
			memset(v4, 0, sizeof(*v4));
			v4->sin_family = AF_INET;
			v4->sin_addr.s_addr = inet_addr(ptSrcAddr);
			v4->sin_port = htons(0);
			printf("(v4=%s) ", inet_ntoa(v4->sin_addr));
		}
	}
}

/*
 * get_adresses : reads the BIND_ADDR_FILE and fills in either the v4 or v6
 * output variable (only one is non NULL) depending of the line read
 */
static	int	get_addresses(
			struct sockaddr_in *v4,
			struct sockaddr_in6 *v6,
			struct sockaddr_in *peerv4,
			struct sockaddr_in6 *peerv6)
{
	int i, j;
	FILE *fp;
	char Line[1024];
	char *bindAddrFile = getenv("BIND_ADDR_FILE");

	if (v4) {
		v4->sin_family = 0;
	} else {
		v6->sin6_family = 0;
	}

	if (bindAddrFile && (fp = fopen(bindAddrFile, "r")) != NULL) {
		while (fgets(Line, sizeof(Line) - 1, fp) != NULL) {
			parseOneLine(Line, v4, v6, peerv4, peerv6);
		}
		fclose(fp);
	}
	return(v4 ? v4->sin_family != 0 : v6->sin6_family != 0);
}

int connect(int sock, const struct sockaddr *sk, socklen_t sl)
{
	struct sockaddr_storage sockaddr;
	int family = sk->sa_family;
	int port = ((struct sockaddr_in *) sk)->sin_port;
	char str[64];

	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	printf("connect : requested sk sa_family : %s\n",
		family == AF_INET ? "AF_INET" :
		family == AF_INET6 ? "AF_INET6" : "<no AF_INETx>");

	if (family != AF_INET && family != AF_INET6) {
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		return real_connect(sock, sk, sl);
	}

	if (family == AF_INET6)
		inet_ntop(AF_INET6, &((struct sockaddr_in6 *) sk)->sin6_addr, str, 63);
	else
		inet_ntop(AF_INET, &((struct sockaddr_in *) sk)->sin_addr, str, 63);

	printf("connect : %s:%d ", str, port);

	if (get_addresses(
		family == AF_INET ? (struct sockaddr_in *) &sockaddr : NULL,
		family == AF_INET ? NULL : (struct sockaddr_in6 *) &sockaddr,
		family == AF_INET ? (struct sockaddr_in *) sk : NULL,
		family == AF_INET ? NULL : (struct sockaddr_in6 *) sk)) {
		if (real_bind(sock,
			      (struct sockaddr *) &sockaddr,
			      sizeof(sockaddr)) == -1) {
			printf("real_bind -> %s\n", strerror(errno));
		}
	}
	printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	return real_connect(sock, sk, sl);
}

