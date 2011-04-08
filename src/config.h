#ifndef __CONFIG_H__
#define __CONFIG_H__

// ------------------------------------------------------------------------- //
// Config defines - feel free to change 'em
// ------------------------------------------------------------------------- //
// Use assembler?
#define USE_ASM

// If possible, use direct file access
#define TRY_USING_FAST_IO

// Output progress every OUTPUTPERIOD secs
#define OUTPUTPERIOD 0.5f

// Types
typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned int size_t;
typedef short int16;
typedef int int32;
typedef long long int64;

// Size of buffer for file read operations
#define FILEBUFSIZE	(1024 * 1024)

// Size of data chunk used for speed testing
#define TESTDATACHUNKSIZE ((1024 - 7) * (1024 - 13))

// Maximal number of test iterations
#define TESTMAXITERATIONS 1024

// Run each test for TESTTIME secs, then calculate hashing speed
#define TESTTIME 1.0f

// Maximal number of errors in hash file
#define HASHFILEMAXERRORS 5

// Size of buffer for console io
// Elric[10/7/2007]: functions that read directly from console work really strange when
// we're trying to read >1000000 bytes at a time.
#define CONIOBUFSIZE 2048

// ------------------------------------------------------------------------- //
// DO NOT CHANGE ANYTHING BELOW THIS LINE!
// ------------------------------------------------------------------------- //
// OS detection - gives WIN32 define on Windows
#if defined(_WIN32) || defined(WIN32)
#  ifndef WIN32
#    define WIN32
#  endif
#endif

// Asm defines for different libs
#ifdef USE_ASM
#  define MD5_ASM
#  define SHA1_ASM
#  define CRC_ASM
#  define SHAREAZA_USE_ASM
#endif

// Boost foreach macro
#define FOREACH BOOST_FOREACH

// Direct disc access defines
#ifdef TRY_USING_FAST_IO
#  ifdef WIN32
#    define USE_WIN32_CHUNKINPUTSTREAM
#  endif
#endif

// ^_^
#define VERSION "v1.2.0"

// degugging defines
/*
#if defined(DEBUG) || defined(_DEBUG)
#  define cout use_wcout_instead
#  define cerr use_wcerr_instead
#  define cin  use_wcin_instead
#  define path use_wpath_instead
#endif
*/

#endif
