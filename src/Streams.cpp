#include "Streams.h"

arx::InputStream createInputStream(boost::filesystem::path path, bool unbuffered) {
  if(path.native() == _T("-"))
    return arx::ConsoleInputStream();
  else
    return arx::FileInputStream(path.native(), (unbuffered) ? (arx::OM_UNBUFFERED | arx::OM_SEQUENTIAL_SCAN) : (arx::OM_NONE));
}

arx::OutputStream createOutputStream(boost::filesystem::path path, bool unbuffered) {
  if(path.native() == _T("-"))
    return arx::ConsoleOutputStream();
  else
    return arx::FileOutputStream(path.native(), (unbuffered) ? (arx::OM_UNBUFFERED | arx::OM_SEQUENTIAL_SCAN) : (arx::OM_NONE));
}


arx::Reader createReader(boost::filesystem::path path, bool unbuffered) {
  return arx::InputStreamReader(createInputStream(path, unbuffered));
}

arx::Writer createWriter(boost::filesystem::path path, bool unbuffered) {
  return arx::OutputStreamWriter(createOutputStream(path, unbuffered));
}

arx::Reader createReader(boost::filesystem::path path, const char* encoding, bool unbuffered) {
  return arx::InputStreamReader(createInputStream(path, unbuffered), encoding);
}

arx::Writer createWriter(boost::filesystem::path path, const char* encoding, bool unbuffered) {
  return arx::OutputStreamWriter(createOutputStream(path, unbuffered), encoding);
}
