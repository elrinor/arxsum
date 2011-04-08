#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "config.h"
#include <string>
#include "arx/Collections.h"
#include "Task.h"

using namespace std;
using namespace arx;

namespace detail {
  class OptionsImpl;
}

class Options {
private:
  Options& operator= (const Options& a);
  Options(const Options& a);

  ::detail::OptionsImpl* impl;

public:
  void parse(arx::ArrayList<std::wstring> cmdLine);

  void printSynopsis();
  void printVersion();
  
  Options();
  ~Options();

  bool isRecursive();
  bool isMultiThreaded();
  bool isInCheckMode();
  bool isUsingFileList();
  bool isQuiet();
  bool isUpperCase();

  const char* getInputEncoding();
  const char* getOutputEncoding();

  HashTask getHashTask();
  OutputTask getOutputTask();

  ArrayList<wstring> getFileMasks();
};


extern Options options;

#endif