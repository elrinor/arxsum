#ifndef HEADER_ED2K_H
#define HEADER_ED2K_H

#include "../md4/md4.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define ED2K_LONG unsigned int
#define ED2K_CBLOCK	64
#define ED2K_LBLOCK	(ED2K_CBLOCK/4)
#define ED2K_DIGEST_LENGTH 16

#define ED2K_BORDER 9728000

typedef struct ED2Kstate_st
	{
		MD4_CTX mainMD4, finMD4;
		long long pos, border;
	} ED2K_CTX;

int ED2K_Init(ED2K_CTX *c);
int ED2K_Update(ED2K_CTX *c, const void *data, size_t len);
int ED2K_Final(unsigned char *md, ED2K_CTX *c);
unsigned char *ED2K(const unsigned char *d, size_t n, unsigned char *md);
#ifdef  __cplusplus
}
#endif

#endif
