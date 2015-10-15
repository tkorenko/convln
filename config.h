#ifndef _config_h_
#define _config_h_

struct Cfg
{
	// iconv stuff
	const char *nameConvFrom;
	const char *nameConvTo;
	char        nameConvIgnoreNonconvertable; // bool type is missing
	unsigned long dirsQty;
	unsigned long filesQty;
};

extern struct Cfg cfg;

void configInit(struct Cfg * pcfg);

#endif /* _config_h_ */

