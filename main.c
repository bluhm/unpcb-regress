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

#include <sys/wait.h>

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

int	nserver = 1;
int	nclient = 2;

struct child {
	pid_t	c_pid;
	void	(*c_func)(void);
} *children;

void	server(void);
void	client(void);

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
	}
	for (i = 0; i < nclient; c++, i++) {
		c->c_func = client;
	}

	for (c = children; c->c_func; c++) {
		pid = fork();
		switch (pid) {
		case -1:
			err(1, "fork");
		case 0:
			c->c_func();
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
	}

	return (0);
}

void
server(void)
{
}

void
client(void)
{
}
