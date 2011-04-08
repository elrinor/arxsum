#ifndef __ARXUTILS_H__
#define __ARXUTILS_H__

#include <Windows.h>
#include <string>
#include <sstream>

template<class T> std::string ToStr(T i)
{
  std::stringstream s;
  s << i;
  return s.str();
}

std::string UpperCase(const std::string s);

std::string HexDump(unsigned char* Data, unsigned int Size);

bool IsHex(char c);

bool IsHex(const std::string s);

bool IsBlank(char c);

std::string ExtractFileName(std::string Path);

std::string ExtractFileExt(std::string Path);

std::string ExtractFilePath(std::string Path);

std::string ExtractFileDrive(std::string Path);

std::string Trim(std::string s);

std::string GetTime(SYSTEMTIME *lpTime = NULL);

std::string GetDate(SYSTEMTIME *lpDate = NULL);

std::string GetFileModifyTime(std::string FileName);

std::string GetFileModifyDate(std::string FileName);

unsigned long long GetFileSize(std::string FileName);

//std::string HtmlEncode(std::string s);

#endif