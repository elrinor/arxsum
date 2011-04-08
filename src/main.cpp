/*****************************************************************************
ArXSum - HashSum generating / checking utility.

Copyright (c) 2006-2011 Alexander Fokin, apfokin@gmail.com.
 
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
- linux32 port
- linux64 port
- printf output
- 2ch hash checking
- faster crc --failed

v1.3.5
! ported to MSVC2010
- removed unused files.

v1.3.4
* fixed an issue with flush() in arxlib
+ checking system underwent a major refactoring
+ callback system refactored
+ .bsd output format checking

v1.3.3
* fixed stdin hashing while isatty
* -u (uppercase) option had no effect, fixed
* fixed a bug in multithreaded hasher intruduced in v1.3.0

v1.3.2
+ Major rewrite of checking system
+ .torrent file checking

v1.3.1
+ Output in sha1 and bsd format
+ sha1 output format checking
+ -u option (output hash in uppercase)

v1.3.0
+ Unicode support (still needs tuning)
* Command line options format changed

v1.2.0
+ Major rewrite using boost and OOP, the first step towards linux port

v1.1.1
+ TTH hash added

v1.1.0
* Fixed eof without eoln issue
* FILE_SHARE_READ added in CreateFile

v1.0.9
* Fixed absolute paths in .md5 files

v1.0.8
+ Simultaneous output into several files with different formats

v1.0.7
* IsValidFileName fixed - there were bugs with characters with code > 127

v1.0.6
+ Multithreading added

v1.0.5
* Changed command line options format
+ Added .ed2k file checking
+ Added .sfv file output

v1.0.4
* Now treating lines in .md5 files starting with '#' as comments
  Elric: this may lead to a bug in parsing .sfv files
* Fixed bug with blank lines in .md5 files
* IsValidFileName fixed
+ Added .ed2k file output

v1.0.3
* ed2k hashing method changed to the one used in eMule 0.46c
  see http://wiki.anidb.info/w/AniDB:Ed2k-hash for details

v1.0.2
+ Overlapped IO

v1.0.1
+ Output hashing speed
+ Now using faster IO routines (~ x2 speedup)
+ Checksum verification
+ Read filelist from file / stdin

v1.0.0
+ First Release - crc, md4, ed2k, md5, sha1, sha256, sha512 hashing
*****************************************************************************/

#include "config.h"
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>
#include <time.h>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/timer.hpp>
#include <boost/program_options.hpp>
#include "arx/Streams.h"
#include "Hash.h"
#include "ArrayList.h"
#include "Options.h"
#include "Streams.h"
#include "FileEntry.h"
#include "Hasher.h"
#include "Task.h"
#include "Output.h"
#include "Torrent.h"
#include "Checker.h"

#ifdef _WIN32
#  define NOMINMAX
#  include <Windows.h>
#endif

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace boost::filesystem;
using namespace arx;

ArrayList<FileEntry> addToFileList(path dir, wregex fileNameMask, bool isRecursive, ArrayList<FileEntry> fileList) {
  directory_iterator end;
  try {
    for(directory_iterator itr(dir); itr != end; itr++) {
      if(isRecursive && is_directory(itr->status())) {
        addToFileList(*itr, fileNameMask, isRecursive, fileList);
      } else if(is_regular(itr->status())) {
        if(regex_match(itr->path().filename().wstring(), fileNameMask)) {
          wstring filePath = (dir / itr->path().filename()).native();
          if(starts_with(filePath, ".") && !starts_with(filePath, ".."))
            filePath = filePath.substr(2);
          fileList.add(FileEntry(filePath));
        }
      }
    }
  } catch (std::exception &) {
    // cout << e.what() << endl;
  }
  return fileList;
}

ArrayList<FileEntry> addToFileList(wstring filePathMask, bool isRecursive, ArrayList<FileEntry> fileList) {
  path fullPath(filePathMask);
  path dir = fullPath.branch_path();
  if(dir.empty())
    dir = path(_T("."));
  wstring mask = fullPath.filename().wstring();
  struct regex_replacement {
    wregex expression;
    wstring replacement;
  };
  const static regex_replacement replaces[] = {
    //{regex("\\\\"), "\\\\\\\\"},
    //{regex("\\|"), "\\\\|"},
    {wregex(_T("\\.")), _T("\\\\.")},
    {wregex(_T("\\[")), _T("\\\\[")},
    {wregex(_T("\\{")), _T("\\\\{")},
    {wregex(_T("\\(")), _T("\\\\(")},
    {wregex(_T("\\)")), _T("\\\\)")},
    {wregex(_T("\\+")), _T("\\\\+")},
    {wregex(_T("\\^")), _T("\\\\^")},
    {wregex(_T("\\$")), _T("\\\\$")},
    {wregex(_T("\\?")), _T(".")},
    {wregex(_T("\\*")), _T(".*")}
  };
  for(int i = 0; i < sizeof(replaces) / sizeof(regex_replacement); i++)
    mask = regex_replace(mask, replaces[i].expression, replaces[i].replacement);
  return addToFileList(dir, wregex(mask), isRecursive, fileList);
}

ArrayList<FileEntry> fileList;

int main(int argc, char** argv) {
  ArrayList<wstring> commandLine;

#ifdef _WIN32
  {
    int numArgs;
    LPWSTR *lppCommandLine = CommandLineToArgvW(GetCommandLineW(), &numArgs);

    for(int i = 0; i < numArgs; i++)
      commandLine.push_back(lppCommandLine[i]);
  }
#else
#  error Not implemented
#endif

  options.parse(commandLine);

  changeCinCoutEncoding(options.getInputEncoding(), options.getOutputEncoding());

  FOREACH(wstring fileMask, options.getFileMasks())
    addToFileList(fileMask, options.isRecursive(), fileList);

  if(options.getFileMasks().empty())
    fileList.add(FileEntry(_T("-")));

  if(options.isUsingFileList()) {
    ArrayList<FileEntry> newFileList;
    FOREACH(FileEntry file, fileList) {
      Reader reader;
      try {
        reader = createReader(file.getPath(), options.getInputEncoding());
      } catch (...) {
        wCout << "[error] cannot access file: " << file.getPath() << endl;
        continue;
      }
      Scanner scanner(reader);
      while(!scanner.eof() && !scanner.fail()) {
        wstring fileName;
        getline(scanner, fileName);
        newFileList.add(FileEntry(fileName));
      }
    }
    fileList = newFileList;
  }

  sort(fileList.begin(), fileList.end());

  uint64 sumSize = 0;
  FOREACH(FileEntry file, fileList) {
    if(file.getPath() == _T("-")) {
      file.setSize(numeric_limits<uint64>::max());
      time_t lTime;
      time(&lTime);
      file.setDateTime(lTime);
      sumSize += file.getSize();
    } else {
      try {
        file.setSize(file_size(file.getPath()));
        sumSize += file.getSize();
        file.setDateTime(last_write_time(file.getPath()));
      } catch (std::exception &) {
        file.setFailed(true);
        wCout << "[error] cannot access file: " << file.getPath() << endl;
      }
    }
  }

  if(options.isInCheckMode()) {
    Checker checker = Checker(PrinterCheckResultReporter(&wCout));
    FOREACH(FileEntry file, fileList)
      checker.check(file.getPath());
  } else {
    HasherCallBack callback;
    if(!options.isQuiet())
      callback = PrinterHasherCallBack(sumSize, &wCout);
    Hasher hasher(options.getHashTask(), options.isMultiThreaded(), callback);
    FOREACH(FileEntry file, fileList) 
      if(!file.isFailed())
        hasher.hash(file);
    options.getOutputTask().perform(options.getHashTask(), fileList);
  }
  
  return 0;
}


