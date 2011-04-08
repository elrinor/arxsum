#ifndef __ARX_CONFIG_H__
#define __ARX_CONFIG_H__

// -------------------------------------------------------------------------- //
// General settings - feel free to change
// -------------------------------------------------------------------------- //
// Use boost?
#define ARX_USE_BOOST


// -------------------------------------------------------------------------- //
// Guess defines - do not change
// -------------------------------------------------------------------------- //
#if defined(_WIN32) || defined(WIN32)
#  define ARX_WIN
#else
#  define ARX_LINUX
#endif

#endif