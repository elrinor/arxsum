#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "arx/config.h"

// ------------------------------------------------------------------------- //
// Config defines - feel free to change 'em
// ------------------------------------------------------------------------- //
// Use assembler?
#define USE_ASM

// Output progress every OUTPUTPERIOD secs
#define OUTPUTPERIOD 0.5f

// Size of buffer for file read operations
#define FILEBUFSIZE  (1024 * 1024)

// Size of data chunk used for speed testing
#define TESTDATACHUNKSIZE ((1024 - 7) * (1024 - 13))

// Maximal number of test iterations
#define TESTMAXITERATIONS 1024

// Run each test for TESTTIME secs, then calculate hashing speed
#define TESTTIME 1.0f

// Maximal number of errors in hash file
#define HASHFILEMAXERRORS 5

// Maximal size of .torrent file
#define MAX_TORRENT_FILE_SIZE (1024 * 1024 * 32)

// ------------------------------------------------------------------------- //
// DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT YOU'RE DOING!
// ------------------------------------------------------------------------- //
// Types
typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned int size_t;
typedef short int16;
typedef int int32;
typedef long long int64;

// Asm defines for different libs
#ifdef USE_ASM
#  define MD5_ASM
#  define SHA1_ASM
#  define CRC_ASM
#  define SHAREAZA_USE_ASM
#endif

// Boost foreach macro
#define FOREACH BOOST_FOREACH

// ^_^
#define VERSION "v1.3.3"

// Wide strings
#ifndef _T
#  define _T(x) L ## x
#endif

#endif
