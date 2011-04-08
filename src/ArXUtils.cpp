#include "ArXUtils.h"
#include <Windows.h>
#include <string>
#include <sstream>

using namespace std;

string UpperCase(const string s)
{
  string result = s;
  for(int i = 0; i < result.size(); i++)
    result[i] = toupper(result[i]);
  return result;
}

string HexDump(unsigned char* Data, unsigned int Size)
{
  string result;
  char tmp[3];
  for(int i = 0; i < Size; i++)
  {
    sprintf(tmp, "%02x", Data[i]);
    result += tmp;
  }
  return result;
}

bool IsHex(char c)
{
  switch(c)
  {
  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': 
  case '8': case '9': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    return true;
  default:
    return false;
  }
}

bool IsHex(const string s)
{
  for(int i = 0; i < s.size(); i++)
    if(!IsHex(s[i]))
      return false;
  return true;
}

bool IsBlank(char c)
{
  switch(c) 
  {
  case ' ': case '\t':
  	return true;
  default:
    return false;
  }
}

string ExtractFileName(string Path)
{
  unsigned int pos = Path.find_last_of('\\');
  if(pos == string::npos)
    return Path;
  else
    return Path.substr(pos + 1);
}

string ExtractFileExt(string Path)
{
  unsigned int pos = Path.find_last_of('.');
  if(pos == string::npos)
    return Path;
  else
    return Path.substr(pos + 1);
}

string ExtractFilePath(string Path)
{
  unsigned int pos = Path.find_last_of('\\');
  if(pos == string::npos)
    return "";
  else
    return Path.substr(0, pos + 1);
}

string ExtractFileDrive(string Path)
{
  unsigned int pos = Path.find_first_of(':');
  if(pos == string::npos)
    return "";
  else
    return Path.substr(0, pos + 1) + '\\';
}

string Trim(string s)
{
  int i;
  for(i = s.size() - 1; i >= 0; i--)
    if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
      break;
  s.erase(i + 1);
  for(i = 0; i < s.size(); i++)
    if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
      break;
  if(s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r')
    return s.substr(i);
  else
    return "";
}

string GetTime(SYSTEMTIME *lpTime)
{
  char buf[1024];
  strcpy(buf, "00:00:00");
  GetTimeFormat(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, lpTime, "hh':'mm':'ss", buf, 1023);
  return buf;
}

string GetDate(SYSTEMTIME *lpDate)
{
  char buf[1024];
  strcpy(buf, "0000-00-00");
  GetDateFormat(LOCALE_USER_DEFAULT, 0, lpDate, "yyyy'-'MM'-'dd", buf, 1023);
  return buf;
}

string GetFileModifyTime(string FileName)
{
  WIN32_FIND_DATA FD;
  HANDLE hSearch;
  if((hSearch = FindFirstFile(FileName.c_str(), &FD)) == INVALID_HANDLE_VALUE)
    return "00:00:00";
  SYSTEMTIME stUTC, stLocal;
  if(FileTimeToSystemTime(&FD.ftLastWriteTime, &stUTC) == 0)
    return "00:00:00";
  if(SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal) == 0)
    return "00:00:00";
  FindClose(hSearch);
  return GetTime(&stLocal);
}

string GetFileModifyDate(string FileName)
{
  WIN32_FIND_DATA FD;
  HANDLE hSearch;
  if((hSearch = FindFirstFile(FileName.c_str(), &FD)) == INVALID_HANDLE_VALUE)
    return "0000-00-00";
  SYSTEMTIME stUTC, stLocal;
  if(FileTimeToSystemTime(&FD.ftLastWriteTime, &stUTC) == 0)
    return "0000-00-00";
  if(SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal) == 0)
    return "0000-00-00";
  FindClose(hSearch);
  return GetDate(&stLocal);
}

unsigned long long GetFileSize(string FileName)
{
  WIN32_FIND_DATA FD;
  HANDLE hSearch;
  if((hSearch = FindFirstFile(FileName.c_str(), &FD)) == INVALID_HANDLE_VALUE)
    return 0;
  SYSTEMTIME tModifyTime;
  if(FileTimeToSystemTime(&FD.ftLastWriteTime, &tModifyTime) == 0)
    return 0;
  FindClose(hSearch);
  return ((unsigned long long)FD.nFileSizeHigh << 32) + FD.nFileSizeLow;
}

/*
string HtmlEncode(string s)
{
  string result;
  for(int i = 0; i < s.size(); i++)

}*/