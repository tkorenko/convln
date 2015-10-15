#!/bin/sh
TAR=/usr/bin/tar
TMP=/tmp
TS="201510141014"

if [ $# -ne 2 ]; then
    echo "* * *   Usage: `basename $0` <distname> <version>   * * *"
    exit 1
else
    PROGNAME=$1
    PROGVER=$2
fi

RELEASE=${PROGNAME}-${PROGVER}
ARCHNAME=${RELEASE}.tar
DISTNAME=${ARCHNAME}.gz

# echo RELEASE $RELEASE
# echo ARCHNAME $ARCHNAME
# echo DISTNAME $DISTNAME

rm -rfd ${TMP}/${PROGNAME}
mkdir -p ${TMP}/${PROGNAME}/${RELEASE}
cp -R * ${TMP}/${PROGNAME}/${RELEASE}/

CWD=`pwd`
cd ${TMP}/${PROGNAME}

find ${RELEASE} -d -exec touch -t ${TS} {} \;
${TAR} -c -f ${ARCHNAME} `find ${RELEASE} -type f | sort`
touch -t ${TS} ${ARCHNAME}
gzip -9 -k -n ${ARCHNAME}
# ls -la
# sha256 -r ${ARCHNAME} ${DISTNAME}

mv ${DISTNAME} ${CWD}/

cd ${CWD}

rm -rfd ${TMP}/${PROGNAME}

exit 0
