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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "convln.h"
#include "general.h"
#include "nameconv.h"
#include "traversal.h"

char src[MAXNAMLEN + 1] = ".";
char dst[MAXNAMLEN + 1] = "convln";
struct stat dst_sb;
int verbose;
static const char *defaultFromCode = "UTF-8";
static const char *defaultToCode = "KOI8-RU";

static void usage(void);
static void help(void);

int
main(int argc, char *argv[])
{
	int ch;

	verbose = 0;
	configInit(&cfg);

	cfg.nameConvFrom = defaultFromCode;
	cfg.nameConvTo = defaultToCode;

	while ((ch = getopt(argc, argv, "cD:f:ht:v")) != -1 ) {
		switch (ch) {
		case 'c':
			// iconv_open(3), look for '//IGNORE'
			cfg.nameConvIgnoreNonconvertable = 1;
			break;
		case 'D':
			strnncpy(dst, MAXNAMLEN + 1, optarg, strlen(optarg));
			break;
		case 'f':
			cfg.nameConvFrom = optarg;
			break;
		case 'h':
			help();
			return 0;
		case 't':
			cfg.nameConvTo = optarg;
			break;
		case 'v':
			verbose++;
			break;
		case '?':
		default:
			usage();
			return -1;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		fprintf(stderr, "Missing source directory, please provide"
		    " one.\n");
		usage();
		return -1;
	}

	nameConvOpen();

	if (mkdir(dst, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0) {
		if (errno != EEXIST) {
			fprintf(stderr, "%s:%d: mkdir(\"%s\") failed: %s\n",
			    __FILE__, __LINE__, dst, strerror(errno));

			goto mOut1;
		} else {
			fprintf(stderr, "Warning: reusing previously created"
			    " directory '%s'\n", dst);
		}
	}

	DIR *srcDp = NULL, *dstDp = NULL;
	if ((dstDp = opendir(dst)) == NULL) {
		fprintf(stderr, "%s:%d: opendir(\"%s\") failed: %s\n",
		    __FILE__, __LINE__, dst, strerror(errno));

		goto mOut1;
	}

	if (fstat(dirfd(dstDp), &dst_sb) < 0) {
		fprintf(stderr, "%s:%d: fstat() failed: %s\n",
		    __FILE__, __LINE__, strerror(errno));

		goto mOut2;
	}

	int i;
	for (i = 0; i < argc; ++i) {
#ifdef DEBUG
		printf("Processing '%s' ...\n", argv[i]);
#endif /* #ifdef DEBUG */
		if ((srcDp = opendir(argv[i])) == NULL) {
			fprintf(stderr, "%s:%d: opendir(\"%s\") failed: %s\n",
			    __FILE__, __LINE__, dst, strerror(errno));
			continue;
		}

		processNode(srcDp, dstDp);

		closedir(srcDp);
	}

mOut2:
	closedir(dstDp);
mOut1:
	nameConvClose();

	fprintf(stderr, "Processed files: %lu\n        +  dirs: %lu\n",
	    cfg.filesQty, cfg.dirsQty);

	return 0;
}

static void
usage(void)
{
	printf("Usage:\n    convln [-c] [-D dstDir] [-f code] [-t code]"
	    " [-v] <srcDir1> [<srcDir2> .. ]\n");
}

static void
help(void)
{
	usage();
	printf(
	    "Options:\n"
	    "    -c         skip invalid characters, see iconv(1).\n"
	    "    -D <dst>   where to place the results.\n"
	    "    -f <code>  the 'from' charset, i.e.: UTF-8.\n"
	    "    -h         this help.\n"
	    "    -t <code>  the 'to' charset, i.e.: KOI8-RU.\n"
	    "    -v         be verbose.\n");
}

