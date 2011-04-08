/*****************************************************************************
ArXSum - HashSum generating / checking utility.

Copyright (c) 2006 Alexander 'Elric' Fokin. All Rights Reserved. 
elric@gmail.ru

This product includes cryptographic software written by
Eric Young (eay@cryptsoft.com)

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written
agreement, is hereby granted, provided that the above copyright notice, 
this paragraph and the following two paragraphs appear in all copies, 
modifications, and distributions.

IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". THE AUTHOR HAS NO OBLIGATION
TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

TODO:
- ..\* and * duplicates

v1.11
+ TTH hash added

v1.10
* Fixed oef without eoln issue
* FILE_SHARE_READ added in CreateFile

v1.09
* Fixed absolute paths in .md5 files

v1.08
+ Simultaneous output into several files with different formats

v1.07
* IsValidFileName fixed - there were bugs with characters with code > 127

v1.06
+ Multithreading added

v1.05
* Changed command line options format
+ Added .ed2k file checking
+ Added .sfv file output

v1.04
* Now treating lines in .md5 files starting with '#' as comments
  Elric: this may lead to a bug in parsing .sfv files
* Fixed bug with blank lines in .md5 files
* IsValidFileName fixed
+ Added .ed2k file output

v1.03
* ed2k hashing method changed to the one used in eMule 0.46c
  see http://wiki.anidb.info/w/AniDB:Ed2k-hash for details

v1.02
+ Overlapped IO

v1.01
+ Output hashing speed
+ Now using faster IO routines (~ x2 speedup)
+ Checksum verification
+ Input filelist from file / stdin

v1.00
+ First Release - crc, md4, ed2k, md5, sha1, sha256, sha512 hashing
*****************************************************************************/

#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>
#include <ctype.h>
#include "ArXUtils.h"
#include "hash/Hashes.h"

using namespace std;

#undef max
#undef min
#define FILEBUFSIZE	(1024*1024)
#define IOBUFSIZE 16384
#define VERSION "v1.11"

enum CheckSumType
{
  CS_MD5,
  CS_CRC,
  CS_ED2K,
  CS_MD4,
  CS_SHA1,
  CS_SHA256,
  CS_SHA512,
  CS_TTH,
  CS_COUNT,
  CS_UNKNOWN = -1
};

enum OutputFormatType
{
  OF_MD5,
  OF_SFV,
  OF_ED2K,
  OF_COUNT
};

enum OutputType
{
  O_NONE,
  O_FILE,
  O_STD,
  O_SMART
};

const char* CheckSumNames[] = {"MD5", "CRC", "ED2K", "MD4", "SHA1", "SHA256", "SHA512", "TTH"};

const char* CheckSumExts[] = {"MD5", "SFV", "ED2K"};

class CheckSumOpts 
{
private:
  bool Sums[CS_COUNT];
public:
  const bool& operator[] (const int n) const {return Sums[n];}
  bool& operator[] (const int n) {return Sums[n];}
  CheckSumOpts(bool DoMD5, bool DoMD4, bool DoED2K, bool DoCRC, bool DoSHA1, bool DoSHA256, bool DoSHA512, bool DoTTH)
  {
    Sums[CS_CRC]    = DoCRC;
    Sums[CS_MD4]    = DoMD4;
    Sums[CS_ED2K]   = DoED2K;
    Sums[CS_MD5]    = DoMD5;
    Sums[CS_SHA1]   = DoSHA1;
    Sums[CS_SHA256] = DoSHA256;
    Sums[CS_SHA512] = DoSHA512;
    Sums[CS_TTH]    = DoTTH;
  }
  CheckSumOpts(bool Default)
  {
    for(int i = 0; i < CS_COUNT; i++)
      Sums[i] = Default;
  }
  CheckSumOpts(int CSId)
  {
    memset(Sums, 0, sizeof(bool) * CS_COUNT);
    Sums[CSId] = true;
  }
  CheckSumOpts()
  {
    memset(Sums, 0, sizeof(bool) * CS_COUNT);
  }
  int GetFirstCheckSum()
  {
    for(int i = 0; i < CS_COUNT; i++)
      if(Sums[i])
        return i;
    return CS_UNKNOWN;
  }
};

typedef struct ThreadParam
{
  CheckSumType SumType;
  void* ctx;
  HANDLE hBeginHash;
  HANDLE hBeginIO;
  unsigned char* ptr;
  unsigned long n;
  bool Continue;
} ThreadParam;

CheckSumOpts Do(false);
OutputType OutputFormats[OF_COUNT] = {O_NONE, O_NONE, O_NONE};
string OutputFiles[OF_COUNT];
bool FileOutput = false;
bool Recursive = false;
bool Quiet = false;
bool CheckMode = false;
bool UseFileList = false;
bool ReadFileListFromStdIn = false;
bool MultiThreaded = false;
bool TestSpeed = false;

class Entry
{
public:
  string Name;
  string Digest[CS_COUNT];
  unsigned long long Size;
  string Time;
  string Date;
  bool Ok;
  bool operator< (const Entry& e) const 
  {
    int n;
    for(n = 0; n < min(e.Name.size(), Name.size()); n++)
      if(toupper(e.Name[n]) != toupper(Name[n]))
        break;
    if(e.Name.find_first_of("\\", n) != string::npos)
    {
      if(Name.find_first_of("\\", n) != string::npos)
        return UpperCase(Name) < UpperCase(e.Name);
      else
        return false;
    }
    else
    {
      if(Name.find_first_of("\\", n) != string::npos)
        return true;
      else
        return UpperCase(Name) < UpperCase(e.Name);
    }
  }
  Entry(string FileName) : Name(FileName), Ok(true) {}
};

vector<string> Mask;
set<Entry> FileList;
string ListFile;

unsigned long long SumSize = 0, Progress;
unsigned int LastTickCount;
unsigned long long LastProgress;

unsigned int GetSectorSize(string RootPath)
{
  unsigned long SectorSize;
  const char* p = NULL;
  if(RootPath.size() > 0)
    p = RootPath.c_str();
  if(GetDiskFreeSpace(p, NULL, &SectorSize, NULL, NULL) != 0)
    return SectorSize;
  else
    return 0;
}

void failed(const char *msg) 
{
  DWORD fm;
  char *msg1, *msg2;
  const char *args[2];

  fm = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, GetLastError(), 0, (LPTSTR)&msg1, 0, NULL);
  if(fm == 0)
    ExitProcess(1);
  args[0] = msg;
  args[1] = msg1;
  fm=FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
    "%1: %2", 0, 0, (LPTSTR)&msg2, 0, (va_list*)&args[0]);
  if(fm == 0)
    ExitProcess(1);
  MessageBox(NULL, msg2, "Error", MB_OK|MB_ICONERROR);
  ExitProcess(1);
}

void MakeFileList(string Mask)
{
  WIN32_FIND_DATA FD;
  HANDLE hSearch;

  if(Recursive && (hSearch = FindFirstFile((ExtractFilePath(Mask) + "*").c_str(), &FD)) != INVALID_HANDLE_VALUE)
    do{
      if((FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && 
        strcmp(FD.cFileName, ".") != 0 && strcmp(FD.cFileName, "..") != 0)
          MakeFileList(ExtractFilePath(Mask) + FD.cFileName + string("\\") + ExtractFileName(Mask));
    }while(FindNextFile(hSearch, &FD) != 0);

  if((hSearch = FindFirstFile(Mask.c_str(), &FD)) == INVALID_HANDLE_VALUE)
    return;
  do{
    if((FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
      FileList.insert(Entry(ExtractFilePath(Mask) + FD.cFileName));
  }while(FindNextFile(hSearch, &FD) != 0);
  FindClose(hSearch);
  return;
}

DWORD WINAPI HashThreadFunc(LPVOID lpParam)
{
  ThreadParam* Param = (ThreadParam*) lpParam;
  while(true)
  {
    WaitForSingleObject(Param->hBeginHash, INFINITE);
    if(!Param->Continue)
      return 0;
    switch(Param->SumType) 
    {
    case CS_CRC:       CRC_Update((CRC_CTX*)    Param->ctx, Param->ptr, Param->n); break;
    case CS_MD4:       MD4_Update((MD4_CTX*)    Param->ctx, Param->ptr, Param->n); break;
    case CS_ED2K:     ED2K_Update((ED2K_CTX*)   Param->ctx, Param->ptr, Param->n); break;
    case CS_MD5:       MD5_Update((MD5_CTX*)    Param->ctx, Param->ptr, Param->n); break;
    case CS_SHA1:     SHA1_Update((SHA_CTX*)    Param->ctx, Param->ptr, Param->n); break;
    case CS_SHA256: SHA256_Update((SHA256_CTX*) Param->ctx, Param->ptr, Param->n); break;
    case CS_SHA512: SHA512_Update((SHA512_CTX*) Param->ctx, Param->ptr, Param->n); break;
    case CS_TTH:       TTH_Update((TTH_CTX*)    Param->ctx, Param->ptr, Param->n); break;
    }
    SetEvent(Param->hBeginIO);
  }
  return 0;
}

void TestHashSpeed() 
{
  CRC_CTX crcctx;
  MD4_CTX md4ctx; 
  ED2K_CTX ed2kctx;
  MD5_CTX md5ctx;
  SHA_CTX sha1ctx;
  SHA256_CTX sha256ctx;
  SHA512_CTX sha512ctx;
  TTH_CTX tthctx;
  CRC_Init(&crcctx);
  MD4_Init(&md4ctx);
  ED2K_Init(&ed2kctx);
  MD5_Init(&md5ctx);
  SHA1_Init(&sha1ctx);
  SHA256_Init(&sha256ctx);
  SHA512_Init(&sha512ctx);
  TTH_Init(&tthctx, 1024 * FILEBUFSIZE);

  unsigned char* buf = new unsigned char[FILEBUFSIZE];
  unsigned int LastTickCount;
  unsigned int i;
  cout << "Testing hash speed..." << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    CRC_Update(&crcctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_CRC] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    MD4_Update(&md4ctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_MD4] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    ED2K_Update(&ed2kctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_ED2K] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    MD5_Update(&md5ctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_MD5] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    SHA1_Update(&sha1ctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_SHA1] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    SHA256_Update(&sha256ctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_SHA256] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    SHA512_Update(&sha512ctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_SHA512] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  LastTickCount = GetTickCount();
  for(i = 0; i < 32 || (GetTickCount() - LastTickCount) < 3723; i++)
    TTH_Update(&tthctx,    buf, FILEBUFSIZE);
  cout << CheckSumNames[CS_TTH] << ": " << fixed << showpoint << setprecision(3) << (((float)i) * FILEBUFSIZE / (1024 * 1024)) / ((GetTickCount() - LastTickCount) / 1000.0f) << "Mbps       " << endl;

  delete[] buf;
}

void HashFile(Entry& File, bool Quiet, bool MultiThreaded, const CheckSumOpts& Do)
{
  bool StdInput = (File.Name == "-");
  bool SelfProgressOnly = false;
  if(!Quiet && SumSize == 0)
  {
    SelfProgressOnly = true;
    SumSize = GetFileSize(File.Name);
    if(SumSize == 0)
      Quiet = true;
    else
    {
      Progress = 0;
      LastTickCount = GetTickCount();
      LastProgress = Progress;
    }
  }

  if(MultiThreaded && File.Size <= FILEBUFSIZE)
    MultiThreaded = false;

  HANDLE f;
  if(!StdInput)
    f = CreateFile(File.Name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
      FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OVERLAPPED, NULL);
  else
    f = GetStdHandle(STD_INPUT_HANDLE);
  if(f == INVALID_HANDLE_VALUE)
  {
    File.Ok = false;
    if(!Quiet)
      Progress += File.Size;
    return;
  }

  CRC_CTX crcctx;
  MD4_CTX md4ctx; 
  ED2K_CTX ed2kctx;
  MD5_CTX md5ctx;
  SHA_CTX sha1ctx;
  SHA256_CTX sha256ctx;
  SHA512_CTX sha512ctx;
  TTH_CTX tthctx;
  unsigned char crcdgst[CRC_DIGEST_LENGTH]; 
  unsigned char md4dgst[MD4_DIGEST_LENGTH]; 
  unsigned char ed2kdgst[ED2K_DIGEST_LENGTH]; 
  unsigned char md5dgst[MD5_DIGEST_LENGTH];
  unsigned char sha1dgst[SHA_DIGEST_LENGTH]; 
  unsigned char sha256dgst[SHA256_DIGEST_LENGTH]; 
  unsigned char sha512dgst[SHA512_DIGEST_LENGTH]; 
  unsigned char tthdgst[TTH_DIGEST_LENGTH]; 

  static unsigned char buf0[FILEBUFSIZE];
  static unsigned char buf1[FILEBUFSIZE];
  unsigned char* ptr = buf0;
  unsigned char* ptrn;
  // Elric [1/14/2007]: It seems unbuffered reading works without data alignment o_O
  //unsigned int SectorSize = GetSectorSize(ExtractFileDrive(File.Name));
  //char* ptr = (char*)(((unsigned int) buf + SectorSize - 1) & ~(SectorSize - 1));
  //unsigned int BufSize = (BUFSIZE - ((unsigned int) ptr - (unsigned int) buf)) & ~(SectorSize - 1));
  //unsigned char* ptr = buf;
  //unsigned int BufSize = BUFSIZE;

  unsigned long long Offset = 0;
  bool Parity = false;
  OVERLAPPED ovr;
  memset(&ovr, 0, sizeof(ovr));

  CRC_Init(&crcctx);
  MD4_Init(&md4ctx);
  ED2K_Init(&ed2kctx);
  MD5_Init(&md5ctx);
  SHA1_Init(&sha1ctx);
  SHA256_Init(&sha256ctx);
  SHA512_Init(&sha512ctx);
  TTH_Init(&tthctx, File.Size);

  ThreadParam Params[CS_COUNT];
  HANDLE hThreads[CS_COUNT];
  if(MultiThreaded)
  {
    Params[CS_CRC].ctx = &crcctx;
    Params[CS_MD4].ctx = &md4ctx;
    Params[CS_ED2K].ctx = &ed2kctx;
    Params[CS_MD5].ctx = &md5ctx;
    Params[CS_SHA1].ctx = &sha1ctx;
    Params[CS_SHA256].ctx = &sha256ctx;
    Params[CS_SHA512].ctx = &sha512ctx;
    Params[CS_TTH].ctx = &tthctx;
    for(int i = 0; i < CS_COUNT; i++) if(Do[i])
    {
      Params[i].SumType = (CheckSumType) i;
      Params[i].hBeginIO = CreateEvent(NULL, FALSE, FALSE, NULL);
      Params[i].hBeginHash = CreateEvent(NULL, FALSE, FALSE, NULL);
      Params[i].Continue = true;
      Params[i].n = 0;
      Params[i].ptr = ptr;
      hThreads[i] = CreateThread(NULL, 0, HashThreadFunc, (LPVOID) &Params[i], 0, NULL);
    }
  }

  unsigned long read = 0;
  float Speed = 0.0f;
  while(true)
  {
    ptrn = Parity ? buf0 : buf1;
    ovr.Offset = (unsigned int) Offset;
    ovr.OffsetHigh = (unsigned int) (Offset >> 32);
    unsigned int bResult;
    if(!StdInput)
      bResult = ReadFile(f, ptrn, FILEBUFSIZE, NULL, &ovr);
    else
      bResult = ReadFile(f, ptrn, 1024, &read, NULL);
    unsigned int LastError = GetLastError();
    if(read != 0)
    {
      if(MultiThreaded)
      {
        for(int i = 0; i < CS_COUNT; i++) if(Do[i])        
        {
          Params[i].ptr = ptr;
          Params[i].n = read;
          SetEvent(Params[i].hBeginHash);
        }
        for(int i = 0; i < CS_COUNT; i++) if(Do[i])
          WaitForSingleObject(Params[i].hBeginIO, INFINITE);
      }
      else
      {
        if(Do[CS_CRC])       CRC_Update(&crcctx,    ptr, read);
        if(Do[CS_MD4])       MD4_Update(&md4ctx,    ptr, read);
        if(Do[CS_ED2K])     ED2K_Update(&ed2kctx,   ptr, read);
        if(Do[CS_MD5])       MD5_Update(&md5ctx,    ptr, read);
        if(Do[CS_SHA1])     SHA1_Update(&sha1ctx,   ptr, read);
        if(Do[CS_SHA256]) SHA256_Update(&sha256ctx, ptr, read);
        if(Do[CS_SHA512]) SHA512_Update(&sha512ctx, ptr, read);
        if(Do[CS_TTH])       TTH_Update(&tthctx,    ptr, read);
      }
    }
    if(!bResult && LastError != ERROR_IO_PENDING)
      break;
    if(!StdInput)
      GetOverlappedResult(f, &ovr, &read, TRUE);
    if (read <= 0) break;
    ptr = ptrn;
    Parity =! Parity;
    Offset += read;
    if (!Quiet)
    {
      unsigned int TickCount = GetTickCount();
      if (TickCount - LastTickCount > 1000)
      {
        Speed = (Progress - LastProgress) * 1000.0f / (TickCount - LastTickCount);
        LastTickCount = TickCount;
        LastProgress = Progress;
      }
      Progress += read;
      cout << "\rProgress: " << setw(3) << 100 * Progress / SumSize << "%";
      if(Speed != 0.0f)
        cout << "   Speed: " << fixed << showpoint << setprecision(3) << (float) Speed / (1024 * 1024) << "Mbps    ";
    }
  }
  CloseHandle(f);

  CRC_Final(crcdgst, &crcctx);
  MD4_Final(md4dgst, &md4ctx);
  ED2K_Final(ed2kdgst, &ed2kctx);
  MD5_Final(md5dgst, &md5ctx);
  SHA1_Final(sha1dgst, &sha1ctx);
  SHA256_Final(sha256dgst, &sha256ctx);
  SHA512_Final(sha512dgst, &sha512ctx);
  TTH_Final(tthdgst, &tthctx);

  File.Digest[CS_CRC]    = HexDump(crcdgst, CRC_DIGEST_LENGTH);
  File.Digest[CS_MD4]    = HexDump(md4dgst, MD4_DIGEST_LENGTH);
  File.Digest[CS_ED2K]   = HexDump(ed2kdgst, ED2K_DIGEST_LENGTH);
  File.Digest[CS_MD5]    = HexDump(md5dgst, MD5_DIGEST_LENGTH);
  File.Digest[CS_SHA1]   = HexDump(sha1dgst, SHA_DIGEST_LENGTH);
  File.Digest[CS_SHA256] = HexDump(sha256dgst, SHA256_DIGEST_LENGTH);
  File.Digest[CS_SHA512] = HexDump(sha512dgst, SHA512_DIGEST_LENGTH);
  File.Digest[CS_TTH]    = HexDump(tthdgst, TTH_DIGEST_LENGTH);

  if(MultiThreaded)
  {
    for(int i = 0; i < CS_COUNT; i++) if(Do[i])
    {
      Params[i].Continue = false;
      SetEvent(Params[i].hBeginHash);
      WaitForSingleObject(hThreads[i], INFINITE);
      CloseHandle(Params[i].hBeginHash);
      CloseHandle(Params[i].hBeginIO);
    }
  }

  if(SelfProgressOnly)
    SumSize = 0;

  return;
}

bool IsValidFileName(string Name)
{
  if(Name.size() == 0)
    return false;
  if(Name[Name.size()] == '/' || Name[Name.size()] == '\\')
    return false;
  int ColonN = 0;
  for(int i = 0; i < Name.size(); i++) 
  {
    /* TODO: This doesn't work =(
    if(!isprint(Name[i]))
      return false;
    */
    if(strchr("*?\"<>|", Name[i]) != NULL)
      return false;
    if(Name[i] == ':')
    {
      ColonN++;
      if(ColonN > 1)
        return false;
    }
  }
  if(ColonN == 1 && (Name.size() < 2 || (Name.size() >= 2 && Name[1] != ':')))
    return false;
  return true;
}

bool IsAbsoluteFileName(string Name)
{
  if(Name[1] == ':' && (Name[2] == '\\' || Name[2] == '/'))
    return true;
  else
    return false;
}

bool CheckSumFile(string Name)
{
  bool result = true;
  unsigned long long FileSize = GetFileSize(Name);
  unsigned long long Read = 0;
  bool FileNeedsDelete = (Name != "-");
  istream* f;

  cout << "  " << Name << ":";
  if(Name != "-")
  {
    f = new ifstream;
    ((ifstream*)f)->open(Name.c_str());
    if(!((ifstream*)f)->is_open())
    {
      cout << "Could not open for reading." << endl << endl;
      return false;
    }
  }
  else
    f = &cin;
    
  cout << endl;

  unsigned int Errors = 0;
  unsigned int Successes = 0;
  unsigned int LineN = 0;
  char buf[IOBUFSIZE];
  while(true)
  {
    if(f->eof() || f->fail())
      break;
    f->getline(buf, IOBUFSIZE);
    LineN++;
    Read += strlen(buf) + 2;
    if(buf[0] == ';' || buf[0] == '#') // skip comments   TODO: #???
      continue;
    string s(buf);
    s = Trim(s);
    if(s == "") // skip blanks
      continue;

    CheckSumType SumType = CS_UNKNOWN;
    string FileName[CS_COUNT], Digest[CS_COUNT];
    if(s.size() > 7 && s.substr(0, 7) == "ed2k://")
    {
      //ed2k://|file|Amaenaide yo!! Katsu!! - 01 =Mendoi=.avi|244576256|bd4bffffc7664e11e85485383c984507|/
      int NameEndPos = s.find("|", 13);
      FileName[CS_ED2K] = s.substr(13, NameEndPos - 13);
      Digest[CS_ED2K] = s.substr(s.rfind("|", s.size() - 3) + 1, 32);
      if(IsValidFileName(FileName[CS_ED2K]) && IsHex(Digest[CS_ED2K]))
        SumType = CS_ED2K;
    }
    else
    {
      if(s.size() >= 10)
      {
        Digest[CS_CRC] = s.substr(s.size() - 8);
        int i;
        for(i = s.size() - 9; i >= 0; i--)
          if(strchr(" \t*", s[i]) == NULL)
            break;
        FileName[CS_CRC] = Trim(s.substr(0, i + 1));
        if(IsValidFileName(FileName[CS_CRC]) && IsHex(Digest[CS_CRC]))
          SumType = CS_CRC;
      }
      if(s.size() >= 34)
      {
        Digest[CS_MD5] = s.substr(0, 32);
        int i;
        for(i = 33; i < s.size(); i++)
          if(strchr(" \t*", s[i]) == NULL)
            break;
        FileName[CS_MD5] = Trim(s.substr(i));
        if(IsValidFileName(FileName[CS_MD5]) && IsHex(Digest[CS_MD5]))
        {
          if(SumType == CS_CRC)
          {
            if(UpperCase(ExtractFileExt(Name)) == "MD5")
              SumType = CS_MD5;
            else if(UpperCase(ExtractFileExt(Name)) == "SFV")
              SumType = CS_CRC;
            else
            {
              cout << "Ambiguity (line " << LineN << ")" << endl;
              result = false;
              continue;
            }
          }
          else
            SumType = CS_MD5;
        }
      }
    }

    if(SumType == -1)
    {
      cout << "Unreadable file format (line " << LineN << ")" << endl << endl;
      return false;
    }

    Entry File(((IsAbsoluteFileName(FileName[SumType]))?"":ExtractFilePath(Name)) + FileName[SumType]);
    string out = FileName[SumType];
    HashFile(File, Quiet, MultiThreaded, CheckSumOpts(SumType));
    if(!File.Ok)
    {
      out = "\rCould not open file: " + out;
      Errors++;
    }
    else if(UpperCase(Digest[SumType]) == UpperCase(File.Digest[SumType]))
    {
      out = "\rOk: " + out;
      Successes++;
    }
    else
    {
      out = "\rWrong " + string(CheckSumNames[SumType]) + ": " + out;
      Errors++;
    }
    cout << out;
    if(out.size() < 80)
      cout << string(80 - out.size() - 1, ' '); // Clean progress
    cout << endl;
  }
  if(FileNeedsDelete)
    ((ifstream*)f)->close();

  if(Read < FileSize || (Errors == 0 && Successes == 0))
    cout << "Unreadable file format (line " << LineN << ")" << endl << endl;
  else
    cout << "  Errors: " << Errors << endl << "  Ok: " << Successes << endl << endl;

  return (Errors == 0);
}

void Synopsis()
{
  cout << "arxsum - ArX Checksum Calculator " << VERSION << endl;
  cout << "" << endl;
  cout << "USAGE:" << endl;
  cout << "arxsum [filemasks] [options]" << endl;
  cout << endl;
  cout << "Checksum options:" << endl;
  cout << "-crc            Calculate crc" << endl;
  cout << "-md4            Calculate md4" << endl;
  cout << "-ed2k           Calculate ed2k hash" << endl;
  cout << "-md5            Calculate md5" << endl;
  cout << "-sha1           Calculate sha1" << endl;
  cout << "-sha256         Calculate sha256" << endl;
  cout << "-sha512         Calculate sha512" << endl;
  cout << "-tth            Calculate tth" << endl;
  cout << "-all            Calculate all possible checksums" << endl;
  cout << endl;
  cout << "Output options:" << endl;
  cout << "-omd5[=FILE]    Output in .md5 format (adds -md5)" << endl;
  cout << "-osfv[=FILE]    Output in .sfv format (adds -crc)" << endl;
  cout << "-oed2k[=FILE]   Output in .ed2k format (adds -ed2k)" << endl;
  cout << "  If FILE is not specified, then output to stdout. If FILE is set to '*'," << endl;
  cout << "  then output to file with name set according to filenames processed." << endl;
  cout << "  If no option is provided, then -omd5 is used." << endl;
  cout << endl;
  cout << "Input options:" << endl;
  cout << "-l[=FILE]       Read file masks from FILE. If FILE is not specified, then from" << endl;
  cout << "                stdin" << endl;
  cout << "  If no filemask is provided, then checksum of stdin is calculated." << endl;
  cout << endl;
  cout << "Other options:" << endl;
  cout << "-r              Recursive" << endl;
  cout << "-m              Multithreaded" << endl;
  cout << "-c              Check mode - treat all input files as checksum files" << endl;
  cout << "-q              Do not output progress" << endl;
  cout << "-h, --help      Display this help" << endl;
  cout << "-t, --test      Do not hash anything, just test hash speed" << endl;
  return;
}

int main(int argc, char **argv)
{
  for(int i = 1; i < argc; i++)
  {
    string arg(argv[i]);
    if(arg[0]=='-')
    {
      if(arg == "-h" || arg == "--help")
      {
        Synopsis();
        exit(0);
      }
      else if(arg == "-t" || arg == "--test")
        TestSpeed = true;
      else if(arg == "-r")
        Recursive = true;
      else if(arg == "-m")
        MultiThreaded = true;
      else if(arg == "-c")
        CheckMode = true;
      else if(arg == "-q")
        Quiet = true;
      else if(arg == "-all")
        Do = CheckSumOpts(true);
      else if(arg == "-crc")
        Do[CS_CRC] = true;
      else if(arg == "-md4")
        Do[CS_MD4] = true;
      else if(arg == "-ed2k")
        Do[CS_ED2K] = true;
      else if(arg == "-md5")
        Do[CS_MD5] = true;
      else if(arg == "-sha1")
        Do[CS_SHA1] = true;
      else if(arg == "-sha256")
        Do[CS_SHA256] = true;
      else if(arg == "-sha512")
        Do[CS_SHA512] = true;
      else if(arg == "-tth")
        Do[CS_TTH] = true;
      else if(arg.substr(0, 5) == "-omd5")
      {
        if(arg.size() > 6 && arg[5] == '=')
        {
          if(arg.substr(6) == "*")
            OutputFormats[OF_MD5] = O_SMART;
          else
          {
            OutputFormats[OF_MD5] = O_FILE;
            OutputFiles[OF_MD5] = arg.substr(6);
          }
        }
        else
          OutputFormats[OF_MD5] = O_STD;
      }
      else if(arg.substr(0, 5) == "-osfv")
      {
        if(arg.size() > 6 && arg[5] == '=')
        {
          if(arg.substr(6) == "*")
            OutputFormats[OF_SFV] = O_SMART;
          else
          {
            OutputFormats[OF_SFV] = O_FILE;
            OutputFiles[OF_SFV] = arg.substr(6);
          }
        }
        else
          OutputFormats[OF_SFV] = O_STD;
      }
      else if(arg.substr(0, 6) == "-oed2k")
      {
        if(arg.size() > 7 && arg[6] == '=')
        {
          if(arg.substr(7) == "*")
            OutputFormats[OF_ED2K] = O_SMART;
          else
          {
            OutputFormats[OF_ED2K] = O_FILE;
            OutputFiles[OF_ED2K] = arg.substr(7);
          }
        }
        else
          OutputFormats[OF_ED2K] = O_STD;
      }
      else if(arg.substr(0, 2) == "-l")
      {
        UseFileList = true;
        if(arg.size() > 3 && arg[2] == '=')
          ListFile = arg.substr(3);
        else
          ReadFileListFromStdIn = true;
      }
    }
    else
      Mask.push_back(argv[i]);
  }

  if(TestSpeed) {
    TestHashSpeed();
    exit(0);
  }

  if(OutputFormats[OF_MD5] == O_NONE && OutputFormats[OF_SFV] == O_NONE && OutputFormats[OF_ED2K] == O_NONE)
    OutputFormats[OF_MD5] = O_STD;
  if((OutputFormats[OF_MD5] == O_STD)?0:1 + (OutputFormats[OF_SFV] == O_STD)?0:1 + (OutputFormats[OF_ED2K] == O_STD)?0:1 > 1)
  {
    cerr << "ERROR: Cannot output to stdout in several different formats." << endl;
    exit(1);
  }

  if(OutputFormats[OF_MD5] != O_NONE)
    Do[CS_MD5] = true;
  if(OutputFormats[OF_SFV] != O_NONE)
    Do[CS_CRC] = true;
  if(OutputFormats[OF_ED2K] != O_NONE)
    Do[CS_ED2K] = true;

  while(UseFileList)
  {
    istream* f;
    if(ReadFileListFromStdIn)
      f = &cin;
    else
    {
      f = new ifstream;
      ((ifstream*)f)->open(ListFile.c_str());
      if(!((ifstream*)f)->is_open())
      {
        delete f;
        break;
      }
    }
    char buf[IOBUFSIZE];
    while(true)
    {
      f->getline(buf, IOBUFSIZE);
      if(f->eof() || f->fail())
        break;
      Mask.push_back(buf);
    }
    if(!ReadFileListFromStdIn)
    {
      ((ifstream*)f)->close();
      delete f;
    }
    break;
  }

  for(vector<string>::iterator i = Mask.begin(); i != Mask.end(); i++)
    MakeFileList(*i);

  if(FileList.size() == 0) // Hash stdin
    FileList.insert(Entry("-"));

  if(CheckMode)
  {
    for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++)
      CheckSumFile(i->Name);
    cout << endl;
  }
  else
  {
    SumSize = Progress = 0;
    LastTickCount = GetTickCount();
    LastProgress = Progress;

    for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++)
    {
      i->Date = GetFileModifyDate(i->Name);
      i->Time = GetFileModifyTime(i->Name);
      SumSize += (i->Size = GetFileSize(i->Name));
    }

    for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++)
      HashFile(*i, Quiet, MultiThreaded, Do);

    for(int OutFmt = 0; OutFmt < OF_COUNT; OutFmt++)
    {
      if(OutputFormats[OutFmt] == O_NONE)
        continue;
      ostream* out = &cout;
      if(OutputFormats[OutFmt] == O_SMART || OutputFormats[OutFmt] == O_FILE)
      {
        if(OutputFormats[OutFmt] == O_SMART)
        {
          OutputFiles[OutFmt] = FileList.begin()->Name;
          for(set<Entry>::iterator i = ++FileList.begin(); i != FileList.end(); i++)
          {
            int j;
            for(j = 0; j < OutputFiles[OutFmt].size(); j++)
              if(i->Name[j] != OutputFiles[OutFmt][j])
                break;
            OutputFiles[OutFmt].erase(j);
            if(OutputFiles[OutFmt].empty())
              break;
          }
          OutputFiles[OutFmt] = Trim(OutputFiles[OutFmt]);
          if(OutputFiles[OutFmt].empty())
            OutputFiles[OutFmt] = "sum";
          OutputFiles[OutFmt] += "." + LowerCase(CheckSumExts[OutFmt]);
        }

        ofstream* f = new ofstream;
        f->open(OutputFiles[OutFmt].c_str());
        if(f->is_open())
          out = f;
        else
          delete f;
      }

      if(OutFmt == OF_ED2K)
      {
        //ed2k://|file|Amaenaide yo!! Katsu!! - 01 =Mendoi=.avi|244576256|bd4bffffc7664e11e85485383c984507|/
        for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
          *out << "ed2k://|file|" << ExtractFileName(i->Name) << "|" << i->Size << "|" << i->Digest[CS_ED2K] << "|/" << endl;
      }
      else
      {
        *out << "; Generated by ArXSum " << VERSION << " on " << GetDate() << " at " << GetTime() << endl;
        *out << "; ArXSum includes cryptographic software written by Eric Young (eay@cryptsoft.com)" << endl;
        *out << "; (c) Alexander 'Elric' Fokin, [ArX] Team, 2007" << endl;
        *out << ";" << endl;
        for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
          *out << "; " << setw(12) << i->Size << "  " << i->Time << " " << i->Date << " " << i->Name << endl;
        if(OutFmt == OF_SFV)
        {
          for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
            *out << i->Name << " " << i->Digest[CS_CRC] << endl;
        }
        else
        {
          for(int n = 0; n < CS_COUNT; n++) if(Do[n] && n != CS_MD5 && ((n < OF_COUNT)?(OutputFormats[n] == O_NONE):true))
          {
            *out << ";" << endl << "; * " << CheckSumNames[n] << " Block *" << endl;
            for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
              *out << "; " << i->Digest[n] << " *" << i->Name << endl;
          }
          for(set<Entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
            *out << i->Digest[CS_MD5] << " *" << i->Name << endl;
        }
      }

      if(out != &cout)
      {
        ((ofstream*) out)->close();
        delete out;
      }
    }
  }

  return 0;
}