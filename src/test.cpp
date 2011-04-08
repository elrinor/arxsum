#include "config.h"
#include "Test.h"

#include <iostream>
#include <iomanip>
#include <boost/timer.hpp>
#include "arx/Streams.h"
#include "Hash.h"
#include "Hasher.h"

using namespace arx;
using namespace std;
using namespace boost;

void testHashSpeed() {
  wCout << "Testing hash speed...\n";
  unsigned char* dataChunk = new unsigned char[TESTDATACHUNKSIZE];
  for(int i = 0; i < H_COUNT; i++) {
    Hash hash(i, (uint64) TESTDATACHUNKSIZE * TESTMAXITERATIONS);
    wCout << "  " << hash.getName() << ": ";
    timer t;
    int iterations = 0;
    while(t.elapsed() < TESTTIME) {
      hash.update(dataChunk, TESTDATACHUNKSIZE);
      if(++iterations > TESTMAXITERATIONS)
        break;
    }
    wCout << fixed << showpoint << setprecision(3) << (1.0 * iterations * TESTDATACHUNKSIZE) / (t.elapsed() * 1024 * 1024) << "Mbps\n";
  }
  wCout << "End of test.\n";
  delete[] dataChunk;
}
