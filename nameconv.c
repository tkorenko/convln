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
#include <dirent.h>
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#include "general.h"
#include "nameconv.h"

static iconv_t icd = (iconv_t)(-1);
static const char * iconvIgnoreKey = "//IGNORE";

static int
isIcdValid()
{
	if (icd == (iconv_t)(-1)) {
		fprintf(stderr, "iconv is not initialized\n");
		return 0;
	}

	return 1;
}

int
nameConvOpen()
{
	int rs = 0;
	char * locNameConvTo = NULL;
	size_t locNameConvTo_Size = 0;

	if (cfg.nameConvFrom == NULL || cfg.nameConvTo == NULL) {
		fprintf(stderr, "%s:%d: bad input: nameConvFrom(%p)"
		    " nameConvTo(%p)\n", __FILE__, __LINE__,
		    (void *)cfg.nameConvFrom, (void *)cfg.nameConvTo);

		return -1;
	}

	locNameConvTo_Size = strlen(cfg.nameConvTo)
	    + strlen(iconvIgnoreKey) + 1;
	locNameConvTo = malloc(locNameConvTo_Size);
	if (locNameConvTo == NULL) {
		fprintf(stderr, "%s:%d: malloc() failed.",
		    __FILE__, __LINE__);

		return -1;
	}

	strcpy(locNameConvTo, cfg.nameConvTo);

	if (cfg.nameConvIgnoreNonconvertable) {
		strcat(locNameConvTo, iconvIgnoreKey);
	}

#ifdef DEBUG
	fprintf(stderr, "%s:%d: I gonna feed to iconv_open '%s' and '%s'\n",
	    __FILE__, __LINE__, cfg.nameConvFrom, locNameConvTo);
#endif /* #ifdef DEBUG */

	icd = iconv_open(locNameConvTo, cfg.nameConvFrom);

	if (!isIcdValid()) {
		rs = -1;
	}

	free(locNameConvTo);

	return rs;
}

int
nameConvClose()
{
	int rs = 0;

	if (isIcdValid()) {
		iconv_close(icd);
		icd = (iconv_t)(-1);
	} else {
		rs = -1;
	}

	return rs;
}

#define LOCNAMEBUFS_SZ (MAXNAMLEN * 2 + 1)
static char locNameFromBuf[LOCNAMEBUFS_SZ];
static char locNameToBuf[LOCNAMEBUFS_SZ];

int
nameConvTranslate(const char * from, size_t fromLen,
    char * to, size_t to_Sz)
{
	size_t rs;
	char * inBuf = locNameFromBuf;
	char * outBuf = locNameToBuf;
	size_t locFromBytesLeft = fromLen;
	size_t locToBytesLeft = LOCNAMEBUFS_SZ - 1;
	size_t locToLen = 0;

	if (!isIcdValid()) {
		return -1;
	}

	// to get rid of 'from'-const-ness (in sake of iconv(3)) we have
	// to make a local copy of 'from' string:
	strnncpy(locNameFromBuf, LOCNAMEBUFS_SZ, from, fromLen);

	rs = iconv(icd,
		&inBuf,  &locFromBytesLeft,
		&outBuf, &locToBytesLeft);

	if (locFromBytesLeft != 0) { // failed to convert entire name
		fprintf(stderr, "%s:%d: iconv() failed",
		    __FILE__, __LINE__);
		if (rs == (size_t)(-1)) {
			fprintf(stderr, ": %s", strerror(errno));
		}
		fprintf(stderr, "\n");

		return -1;
	}

	locToLen = LOCNAMEBUFS_SZ - locToBytesLeft - 1;

	strnncpy(to, to_Sz, locNameToBuf, locToLen);

	return 0;
}
