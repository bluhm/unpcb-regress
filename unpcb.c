/*
 * Copyright (c) 2015 Alexander Bluhm <bluhm@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include <errno.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	nserver = 3;
int	nclient = 100;

struct child {
	pid_t	c_pid;
	void	(*c_func)(const struct child *);
	char	*c_name;
	char	*c_sock;
} *children;

void	server(const struct child *);
void	client(const struct child *);

int
main(int argc, char *argv[])
{
	struct child *c;
	pid_t pid;
	int i, status;

	children = calloc(nserver + nclient + 1, sizeof(struct child));
	if (children == NULL)
		err(1, "calloc");

	for (c = children, i = 0; i < nserver; c++, i++) {
		c->c_func = server;
		if (asprintf(&c->c_name, "server-%d", i) == -1)
			err(1, "asprintf name server %d", i);
		if (asprintf(&c->c_sock, "sock-%d", i) == -1)
			err(1, "asprintf sock server %d", i);
	}
	for (i = 0; i < nclient; c++, i++) {
		c->c_func = client;
		if (asprintf(&c->c_name, "client-%d", i) == -1)
			err(1, "asprintf name client %d", i);
		if (asprintf(&c->c_sock, "sock-%d", i % nserver) == -1)
			err(1, "asprintf sock client %d", i);
	}

	for (c = children; c->c_func; c++) {
		pid = fork();
		switch (pid) {
		case -1:
			err(1, "fork");
		case 0:
			setproctitle("%s", c->c_name);
			c->c_func(c);
			_exit(0);
		default:
			c->c_pid = pid;
		}
	}

	while (1) {
		pid = wait(&status);
		if (pid == -1)
			err(1, "wait");
		for (c = children; c->c_func; c++) {
			if (c->c_pid == pid)
				break;
		}
		if (c->c_pid != pid)
			err(1, "pid %d", pid);
		if (status != 0)
			warnx("pid %d, status %d", pid, status);
		pid = fork();
		switch (pid) {
		case -1:
			err(1, "fork");
		case 0:
			setproctitle("%s", c->c_name);
			c->c_func(c);
			_exit(0);
		default:
			c->c_pid = pid;
		}
	}

	return (0);
}

void
server(const struct child *c)
{
	char buf[1024];
	struct sockaddr_un sun;
	socklen_t sunlen;
	ssize_t n;
	int s, val;

 redo:
	if ((s = socket(PF_UNIX, SOCK_DGRAM, 0)) == -1)
		err(1, "%s socket", c->c_name);
	val = 256*1024;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val)) == -1)
		err(1, "%s setsockopt SO_RCVBUF", c->c_name);
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, c->c_sock, sizeof(sun.sun_path));
	sunlen = sizeof(sun);
	unlink(sun.sun_path);
	if (bind(s, (struct sockaddr *)&sun, sunlen) == -1)
		err(1, "%s bind", c->c_name);
	while (1) {
		switch (arc4random_uniform(10000)) {
			case 0:
				if (close(s) == -1)
					err(1, "%s close", c->c_name);
				goto redo;
			case 1:
				_exit(0);
			case 2:
				sleep(1);
			default:
				if ((n = recv(s, buf, sizeof(buf), 0)) == -1)
					err(1, "%s recv", c->c_name);
				printf("%s recv %zd\n", c->c_name, n);
		}
	}
}

void
client(const struct child *c)
{
	char buf[1024];
	struct sockaddr_un sun;
	socklen_t sunlen;
	ssize_t n;
	int s, val;

	arc4random_buf(buf, sizeof(buf));
 redo:
	if ((s = socket(PF_UNIX, SOCK_DGRAM, 0)) == -1)
		err(1, "%s socket", c->c_name);
	val = 256*1024;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &val, sizeof(val)) == -1)
		err(1, "%s setsockopt SO_SNDBUF", c->c_name);
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, c->c_sock, sizeof(sun.sun_path));
	sunlen = sizeof(sun);
	if (connect(s, (struct sockaddr *)&sun, sunlen) == -1)
		err(1, "%s connect", c->c_name);
	while (1) {
		switch (arc4random_uniform(1000)) {
			case 0:
 reconnect:
				if (close(s) == -1)
					err(1, "%s close", c->c_name);
				goto redo;
			case 1:
				_exit(0);
			default:
				if ((n = send(s, buf, arc4random_uniform(
				    sizeof(buf)), 0)) == -1) {
					if (errno == ECONNRESET ||
					    errno == ECONNREFUSED)
						goto reconnect;
					err(1, "%s send", c->c_name);
					sleep(1);
				}
				printf("%s send %zd\n", c->c_name, n);
				sleep(0);
		}
	}
}
