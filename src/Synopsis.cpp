#include "config.h"
#include "Synopsis.h"
#include <iostream>

using namespace std;

void synopsis() {
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
  cout << "-omd5 FILE      Output in .md5 format (adds -md5)" << endl;
  cout << "-osfv FILE      Output in .sfv format (adds -crc)" << endl;
  cout << "-oed2k FILE     Output in .ed2k format (adds -ed2k)" << endl;
  cout << "  If FILE is set to '*', then output to file with name set according to the" << endl;
	cout << "  filenames processed. If no option is provided, then -omd5 is used." << endl;
  cout << endl;
  cout << "Input options:" << endl;
  cout << "-l              Treat all input files as filelist files." << endl;
  cout << "  If no filemask is provided, then stdin is used." << endl;
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

void synopsisAndExit() {
	synopsis();
	exit(1);
}


