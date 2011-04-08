#ifndef HEADER_CRC_H
#define HEADER_CRC_H

#ifdef  __cplusplus
extern "C" {
#endif

#define CRC_LONG unsigned int // 32-bit
#define CRC_CBLOCK	16
#define CRC_LBLOCK	(CRC_CBLOCK/4)
#define CRC_DIGEST_LENGTH 4

typedef struct CRCstate_st
	{
		CRC_LONG Crc32;
	} CRC_CTX;

int CRC_Init(CRC_CTX *c);
int CRC_Update(CRC_CTX *c, const void *data, size_t len);
int CRC_Final(unsigned char *md, CRC_CTX *c);
unsigned char *CRC(const unsigned char *d, size_t n, unsigned char *md);
#ifdef  __cplusplus
}
#endif

#endif
