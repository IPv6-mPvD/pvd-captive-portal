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
			struct sockaddr *srcAddr,
			const struct sockaddr *peer)
{
#define	v4	((struct sockaddr_in *) srcAddr)
#define	v6	((struct sockaddr_in6 *) srcAddr)

	int	family = peer->sa_family;
	int	isv6 = family == AF_INET6;
	char	*ptSrcAddr = strchr(Line, ' ');
	char	*ptColon = strchr(Line, ':');
	char	str[64];
	char	dstAddr[32];	/* 16 bytes are sufficient */
	void	*addr;

	if ((ptColon != NULL && family != AF_INET6) ||
	    (ptColon == NULL && family != AF_INET)) {
		return;
	}

	if (ptSrcAddr != NULL) {
		*ptSrcAddr++ = '\0';

		/*
		 * Format 3, 4, 5 or 6 : check if dest address field of the line
		 * is matching the peer address
		 */
		if (Line[0] != '*') {
			inet_pton(family, Line, dstAddr);
			if (memcmp(
				dstAddr,
				isv6 ?
				    (void *) &((struct sockaddr_in6 *) peer)->sin6_addr :
				    (void *) &((struct sockaddr_in *) peer)->sin_addr,
				isv6 ?
				    sizeof(struct in6_addr) : 
				    sizeof(struct in_addr)) != 0) {
				return;
			}
		}
	}

	srcAddr->sa_family = family;
	addr = isv6 ? (void *) &v6->sin6_addr : (void *) &v4->sin_addr;

	inet_pton(family, ptSrcAddr ? ptSrcAddr : Line, addr);
	inet_ntop(family, addr, str, 63);
	printf(isv6 ? "(v6 = %s) " : "(v4 %s) ", str);
}

/*
 * get_adresses : reads the BIND_ADDR_FILE and fills in the srcAdr
 * output variable with the source address field of a matching line
 * of the file. A matching line is a line for which :
 * 1) the family type matches (contains either ipv6 or ipv4 addresses
 * specifications)
 * 2) the destination address is the one specified by the peer (or is '*')
 */
static	int	getSrcAddress(
			struct sockaddr *srcAddr,
			const struct sockaddr *peer)
{
	int i, j;
	FILE *fp;
	char Line[1024];
	char *bindAddrFile = getenv("BIND_ADDR_FILE");

	srcAddr->sa_family = 0;

	if (bindAddrFile && (fp = fopen(bindAddrFile, "r")) != NULL) {
		while (fgets(Line, sizeof(Line) - 1, fp) != NULL) {
			parseOneLine(Line, srcAddr, peer);
		}
		fclose(fp);
	}
	return(srcAddr->sa_family != 0);
}

int connect(int sock, const struct sockaddr *sk, socklen_t sl)
{
	struct sockaddr_storage srcAddr;
	int family = sk->sa_family;
	int port = ((struct sockaddr_in *) sk)->sin_port;
	char str[64];

	memset(&srcAddr, 0, sizeof(srcAddr));

	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	printf("connect : requested sk sa_family : %s\n",
		family == AF_INET ? "AF_INET" :
		family == AF_INET6 ? "AF_INET6" : "<no AF_INETx>");

	if (family != AF_INET && family != AF_INET6) {
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		return real_connect(sock, sk, sl);
	}

	if (family == AF_INET6)
		inet_ntop(family, &((struct sockaddr_in6 *) sk)->sin6_addr, str, 63);
	else
		inet_ntop(family, &((struct sockaddr_in *) sk)->sin_addr, str, 63);

	printf("connect : %s:%d ", str, port);

	if (getSrcAddress((struct sockaddr *) &srcAddr, sk)) {
		if (real_bind(sock,
			      (struct sockaddr *) &srcAddr,
			      sizeof(srcAddr)) == -1) {
			printf("real_bind -> %s\n", strerror(errno));
		}
	}
	printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	return real_connect(sock, sk, sl);
}

