#ifndef __ARX_CONFIG_H__
#define __ARX_CONFIG_H__

// -------------------------------------------------------------------------- //
// General settings - feel free to change
// -------------------------------------------------------------------------- //
// Use boost?
#define ARX_USE_BOOST

// For statically sized matrices with cols >= ARX_LINEAR_SOLVER_PERMUTATION_VERTOR_USAGE_BORDER
// we use linear solver that doesn't swap matrix rows directly in memory, but performs all operations
// indirectly, by means of a permutation vector
#define ARX_LINEAR_SOLVER_PERMUTATION_VERTOR_USAGE_BORDER 5

// -------------------------------------------------------------------------- //
// Guess defines - do not change
// -------------------------------------------------------------------------- //
#if defined(_WIN32) || defined(WIN32)
#  define ARX_WIN
#else
#  define ARX_LINUX
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#  define DEBUG
#endif

#endif

