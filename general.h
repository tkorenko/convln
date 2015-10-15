#ifndef _general_h_
#define _general_h_

// strnncpy() copies csrings and terminates them correctly.
// - NOTE: srcLen and dstSz are quantities of different meaning:
//     srcLen - defines how many characters to copy, whereas
//     dstSz  - defines how large the destination container is.
char * strnncpy(char * dst, size_t dstSz, const char * src, size_t srcLen);

#endif /* _general_h_ */

