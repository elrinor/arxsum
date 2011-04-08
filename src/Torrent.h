#ifndef __TORRENT_H__
#define __TORRENT_H__

#include "config.h"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include "Checker.h"

void checkTorrent(boost::filesystem::wpath torrentFile, CheckResultReporter reporter = CheckResultReporter());

#endif