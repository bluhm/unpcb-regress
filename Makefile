# compile client and server program for send and receive test packets

SRCS =		unpcb.c
CLEANFILES +=	*.o stamp-* ktrace.out sock
CDIAGFLAGS +=	-Wall -Werror \
		-Wbad-function-cast \
		-Wcast-align \
		-Wcast-qual \
		-Wdeclaration-after-statement \
		-Wextra \
		-Wmissing-declarations \
		-Wmissing-prototypes \
		-Wpointer-arith \
		-Wshadow \
		-Wsign-compare \
		-Wstrict-prototypes \
		-Wuninitialized \
		-Wunused -Wno-unused-parameter
DEBUG =		-g
NOMAN =		yes
WARNINGS =	yes
PROG =		unpcb

.include <bsd.regress.mk>
