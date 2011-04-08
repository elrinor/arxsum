#ifndef __STREAMS_H__
#define __STREAMS_H__

#include "config.h"
#include <string>
#include <boost/filesystem.hpp>
#include "arx/Streams.h"

arx::Reader createReader(boost::filesystem::wpath path, bool unbuffered = false);
arx::Writer createWriter(boost::filesystem::wpath path, bool unbuffered = false);
arx::Reader createReader(boost::filesystem::wpath path, const char* encoding, bool unbuffered = false);
arx::Writer createWriter(boost::filesystem::wpath path, const char* encoding, bool unbuffered = false);
arx::InputStream createInputStream(boost::filesystem::wpath path, bool unbuffered = false);
arx::OutputStream createOutputStream(boost::filesystem::wpath path, bool unbuffered = false);

#endif