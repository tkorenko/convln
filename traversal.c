/*
 * Copyright (c) 2015, Taras Korenko
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "nameconv.h"
#include "traversal.h"

#define CONVTONAME_LEN MAXNAMLEN

extern struct stat dst_sb;

static int linkAt(int fd1, const char * n1, int fd2, const char * n2);
static int digInto(DIR * pd1, const char * n1, DIR * pd2, const char * n2);
static int convertName(const char * fromName, size_t fromNameLen,
    char * toName, size_t toNameBufSz);
static int setCurDir(DIR * pd);
static int isThisDstDir(DIR * pd);

int
processNode(DIR * srcDp, DIR * dstDp)
{
	struct dirent *pDe;
	char convToName[CONVTONAME_LEN + 1];

#ifdef DEBUG
	printf("processNode():\n\tsrcDp = %p\n\tdstDp = %p\n",
	    srcDp, dstDp);
#endif /* #ifdef DEBUG */

	if (isThisDstDir(srcDp)) {
		fprintf(stderr, "%s:%d: Source and destination trees overlap."
		    "  Please choose another dstDir (-D)!\n",
		    __FILE__, __LINE__);

		return 0;
	}

	++cfg.dirsQty;

	rewinddir(srcDp);
	while ((pDe = readdir(srcDp)) != NULL) {
		// skip specials ('.' and '..')
		if ((pDe->d_namlen == 1 && pDe->d_name[0] == '.') ||
		    (pDe->d_namlen == 2 && pDe->d_name[0] == '.' &&
			pDe->d_name[1] == '.')) continue;

		// we can handle 'dir' and 'file', just skip over
		// other file types
		if (pDe->d_type != DT_DIR && pDe->d_type != DT_REG)
		    continue;

		if (convertName(pDe->d_name, pDe->d_namlen,
		      convToName, CONVTONAME_LEN) != 0) {
			continue;
		}

		if (pDe->d_type == DT_REG) {
			linkAt(dirfd(srcDp), pDe->d_name,
			    dirfd(dstDp), convToName);
		} else if (pDe->d_type == DT_DIR) {
			digInto(srcDp, pDe->d_name,
			    dstDp, convToName);
		}

#ifdef DEBUG
		printf("%4d %s\n", pDe->d_type, pDe->d_name);
#endif /* #ifdef DEBUG */
	}

	return 0;
}

static int
isThisDstDir(DIR * pd)
{
	struct stat loc_sb;

	if (fstat(dirfd(pd), &loc_sb) < 0) {
		fprintf(stderr, "%s:%d fstat() failed: %s\n",
		    __FILE__, __LINE__, strerror(errno));

		return -1;
	}

	if (dst_sb.st_dev == loc_sb.st_dev &&
	    dst_sb.st_ino == loc_sb.st_ino) {
		return 1;
	}

	return 0;
}

static int
linkAt(int fd1, const char * n1, int fd2, const char * n2)
{
	int rs = 0;

#ifdef DEBUG
	printf("linkAt(%d, %s, %d, %s)\n", fd1, n1, fd2, n2);
#endif /* #ifdef DEBUG */

	if ((rs = linkat(fd1, n1, fd2, n2, 0)) < 0) {
		fprintf(stderr, "%s:%d: linkat() failed: %s\n",
		    __FILE__, __LINE__, strerror(errno));
	} else {
		++cfg.filesQty;
	}

	return rs;
}

static int
setCurDir(DIR * pDir)
{
	int rs;

	if ((rs = fchdir(dirfd(pDir))) < 0) {
		fprintf(stderr, "%s:%d: fchdir(%d) failed: %s\n",
		    __FILE__, __LINE__, dirfd(pDir), strerror(errno));
	}

	return rs;
}

static int
digInto(DIR * pSrcParentDir, const char * srcDirName,
    DIR * pDstParentDir, const char * dstDirName)
{
	int rs = 0;
	DIR *srcDp = NULL, *dstDp = NULL;

#ifdef DEBUG
	printf("digInto(%p, %s, %p, %s)\n", pSrcParentDir, srcDirName,
	    pDstParentDir, dstDirName);
#endif /* #ifdef DEBUG */

	if (setCurDir(pSrcParentDir) < 0) {
		return -1;
	}

	if ((srcDp = opendir(srcDirName)) == NULL) {
		fprintf(stderr, "%s:%d: opendir('%s') failed: %s\n",
		    __FILE__, __LINE__, srcDirName, strerror(errno));

		return -1;
	}

	// is the dstTree under the srcTree?
	if (isThisDstDir(srcDp)) {
		fprintf(stderr, "%s:%d: Skipping the destination tree root "
		    "directory.\n", __FILE__, __LINE__);

		goto diOut1;	// release resources correctly
	}

	if ((rs = mkdirat(dirfd(pDstParentDir), dstDirName, 0777)) < 0) {
		fprintf(stderr, "%s:%d: mkdirat(%s) failed: %s\n",
		    __FILE__, __LINE__, dstDirName, strerror(errno));

		goto diOut1;	// release resources correctly
	}

	if (setCurDir(pDstParentDir) < 0) {
		goto diOut1;	// release resources correctly
	}

	if ((dstDp = opendir(dstDirName)) == NULL) {
		fprintf(stderr, "%s:%d: opendir('%s') failed: %s\n",
		    __FILE__, __LINE__, dstDirName, strerror(errno));

		goto diOut1;	// release resources correctly
	}

	rs = processNode(srcDp, dstDp);

	closedir(dstDp);
diOut1:
	closedir(srcDp);

	return rs;
}

static int
convertName(const char * fromName, size_t fromNameLen,
    char * toName, size_t toNameBufSz)
{
	int rs = nameConvTranslate(fromName, fromNameLen,
	    toName, toNameBufSz);

	return rs;
}

