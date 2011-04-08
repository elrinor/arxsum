#include "tth.h"

int TTH_Init(TTH_CTX *c, unsigned long long fileSize) {
  c->BeginFile(9, fileSize);
  return 0;
}

int TTH_Update(TTH_CTX *c, const void *data, size_t len) {
  c->AddToFile((const void*) data, len);
  return 0;
}

int TTH_Final(unsigned char *md, TTH_CTX *c) {
  c->FinishFile();
  c->GetRoot(md);
  return 0;
}

unsigned char *TTH(const unsigned char *d, size_t n, unsigned char *md) {
  //
}
