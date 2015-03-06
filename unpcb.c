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
const	char *sockname = "sock";

struct child {
	pid_t	c_pid;
	void	(*c_func)(const char *);
	char	*c_name;
} *children;

void	server(const char *);
void	client(const char *);

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
		asprintf(&c->c_name, "server-%d", i);
	}
	for (i = 0; i < nclient; c++, i++) {
		c->c_func = client;
		asprintf(&c->c_name, "client-%d", i);
	}

	for (c = children; c->c_func; c++) {
		pid = fork();
		switch (pid) {
		case -1:
			err(1, "fork");
		case 0:
			setproctitle("%s", c->c_name);
			c->c_func(c->c_name);
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
			c->c_func(c->c_name);
			_exit(0);
		default:
			c->c_pid = pid;
		}
	}

	return (0);
}

void
server(const char *name)
{
	char buf[1024];
	struct sockaddr_un sun;
	socklen_t sunlen;
	ssize_t n;
	int s;

 redo:
	if ((s = socket(PF_UNIX, SOCK_DGRAM, 0)) == -1)
		err(1, "%s socket", name);
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, sockname, sizeof(sun.sun_path));
	sunlen = sizeof(sun);
	unlink(sun.sun_path);
	if (bind(s, (struct sockaddr *)&sun, sunlen) == -1)
		err(1, "%s bind", name);
	while (1) {
		switch (arc4random_uniform(100)) {
			case 0:
				if (close(s) == -1)
					err(1, "%s close", name);
				goto redo;
			case 1:
				_exit(0);
			case 2:
				sleep(1);
			default:
				if ((n = recv(s, buf, sizeof(buf), 0)) == -1)
					err(1, "%s recv", name);
				printf("%s recv %zd\n", name, n);
		}
	}
}

void
client(const char *name)
{
	char buf[] = "log data";
	struct sockaddr_un sun;
	socklen_t sunlen;
	ssize_t n;
	int s;

 redo:
	if ((s = socket(PF_UNIX, SOCK_DGRAM, 0)) == -1)
		err(1, "%s socket", name);
	sun.sun_len = sizeof(sun);
	sun.sun_family = AF_UNIX;
	strlcpy(sun.sun_path, sockname, sizeof(sun.sun_path));
	sunlen = sizeof(sun);
	if (connect(s, (struct sockaddr *)&sun, sunlen) == -1)
		err(1, "%s connect", name);
	while (1) {
		switch (arc4random_uniform(100)) {
			case 0:
				if (close(s) == -1)
					err(1, "%s close", name);
				goto redo;
			case 1:
				_exit(0);
			default:
				if ((n = send(s, buf, sizeof(buf), 0)) == -1) {
					if (errno != ENOBUFS)
						err(1, "%s send", name);
					sleep(1);
				}
				printf("%s send %zd\n", name, n);
		}
	}
}