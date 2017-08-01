/*
   Copyright (C) 2000  Daniel Ryde

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   */

/*
   LD_PRELOAD library to make bind and connect to use a virtual
   IP address as localaddress. Specified via the enviroment
   variable BIND_ADDR.

   Compile on Linux with:
   gcc -nostartfiles -fpic -shared bind.c -o bind.so -ldl -D_GNU_SOURCE


   Example in bash to make inetd only listen to the localhost
   lo interface, thus disabling remote connections and only
   enable to/from localhost:

   BIND_ADDR="127.0.0.1" LD_PRELOAD=./bind.so /sbin/inetd


   Example in bash to use your virtual IP as your outgoing
   sourceaddress for ircII:

   BIND_ADDR="your-virt-ip" LD_PRELOAD=./bind.so ircII

   Note that you have to set up your servers virtual IP first.


   This program was made by Daniel Ryde
email: daniel@ryde.net
web:   http://www.ryde.net/

TODO: I would like to extend it to the accept calls too, like a
general tcp-wrapper. Also like an junkbuster for web-banners.
For libc5 you need to replace socklen_t with int.
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
#include <time.h>

#define INIT_GRACE_TIME 2

int (*real_bind)(int, const struct sockaddr *, socklen_t);
int (*real_connect)(int, const struct sockaddr *, socklen_t);

clock_t begin;
double time_spent() {
	return (double)(clock() - begin) / CLOCKS_PER_SEC;
}

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

	begin = clock();
}

void get_addresses(struct sockaddr_in *v4, struct sockaddr_in6 *v6)
{
	char buff[1024];
	char *addresses = NULL;
	char *v4_addr = NULL, *v6_addr = NULL;
	char *bindAddrFile = getenv("BIND_ADDR_FILE");

	if (bindAddrFile) {
		FILE *f;
		if ((f = fopen(bindAddrFile, "r")) == NULL) {
			perror(bindAddrFile);
		} else {
			int i, j;

			if ((i = fread(buff, 1, 1023, f)) > 0) {
				for (j = 0; j < i && buff[j] != '\n'; j++);
				buff[j] = '\0';
				addresses = buff;
				fclose(f);
			}
		}
	}

	if (addresses == NULL)
		addresses = getenv("BIND_ADDR");

	printf("get_addresses : address used : %s\n", addresses);

	if (addresses) {
		v6_addr = addresses;
		v4_addr = strchr(addresses, ',');
		if (v4_addr) {
			*v4_addr++ = '\0';
		}
		if (strchr(addresses, ':') == NULL) {
			v6_addr = v4_addr;
			v4_addr = addresses;
		}
	}

	v4->sin_family = 0;
	v6->sin6_family = 0;

	if (v4_addr) {
		v4->sin_family = AF_INET;
		v4->sin_addr.s_addr = inet_addr(v4_addr);
		v4->sin_port = htons(0);
		printf("(v4=%s)", inet_ntoa(v4->sin_addr));
	}
	if (v6_addr) {
		char str[64];
		memset(v6, 0, sizeof(*v6));
		v6->sin6_family = AF_INET6;
		inet_pton(AF_INET6, v6_addr, &v6->sin6_addr);
		inet_ntop(AF_INET6, &v6->sin6_addr, str, 64);
		printf("(v6=%s)", str);
	}
	printf("  ");
}

int get_address2(struct sockaddr_storage *l, int family)
{
	struct sockaddr_storage foo;
	if (family == AF_INET) {
		printf("AF_INET requested\n");
		get_addresses((struct sockaddr_in *) l, (struct sockaddr_in6 *) &foo);
	} else {
		printf("AF_INET6 requested\n");
		get_addresses((struct sockaddr_in *) &foo, (struct sockaddr_in6 *) l);
	}
	return ((struct sockaddr_in *) l)->sin_family == family;
}

int connect(int fd, const struct sockaddr *sk, socklen_t sl)
{
	struct sockaddr_in *rsk_in;
	struct sockaddr_storage skaddr;
	char str[64];

	printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	printf("connect : requested sk sa_family : %s\n",
		sk->sa_family == AF_INET ? "AF_INET" :
		sk->sa_family == AF_INET6 ? "AF_INET6" : "<no AF_INETx>");

	printf("connect : requested sk sin_family : %s\n",
		((struct sockaddr_in *)sk)->sin_family == AF_INET ? "AF_INET" :
		((struct sockaddr_in *)sk)->sin_family == AF_INET6 ? "AF_INET6" : "<no AF_INETx>");

	if (sk->sa_family != AF_INET && sk->sa_family != AF_INET6) {
		printf("connect: Weird family %d\n", sk->sa_family);
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		return real_connect(fd, sk, sl);
	}

	if (sk->sa_family == AF_INET6)
		inet_ntop(AF_INET6, &((struct sockaddr_in6 *)sk)->sin6_addr, str, 64);
	else
		inet_ntop(AF_INET, &((struct sockaddr_in *)sk)->sin_addr, str, 64);

	rsk_in = (struct sockaddr_in *) sk;
	printf("connect : %d %s:%d ", fd, str, ntohs(rsk_in->sin_port));
	if (get_address2(&skaddr, ((struct sockaddr_in *)sk)->sin_family)) {
		int rc = real_bind(fd, (struct sockaddr *) &skaddr, sizeof(skaddr));
		printf("real_bind -> %d: %s\n", rc, rc != 0 ? strerror(errno) : "ok");
	} else if (time_spent() < INIT_GRACE_TIME) {
		printf("Grace time\n");
	} else {
		printf("Wrong address family (%d)\n", ((struct sockaddr *) &skaddr)->sa_family);
	}
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	return real_connect(fd, sk, sl);
}

