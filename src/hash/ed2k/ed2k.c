#include <stdlib.h>
#include "ed2k.h"

int ED2K_Init(ED2K_CTX *c)
{
	MD4_Init(&c->mainMD4);
	MD4_Init(&c->finMD4);
	c->pos = 0;
	c->border = ED2K_BORDER;
	return 1;
}

int ED2K_Update(ED2K_CTX *c, const void *data, size_t len)
{
	if(len + c->pos < c->border)
	{
		MD4_Update(&c->mainMD4, data, len);
		c->pos += len;
	}
	else
	{
		unsigned int newlen = c->border - c->pos;
		unsigned char md4dgst[MD4_DIGEST_LENGTH];
		MD4_Update(&c->mainMD4, data, newlen);
		MD4_Final(&(md4dgst[0]), &c->mainMD4);
		MD4_Init(&c->mainMD4);
		c->pos = c->border;
		c->border += ED2K_BORDER;
	
		MD4_Update(&c->finMD4, &(md4dgst[0]), MD4_DIGEST_LENGTH);
		if(newlen < len)
			ED2K_Update(c, &(((const unsigned char*)data)[newlen]), len - newlen);
	}
	return 1;
}

int ED2K_Final(unsigned char *md, ED2K_CTX *c)
{
  // Elric [1/14/2007]:
  // I've just found out that there are 2 different ways of dealing with files 
  // which size is the multiple of 9728000.
  // I've switched to the second one, which is used in eMule:
  // If a filesize is a multiple of 9728000, append a md4 of null to hashlist, 
  // and then hash it.
  //
  // File of zeros with size 9728000:
  // 1st method: d7def262a127cd79096a108e7a9fc138
  // 2nd method: fc21d9af828f92a8df64beac3357425d
  //
  // see http://wiki.anidb.info/w/AniDB:Ed2k-hash for details
	if(c->pos < ED2K_BORDER)
		MD4_Final(md, &c->mainMD4);
	else if(c->border - c->pos == ED2K_BORDER)
  {
    static unsigned char md4dgst[MD4_DIGEST_LENGTH] = {0x31,0xd6,0xcf,0xe0,0xd1,0x6a,0xe9,0x31,0xb7,0x3c,0x59,0xd7,0xe0,0xc0,0x89,0xc0};
    MD4_Update(&c->finMD4, &(md4dgst[0]), MD4_DIGEST_LENGTH);
    MD4_Final(md, &c->finMD4);
  }
	else
	{
		unsigned char md4dgst[MD4_DIGEST_LENGTH];
		MD4_Final(&(md4dgst[0]), &c->mainMD4);
		MD4_Update(&c->finMD4, &(md4dgst[0]), MD4_DIGEST_LENGTH);
		MD4_Final(md, &c->finMD4);
	}
	return 1;
}

/*
// OLD CODE
int ED2K_Final(unsigned char *md, ED2K_CTX *c)
{
  if(c->pos <= ED2K_BORDER)
    MD4_Final(md, &c->mainMD4);
  else if(c->border - c->pos == ED2K_BORDER)
    MD4_Final(md, &c->finMD4);
  else
  {
    unsigned char md4dgst[MD4_DIGEST_LENGTH];
    MD4_Final(&(md4dgst[0]), &c->mainMD4);
    MD4_Update(&c->finMD4, &(md4dgst[0]), MD4_DIGEST_LENGTH);
    MD4_Final(md, &c->finMD4);
  }
  return 1;
}
*/


unsigned char *ED2K(const unsigned char *d, size_t n, unsigned char *md)
{
	ED2K_CTX c;
	static unsigned char m[ED2K_DIGEST_LENGTH];

	if (md == NULL) md=m;
	if (!ED2K_Init(&c))
		return NULL;
	ED2K_Update(&c,d,n); 
	ED2K_Final(md,&c);
	return(md);
}
