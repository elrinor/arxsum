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

v1.01
+ Output hash speed
+ Now using faster IO routines (~ x2 speedup)
+ Checksum verification
+ Input filelist from file / stdin

v1.00
* First Release - crc, md4, ed2k, md5, sha1, sha256, sha512 hashing
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
#include "openssl/md5/md5.h"
#include "openssl/md4/md4.h"
#include "openssl/ed2k/ed2k.h"
#include "openssl/sha/sha.h"
#include "openssl/crc/crc.h"

using namespace std;

#undef max
#undef min
#define BUFSIZE	1024*1024

typedef struct CheckSumOpts 
{
  bool DoMD5;
  bool DoMD4; 
  bool DoED2K; 
  bool DoCRC; 
  bool DoSHA1; 
  bool DoSHA256;
  bool DoSHA512;
  CheckSumOpts(bool DoMD5, bool DoMD4, bool DoED2K, bool DoCRC, bool DoSHA1, bool DoSHA256, bool DoSHA512) : 
  DoMD5(DoMD5), DoMD4(DoMD4), DoED2K(DoED2K), DoCRC(DoCRC), DoSHA1(DoSHA1), DoSHA256(DoSHA256), DoSHA512(DoSHA512) {};
} CheckSumOpts;

CheckSumOpts Sums(false, false, false, false, false, false, false);
bool SmartOutput = false;
bool FileOutput = false;
bool Recursive = false;
bool NoProgress = false;
bool CheckAll = false;
bool HashAll = false;
bool UseStdIn = false;
bool UseFileIn = false;

typedef struct entry
{
  string Name;
  string CRC;
  string MD4;
  string ED2K;
  string MD5;
  string SHA1;
  string SHA256;
  string SHA512;
  unsigned long long Size;
  string Time;
  string Date;
  bool Ok;
  bool operator< (const entry& e) const 
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
} entry;

entry MakeEntry(string FileName)
{
  entry result;
  result.Name = FileName;
  result.Ok = true;
  return result;
}

vector<string> Mask;
set<entry> FileList;
string OutFile;
string InFile;

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

void MakeFileList(string Mask)
{
  WIN32_FIND_DATA FD;
  HANDLE hSearch;

  if(Recursive && (hSearch = FindFirstFile((ExtractFileDir(Mask) + "*").c_str(), &FD)) != INVALID_HANDLE_VALUE)
    do{
      if((FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && 
        strcmp(FD.cFileName, ".") != 0 && strcmp(FD.cFileName, "..") != 0)
          MakeFileList(ExtractFileDir(Mask) + FD.cFileName + string("\\") + ExtractFileName(Mask));
    }while(FindNextFile(hSearch, &FD) != 0);

  if((hSearch = FindFirstFile(Mask.c_str(), &FD)) == INVALID_HANDLE_VALUE)
    return;
  do{
    if((FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
      FileList.insert(MakeEntry(ExtractFileDir(Mask) + FD.cFileName));
  }while(FindNextFile(hSearch, &FD) != 0);
  FindClose(hSearch);
  return;
}

void HashFile(entry& File, bool NoProgress, const CheckSumOpts& Sums)
{
  bool SelfProgressOnly = false;
  if(!NoProgress && SumSize == 0)
  {
    SelfProgressOnly = true;
    SumSize = GetFileSize(File.Name);
    if(SumSize == 0)
      NoProgress = true;
    else
    {
      Progress = 0;
      LastTickCount = GetTickCount();
      LastProgress = Progress;
    }
  }

  HANDLE f = CreateFile(File.Name.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 
    FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN/* | FILE_FLAG_OVERLAPPED*/, NULL);
  if (f == INVALID_HANDLE_VALUE)
  {
    File.Ok = false;
    if(!NoProgress)
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
  unsigned char crcdgst[CRC_DIGEST_LENGTH]; 
  unsigned char md4dgst[MD4_DIGEST_LENGTH]; 
  unsigned char ed2kdgst[ED2K_DIGEST_LENGTH]; 
  unsigned char md5dgst[MD5_DIGEST_LENGTH];
  unsigned char sha1dgst[SHA_DIGEST_LENGTH]; 
  unsigned char sha256dgst[SHA256_DIGEST_LENGTH]; 
  unsigned char sha512dgst[SHA512_DIGEST_LENGTH]; 

  //unsigned int SectorSize = GetSectorSize(ExtractFileDrive(File.Name));
  static unsigned char buf[BUFSIZE];
  //char* ptr = (char*)(((unsigned int) buf + SectorSize - 1) & ~(SectorSize - 1));
  //unsigned int BufSize = (BUFSIZE - ((unsigned int) ptr - (unsigned int) buf)) & ~(SectorSize - 1));
  //unsigned char* ptr = buf;
  //unsigned int BufSize = BUFSIZE;

  CRC_Init(&crcctx);
  MD4_Init(&md4ctx);
  ED2K_Init(&ed2kctx);
  MD5_Init(&md5ctx);
  SHA1_Init(&sha1ctx);
  SHA256_Init(&sha256ctx);
  SHA512_Init(&sha512ctx);

  unsigned long n;
  float Speed = 0.0f;
  while(true)
  {
    ReadFile(f, buf, BUFSIZE, &n, NULL);
    if (n <= 0) break;
    if (!NoProgress)
    {
      unsigned int TickCount = GetTickCount();
      if (TickCount - LastTickCount > 1000)
      {
        Speed = (Progress - LastProgress) * 1000.0f / (TickCount - LastTickCount);
        LastTickCount = TickCount;
        LastProgress = Progress;
      }
      Progress += n;
      cout << "\rProgress: " << setw(3) << 100 * Progress / SumSize << "%";
      if(Speed != 0.0f)
        cout << "   Speed: " << fixed << showpoint << setprecision(3) << (float) Speed / (1024 * 1024) << "Mbps    ";
    }
    if(Sums.DoCRC)       CRC_Update(&crcctx,    buf, n);
    if(Sums.DoMD4)       MD4_Update(&md4ctx,    buf, n); 
    if(Sums.DoED2K)     ED2K_Update(&ed2kctx,   buf, n);
    if(Sums.DoMD5)       MD5_Update(&md5ctx,    buf, n);
    if(Sums.DoSHA1)     SHA1_Update(&sha1ctx,   buf, n);
    if(Sums.DoSHA256) SHA256_Update(&sha256ctx, buf, n);
    if(Sums.DoSHA512) SHA512_Update(&sha512ctx, buf, n);
  }
  CloseHandle(f);

  CRC_Final(crcdgst, &crcctx);
  MD4_Final(md4dgst, &md4ctx);
  ED2K_Final(ed2kdgst, &ed2kctx);
  MD5_Final(md5dgst, &md5ctx);
  SHA1_Final(sha1dgst, &sha1ctx);
  SHA256_Final(sha256dgst, &sha256ctx);
  SHA512_Final(sha512dgst, &sha512ctx);

  File.CRC    = HexDump(crcdgst, CRC_DIGEST_LENGTH);
  File.MD4    = HexDump(md4dgst, MD4_DIGEST_LENGTH);
  File.ED2K   = HexDump(ed2kdgst, ED2K_DIGEST_LENGTH);
  File.MD5    = HexDump(md5dgst, MD5_DIGEST_LENGTH);
  File.SHA1   = HexDump(sha1dgst, SHA_DIGEST_LENGTH);
  File.SHA256 = HexDump(sha256dgst, SHA256_DIGEST_LENGTH);
  File.SHA512 = HexDump(sha512dgst, SHA512_DIGEST_LENGTH);

  if(SelfProgressOnly)
    SumSize = 0;

  return;
}

bool IsValidFileName(string Name)
{
  if(Name.size() == 0)
    return false;
  for(int i = 0; i < Name.size(); i++) 
  {
    if(!isprint(Name[i]))
      return false;
    if(strchr("\\/:*?\"<>|", Name[i]) != NULL)
      return false;
  }
  return true;
}

bool CheckSumFile(string Name)
{
  bool result = true;
  unsigned long long FileSize = GetFileSize(Name);
  unsigned long long Read = 0;

  cout << "  " << Name << ":";

  ifstream f;
  f.open(Name.c_str());
  if(!f.is_open())
  {
    cout << "Could not open for reading." << endl << endl;
    return false;
  }
  cout << endl;
  
  unsigned int Errors = 0;
  unsigned int Successes = 0;
  unsigned int LineN = 0;
  char buf[16384];
  while(true)
  {
    f.getline(buf, 16384);
    LineN++;
    Read += strlen(buf) + 2;
    if(f.eof())
      break;
    if(f.fail())
      return false;
    if(buf[0] == ';')
      continue;
    string s(buf);
    s = Trim(s);
    bool IsMD5 = false, IsCRC = false;
    string CRCFile, MD5File, CRC, MD5;
    if(s.size() >= 10)
    {
      IsCRC = true;
      for(unsigned int i = s.size() - 8; i < s.size(); i++)
        if(!IsHex(s[i]))
        {
          IsCRC = false;
          break;
        }
      if(strchr(" \t*", s[s.size() - 9]) == NULL) 
        IsCRC = false;
      if(IsCRC)
      {
        CRC = s.substr(s.size() - 8);
        int i;
        for(i = 10; i <= s.size(); i--)
          if(strchr(" \t*", s[s.size() - i]) == NULL)
            break;
        CRCFile = Trim(s.substr(0, s.size() - i + 1));
        if(!IsValidFileName(CRCFile))
          IsCRC = false;
      }
    }
    if(s.size() >= 34)
    {
      IsMD5 = true;
      for(unsigned int i = 0; i < 32; i++)
        if(!IsHex(s[i]))
        {
          IsMD5 = false;
          break;
        }
      if(strchr(" \t*", s[32]) == NULL) 
        IsMD5 = false;
      if(IsMD5)
      {
        MD5 = s.substr(0, 32);
        int i;
        for(i = 33; i < s.size(); i++)
          if(strchr(" \t*", s[i]) == NULL)
            break;
        MD5File = Trim(s.substr(i));
        if(!IsValidFileName(MD5File))
          IsMD5 = false;
      }
    }
    if(IsMD5 && IsCRC)
    {
      if(UpperCase(ExtractFileExt(Name)) == "MD5")
        IsCRC = false;
      else if(UpperCase(ExtractFileExt(Name)) == "SFV")
        IsMD5 = false;
      else
      {
        cout << "Ambiguity (line " << LineN << ")" << endl;
        result = false;
        continue;
      }
    }
    if(!IsMD5 && !IsCRC)
    {
      cout << "Unreadable file format (line " << LineN << ")" << endl << endl;
      return false;
    }
    if(IsMD5)
    {
      entry File = MakeEntry(ExtractFileDir(Name) + MD5File);
      string out = MD5File;
      HashFile(File, false, CheckSumOpts(true, false, false, false, false, false, false));
      if(!File.Ok)
      {
        out = "\rCould not open file: " + out;
        Errors++;
      }
      else if(UpperCase(MD5) == UpperCase(File.MD5))
      {
        out = "\rOk: " + out;
        Successes++;
      }
      else
      {
        out = "\rWrong MD5: " + out;
        Errors++;
      }
      cout << out;
      if(out.size() < 80)
        cout << string(80 - out.size() - 1, ' ');
      cout << endl;
    }
    if(IsCRC)
    {
      entry File = MakeEntry(ExtractFileDir(Name) + CRCFile);
      string out = CRCFile;
      HashFile(File, false, CheckSumOpts(false, false, false, true, false, false, false));
      if(!File.Ok)
      {
        out = "\rCould not open file: " + out;
        Errors++;
      }
      else if(UpperCase(CRC) == UpperCase(File.CRC))
      {
        out = "\rOk: " + out;
        Successes++;
      }
      else
      {
        out = "\rWrong CRC: " + out;
        Errors++;
      }
      cout << out;
      if(out.size() < 80)
        cout << string(80 - out.size() - 1, ' ');
      cout << endl;
    }
  }
  f.close();

  if((float) Read / FileSize < 0.5 || (Errors == 0 && Successes == 0))
    cout << "Unreadable file format (line " << LineN << ")" << endl << endl;
  else
    cout << "  Errors: " << Errors << endl << "  Ok: " << Successes << endl << endl;

  return (Errors == 0);
}

void Synopsis()
{
  cout << "arxsum - ArX Checksum Calculator" << endl;
  cout << "" << endl;
  cout << "USAGE:" << endl;
  cout << "arxsum filemask1 [filemaskN...] [options]" << endl;
  cout << "Possible options:" << endl;
  cout << "-r          Recursive" << endl;
  cout << "-crc        Calculate crc" << endl;
  cout << "-md4        Calculate md4" << endl;
  cout << "-ed2k       Calculate ed2k hash" << endl;
  cout << "-md5        Calculate md5" << endl;
  cout << "-sha1       Calculate sha1" << endl;
  cout << "-sha256     Calculate sha256" << endl;
  cout << "-sha512     Calculate sha512" << endl;
  cout << "-all        Calculate all possible checksums" << endl;
  cout << "-of         Output to file with name set according to filenames processed" << endl;
  cout << "-o=FILE     Output to FILE. Cannot be used with -of" << endl;
  cout << "-nc         Do not check sums in *.md5 and *.sfv files, hash them instead" << endl;
  cout << "-c          Treat all input files as checksum files. Cannot be used with -nc" << endl;
  cout << "-np         Do not output progress" << endl;
  cout << "-i[=FILE]   Read filelist from FILE. If FILE is not specified, then from stdin";
  return;
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    Synopsis();
    return 0;
  }
  
  for(int i = 1; i < argc; i++)
  {
    if(argv[i][0]=='-')
    {
      if(strcmp(argv[i], "-all") == 0)
        Sums.DoCRC = Sums.DoMD4 = Sums.DoED2K = Sums.DoMD5 = Sums.DoSHA1 = Sums.DoSHA256 = Sums.DoSHA512 = true;
      else if(strcmp(argv[i], "-crc") == 0)
        Sums.DoCRC = true;
      else if(strcmp(argv[i], "-md4") == 0)
        Sums.DoMD4 = true;
      else if(strcmp(argv[i], "-ed2k") == 0)
        Sums.DoED2K = true;
      else if(strcmp(argv[i], "-md5") == 0)
        Sums.DoMD5 = true;
      else if(strcmp(argv[i], "-sha1") == 0)
        Sums.DoSHA1 = true;
      else if(strcmp(argv[i], "-sha256") == 0)
        Sums.DoSHA256 = true;
      else if(strcmp(argv[i], "-sha512") == 0)
        Sums.DoSHA512 = true;
      else if(strcmp(argv[i], "-of") == 0)
        SmartOutput = true;
      else if(strcmp(argv[i], "-r") == 0)
        Recursive = true;
      else if(strcmp(argv[i], "-c") == 0)
        CheckAll = true;
      else if(strcmp(argv[i], "-nc") == 0)
        HashAll = true;
      else if(strcmp(argv[i], "-np") == 0)
        NoProgress = true;
      else if(strlen(argv[i]) > 3 && argv[i][1] == 'o' && argv[i][2] == '=')
      {
        FileOutput = true;
        OutFile = &argv[i][3];
      }
      else if(strlen(argv[i]) >= 2 && argv[i][1] == 'i')
      {
        if(strlen(argv[i]) > 3 && argv[i][2] == '=')
        {
          UseFileIn = true;
          InFile = &argv[i][3];
        }
        else if(strlen(argv[i]) == 2)
          UseStdIn = true;
      }
    }
    else
      Mask.push_back(argv[i]);
  }

  if(!(Sums.DoCRC || Sums.DoMD4 || Sums.DoED2K || Sums.DoMD5 || Sums.DoSHA1 || Sums.DoSHA256 || Sums.DoSHA512) || 
    (CheckAll && HashAll))
  {
    Synopsis();
    return 0;
  }

  if(UseFileIn)
  {
    ifstream f;
    f.open(InFile.c_str());
    if(f.is_open())
    {
      char buf[16384];
      while(true)
      {
        f.getline(buf, 16384);
        if(f.eof() || f.fail())
          break;
        FileList.insert(MakeEntry(string(buf)));
      }
    }
  }

  if(UseStdIn)
  {
    char buf[16384];
    while(true)
    {
      cin.getline(buf, 16384);
      if(cin.eof() || cin.fail())
        break;
      FileList.insert(MakeEntry(string(buf)));
    }
  }

  for(vector<string>::iterator i = Mask.begin(); i != Mask.end(); i++)
    MakeFileList(*i);

  if(FileList.size() == 0)
    return 0;

  if(!HashAll)
  {
    set<entry>::iterator i = FileList.begin();
    while(i != FileList.end()) 
    {
      if(CheckAll)
      {
        CheckSumFile(i->Name);
        i++;
      }
      else
      {
        string Ext = UpperCase(ExtractFileExt(i->Name));
        if(Ext == "MD5" || Ext == "SFV")
        {
          CheckSumFile(i->Name);
          i = FileList.erase(i);
        }
        else
          i++;
      }
    }
    cout << endl;
    if(CheckAll)
      return 0;
  }

  if(FileList.size() == 0)
    return 0;

  ostream* out = &cout;

  if(SmartOutput)
  {
    string s = FileList.begin()->Name;
    for(set<entry>::iterator i = ++FileList.begin(); i != FileList.end(); i++)
    {
      int j;
      for(j = 0; j < s.size(); j++)
        if(i->Name[j] != s[j])
          break;
      s.erase(j);
      if(s.empty())
        break;
    }
    s = Trim(s);
    if(s.empty() && Sums.DoMD5)
      s = "md5";
    if(s.empty())
      s = "sum";
    if(Sums.DoMD5)
      s += ".md5";
    
    ofstream* f = new ofstream;
    f->open(s.c_str());
    if(f->is_open())
      out = f;
    else
      delete f;
  }
  else if(FileOutput)
  {
    ofstream* f = new ofstream;
    f->open(OutFile.c_str());
    if(f->is_open())
      out = f;
    else
      delete f;
  }

  *out << "; Generated by ArXSum v1.01 on " << GetDate() << " at " << GetTime() << endl;
  *out << "; ArXSum includes cryptographic software written by Eric Young (eay@cryptsoft.com)" << endl;
  *out << "; (c) Alexander 'Elric' Fokin, [ArX] Team, 2007" << endl;
  *out << ";" << endl;

  SumSize = Progress = 0;
  LastTickCount = GetTickCount();
  LastProgress = Progress;

  for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++)
  {
    i->Date = GetFileModifyDate(i->Name);
    i->Time = GetFileModifyTime(i->Name);
    SumSize += (i->Size = GetFileSize(i->Name));
  }

  for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++)
    HashFile(*i, NoProgress, Sums);

  for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
    *out << "; " << setw(12) << i->Size << "  " << i->Time << " " << i->Date << " " << i->Name << endl;

  if(Sums.DoCRC)
  {
    *out << ";" << endl << "; * CRC Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->CRC << " *" << i->Name << endl;
  }
  if(Sums.DoMD4)
  {
    *out << ";" << endl << "; * MD4 Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->MD4 << " *" << i->Name << endl;
  }
  if(Sums.DoED2K)
  {
    *out << ";" << endl << "; * ED2K Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->ED2K << " *" << i->Name << endl;
  }
  if(Sums.DoSHA1)
  {
    *out << ";" << endl << "; * SHA1 Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->SHA1 << " *" << i->Name << endl;
  }
  if(Sums.DoSHA256)
  {
    *out << ";" << endl << "; * SHA256 Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->SHA256 << " *" << i->Name << endl;
  }
  if(Sums.DoSHA512)
  {
    *out << ";" << endl << "; * SHA512 Block *" << endl;
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << "; " << i->SHA512 << " *" << i->Name << endl;
  }
  if(Sums.DoMD5)
  {
    for(set<entry>::iterator i = FileList.begin(); i != FileList.end(); i++) if(i->Ok)
      *out << i->MD5 << " *" << i->Name << endl;
  }

  if(out != &cout)
  {
    ((ofstream*) out)->close();
    delete out;
  }

  return 0;
}