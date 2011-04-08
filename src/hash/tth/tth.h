#ifndef __TTH_H__
#define __TTH_H__

#include "TigerTree.h"

#define TTH_DIGEST_LENGTH 24

typedef CTigerTree TTHstate_st;
typedef CTigerTree TTH_CTX;

int TTH_Init(TTH_CTX *c, unsigned long long fileSize);
int TTH_Update(TTH_CTX *c, const void *data, size_t len);
int TTH_Final(unsigned char *md, TTH_CTX *c);
unsigned char *TTH(const unsigned char *d, size_t n, unsigned char *md);

#endif