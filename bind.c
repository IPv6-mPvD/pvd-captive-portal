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

static	void	parseOneLine(
			char *Line,
			struct sockaddr_in *v4,
			struct sockaddr_in6 *v6)
{
	char	*pt = strchr(Line, ':');

	if (pt != NULL) {
		if (v6) {
			char str[64];
			memset(v6, 0, sizeof(*v6));
			v6->sin6_family = AF_INET6;
			inet_pton(AF_INET6, pt, &v6->sin6_addr);
			inet_ntop(AF_INET6, &v6->sin6_addr, str, 64);
			printf("(v6 = %s) ", str);
		}
	}
	else {
		if (v4) {
			memset(v4, 0, sizeof(*v4));
			v4->sin_family = AF_INET;
			v4->sin_addr.s_addr = inet_addr(Line);
			v4->sin_port = htons(0);
			printf("(v4=%s) ", inet_ntoa(v4->sin_addr));
		}
	}
}

/*
 * get_adresses : reads the BIND_ADDR_FILE and fills in either the v4 or v6
 * output variable (only one is non NULL) depending of the line read
 */
static	int	get_addresses(struct sockaddr_in *v4, struct sockaddr_in6 *v6)
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
			parseOneLine(Line, v4, v6);
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
		family == AF_INET ? NULL : (struct sockaddr_in6 *) &sockaddr)) {
		if (real_bind(sock,
			      (struct sockaddr *) &sockaddr,
			      sizeof(sockaddr)) == -1) {
			printf("real_bind -> %s\n", strerror(errno));
		}
	}
	printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	return real_connect(sock, sk, sl);
}

