#include "config.h"
#include "Options.h"

#include <boost/algorithm/string.hpp>
#include "arx/Converter.h"
#include "arx/Streams.h"
#include "Test.h"
#include "Hash.h"
#include "Output.h"

using namespace boost;
using namespace boost::algorithm;
using namespace std;
using namespace arx;

// -------------------------------------------------------------------------- //
// Some helper functions
// -------------------------------------------------------------------------- //
void reportParseError(wstring badToken, wstring problem) {
  wCout << "Error while parsing command line around \"" << badToken << "\" token: " << problem << "." << endl;
  wCout << endl;
  wCout << "Run `arxsum --help` for help." << endl;
  exit(0);
}

bool isOption(wstring cmdLine, wstring option) {
  if(!starts_with(cmdLine, option))
    return false;
  if(cmdLine.size() == option.size())
    return true;
  if(cmdLine[option.size()] != _T('='))
    return false;
  if(cmdLine.size() < option.size() + 2)
    return false;
  return true;
}

wstring extractArgument(ArrayList<wstring> cmdLine, unsigned int& i) {
  wstring::size_type pos = cmdLine[i].find_first_of(_T('='));
  if(pos != wstring::npos)
    return cmdLine[i].substr(pos + 1);
  else {
    if(cmdLine.size() <= i + 1)
      reportParseError(cmdLine[i], _T("argument expected"));
    return cmdLine[++i];
  }
}

// -------------------------------------------------------------------------- //
// OptionsImpl
// -------------------------------------------------------------------------- //
namespace detail {
  class OptionsImpl {
  public:
    bool recursive;
    bool multiThreaded;
    bool checkMode;
    bool usingFileList;
    bool quiet;
    bool upperCase;

    string inputEncoding;
    string outputEncoding;

    HashTask hashTask;
    OutputTask outputTask;

    ArrayList<wstring> fileMasks;
    
    OptionsImpl() {
      this->recursive = false;
      this->multiThreaded = false;
      this->checkMode = false;
      this->usingFileList = false;
      this->quiet = false;
      this->upperCase = false;
      outputEncoding = inputEncoding = "utf-8";
    }
  };
}

// -------------------------------------------------------------------------- //
// Options
// -------------------------------------------------------------------------- //
Options::Options() {
  this->impl = new ::detail::OptionsImpl();
}

Options::~Options() {
  delete this->impl;
}

void Options::parse(ArrayList<wstring> cmdLine) {
  for(unsigned int i = 1; i < cmdLine.size(); i++) {
    wstring arg = cmdLine[i];
    if(starts_with(arg, _T("--"))) {
      if(arg == _T("--version")) {
        printVersion();
        exit(0);
      } else if(arg == _T("--help")) {
        printSynopsis();
        exit(0);
      } else if(arg == _T("--check")) {
        this->impl->checkMode = true;
      } else if(arg == _T("--quiet")) {
        this->impl->quiet = true;
      } else if(arg == _T("--recursive")) {
        this->impl->recursive = true;
      } else if(arg == _T("--multithreaded")) {
        this->impl->multiThreaded = true;
      } else if(arg == _T("--test")) {
        testHashSpeed();
        exit(0);
      } else if(arg == _T("--crc32")) {
        this->impl->hashTask.add(H_CRC);
      } else if(arg == _T("--md5")) {
        this->impl->hashTask.add(H_MD5);
      } else if(arg == _T("--sha1")) {
        this->impl->hashTask.add(H_SHA1);
      } else if(arg == _T("--ed2k")) {
        this->impl->hashTask.add(H_ED2K);
      } else if(arg == _T("--tth")) {
        this->impl->hashTask.add(H_TTH);
      } else if(arg == _T("--md4")) {
        this->impl->hashTask.add(H_MD4);
      } else if(arg == _T("--sha256")) {
        this->impl->hashTask.add(H_SHA256);
      } else if(arg == _T("--sha512")) {
        this->impl->hashTask.add(H_SHA512);
      } else if(arg == _T("--all")) {
        for(int i = 0; i < H_COUNT; i++)
          this->impl->hashTask.add(i);
      } else if(isOption(arg, _T("--uppercase"))) {
        this->impl->upperCase = true;
      } else if(isOption(arg, _T("--omd5"))) {
        this->impl->outputTask.add(O_MD5, extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--osfv"))) {
        this->impl->outputTask.add(O_SFV, extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--oed2k"))) {
        this->impl->outputTask.add(O_ED2K, extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--osha1"))) {
        this->impl->outputTask.add(O_SHA1, extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--obsd"))) {
        this->impl->outputTask.add(O_BSD, extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--oenc"))) {
        this->impl->outputEncoding = toNarrowString(extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--ienc"))) {
        this->impl->inputEncoding = toNarrowString(extractArgument(cmdLine, i));
      } else if(isOption(arg, _T("--list"))) {
        this->impl->usingFileList = true;
      } else 
        reportParseError(arg, _T("unknown option: ") + arg);
    } else if(starts_with(arg, _T("-"))) {
      for(unsigned int j = 1; j < arg.size(); j++) {
        char c = (char) arg[j];
        if(c == 'V') {
          printVersion();
          exit(0);
        } else if(c == 'h') {
          printSynopsis();
          exit(0);
        } else if(c == 'c') {
          this->impl->checkMode = true;
        } else if(c == 'q') {
          this->impl->quiet = true;
        } else if(c == 'r') {
          this->impl->recursive = true;
        } else if(c == 'm') {
          this->impl->multiThreaded = true;
        } else if(c == 't') {
          testHashSpeed();
          exit(0);
        } else if(c == 'C') {
          this->impl->hashTask.add(H_CRC);
        } else if(c == '4') {
          this->impl->hashTask.add(H_MD4);
        } else if(c == 'M') {
          this->impl->hashTask.add(H_MD5);
        } else if(c == 'H') {
          this->impl->hashTask.add(H_SHA1);
        } else if(c == 'E') {
          this->impl->hashTask.add(H_ED2K);
        } else if(c == 'T') {
          this->impl->hashTask.add(H_TTH);
        } else if(c == 'u') {
          this->impl->upperCase = true;
        } else if(c == 'l') {
          this->impl->usingFileList = true;
        } else 
          reportParseError(arg, toWideString(string("unknown option: -") + c));
      }
    } else 
      this->impl->fileMasks.add(arg);
  }
  
  // If nothing is specified - output in .md5 format
  uint32 sum = 0;
  for(uint32 i = 0; i < this->impl->outputTask.size(); i++)
    sum += this->impl->outputTask.isSet(i) ? 1 : 0;
  if(sum == 0)
    this->impl->outputTask.add(O_MD5, _T("-"));

  // Add hashes required by output format
  for(uint32 i = 0; i < this->impl->outputTask.size(); i++)
    if(this->impl->outputTask.isSet(i) && outTaskRequirement[i] != H_UNKNOWN)
      this->impl->hashTask.add(outTaskRequirement[i]);
  
  // If no hashsums specified - calculate md5
  sum = 0;
  for(uint32 i = 0; i < this->impl->hashTask.size(); i++)
    sum += this->impl->hashTask.isSet(i) ? 1 : 0;
  if(sum == 0)
    this->impl->hashTask.add(H_MD5);
}

void Options::printVersion() {
  wCout << "arxsum - ArX Checksum Calculator " << VERSION << endl;
}

void Options::printSynopsis() {
  printVersion();
  wCout << "" << endl;
  wCout << "Usage:" << endl;
  wCout << "  arxsum [options...] [filemasks...]" << endl;
  wCout << endl;
  wCout << "General options:" << endl;
  wCout << "  -V, --version Print arxsum version and exit." << endl;
  wCout << "  -h, --help    Print this help screen." << endl;
  wCout << "  -c, --check   Check mode - treat all input files as checksum files." << endl;
  wCout << "  -q, --quiet   Do not output progress." << endl;
  wCout << "  -r, --recursive  Process directories recursively." << endl;
  wCout << "  -m, --multithreaded  Calculate each hash in a separate thread." << endl;
  wCout << "  -t, --test    Do not hash anything, just test hash speed." << endl;
  wCout << endl;
  wCout << "Checksum options:" << endl;
  wCout << "  -C, --crc32   Calculate crc32 hash." << endl;
  wCout << "  -M, --md5     Calculate md5 hash." << endl;
  wCout << "  -H, --sha1    Calculate sha1 hash." << endl;
  wCout << "  -E, --ed2k    Calculate ed2k hash." << endl;
  wCout << "  -T, --tth     Calculate tth hash." << endl;
  wCout << "  -4, --md4     Calculate md4 hash." << endl;
  wCout << "      --sha256  Calculate sha256 hash." << endl;
  wCout << "      --sha512  Calculate sha512 hash." << endl;
  wCout << "      --all     Calculate all hashes." << endl;
  wCout << endl;
  wCout << "Output options:" << endl;
  wCout << "  -u, --uppercase  Output hash in uppercase." << endl;
  wCout << "      --oenc ENCODING  Output using given character encoding." << endl;
  wCout << "      --omd5 FILE  Output in .md5 format (adds --md5)." << endl;
  wCout << "      --osfv FILE  Output in .sfv format (adds --crc)." << endl;
  wCout << "      --oed2k FILE  Output in .ed2k format (adds --ed2k)." << endl;
  wCout << "      --osha1 FILE  Output in .sha1 format (adds --sha1)." << endl;
  wCout << "      --obsd FILE  Output in .bsd format." << endl;
  wCout << "  If FILE is set to '*', then output to file with name set according to the" << endl;
  wCout << "  filenames processed. If no format option is provided, then -omd5 is used." << endl;
  wCout << "  If no output encoding is specified, then UTF-8 is used." << endl;
  wCout << endl;
  wCout << "Input options:" << endl;
  wCout << "      --ienc ENCODING  Use given character encoding when processing input files." << endl;
  wCout << "  -l, --list    Treat all input files as filelist files." << endl;
  wCout << "  If no filemask is provided, then stdin is used." << endl;
  wCout << "  If no input encoding is specified, then UTF-8 is used." << endl;
  wCout << endl;
  return;
}

bool Options::isRecursive() {
  return this->impl->recursive;
}

bool Options::isMultiThreaded() {
  return this->impl->multiThreaded;
}

bool Options::isInCheckMode() {
  return this->impl->checkMode;
}

bool Options::isUsingFileList() {
  return this->impl->usingFileList;
}

bool Options::isQuiet() {
  return this->impl->quiet;
}

bool Options::isUpperCase() {
  return this->impl->upperCase;
}

const char* Options::getInputEncoding() {
  return this->impl->inputEncoding.c_str();
}

const char* Options::getOutputEncoding() {
  return this->impl->outputEncoding.c_str();
}

HashTask Options::getHashTask() {
  return this->impl->hashTask;
}

OutputTask Options::getOutputTask() {
  return this->impl->outputTask;
}

ArrayList<wstring> Options::getFileMasks() {
  return this->impl->fileMasks;
}


// -------------------------------------------------------------------------- //
// Global options
// -------------------------------------------------------------------------- //
Options options;
