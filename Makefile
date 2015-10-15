PREFIX?=/usr/local
MANDIR?=${PREFIX}/man
CFLAGS+= -g -Wall -I/usr/local/include -L/usr/local/lib
#CFLAGS+= -DDEBUG
LDFLAGS= -liconv
PROGNAME=convln
MANNAME=${PROGNAME}.1
PROGVER=0.4
TAR=/usr/bin/tar
TMP=/tmp
RELEASE=${PROGNAME}-${PROGVER}
ARCHNAME=${RELEASE}.tar
DISTNAME=${ARCHNAME}.gz

all	: ${PROGNAME}

${PROGNAME} : convln.o traversal.o config.o nameconv.o general.o
	${CC} ${CFLAGS} -o ${.TARGET} ${.ALLSRC} ${LDFLAGS}

install	: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install -m 755 ${PROGNAME} ${DESTDIR}${PREFIX}/bin
	install -m 444 ${MANNAME} ${DESTDIR}${MANDIR}/man1

dist	: clean
	./mkdist.sh ${PROGNAME} ${PROGVER}
	sha256 -r ${DISTNAME}

clean	:
	rm -f *.o *.core ${PROGNAME} ${DISTNAME}
	rm -rfd aa xxx

test	: all
	@rm -rfd xxx aa
	@mkdir -p aa xxx
	@mkdir aa/бб
	@touch aa/файл1 aa/файл2 aa/бб/файл3 aa/бб/файл4
	@mkfifo aa/фифо5
	./convln -f 'KOI8-R' -t 'CP1251' -D xxx aa

viewman	:
	groff -Tascii -mdoc convln.1 | less

