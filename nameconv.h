#ifndef _nameconv_h_
#define _nameconv_h_

int nameConvOpen();
int nameConvClose();
int nameConvTranslate(const char * from, size_t fromLen,
    char * to, size_t to_Sz);

#endif /* _nameconv_h_ */
