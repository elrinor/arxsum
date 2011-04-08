#ifndef __MAIN_H__
#define __MAIN_H__

#include "config.h"
#include "Collections.h"

#ifdef ARX_WIN
#  define MAIN() wmain(int argc, wchar_t** argv)
#  define PROCESS_COMMAND_LINE(x) { \
    arx::ArrayList<wstring> commandLine; \
    for(int i = 0; i < argc; i++) \
      commandLine.push_back(argv[i]); \
    x(commandLine); \
  }
#endif

#ifdef ARX_LINUX
#  define MAIN() main(int argc, char** argv)
#  error "just fuckin' implement me!"
#endif


#endif
