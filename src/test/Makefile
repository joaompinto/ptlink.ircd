#
# Makefile for test progs
#
#
# $Id: Makefile,v 1.3 2005/08/27 16:23:50 jpinto Exp $
#
#
CPPFLAGS = -I../../include
CFLAGS   = -O2 -Wall
# CPPFLAGS = -I../../include -DDEAD_CODE

ALL: strncpy_t match_t ilimit_test


ilimit_test: ilimit_test.o
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $^
match_t: match_t.o ../match.o
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $^

strncpy_t: strncpy_t.o
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $^

parsetest: parsetest.o
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $^

.PHONY: clean distclean

clean:
	rm -f *.o match_t ilimit_test strncpy_t

distclean: clean
	rm -f .depend

-include .depend
