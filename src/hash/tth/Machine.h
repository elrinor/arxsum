#ifndef __MACHINE_H__
#define __MACHINE_H__

namespace Machine
{
#ifndef _WIN64
  inline unsigned long long cpuFlags()
  {
    __asm
    {
      mov   eax, 1
        cpuid
        mov   eax, edx
        mov   edx, ecx
    }
  }

  inline bool SupportsMMX() { return ( cpuFlags() & 0x00800000 ) != 0; }
  inline bool SupportsSSE() { return ( cpuFlags() & 0x02000000 ) != 0; }
  inline bool SupportsSSE2() { return ( cpuFlags() & 0x04000000 ) != 0; }
  inline bool SupportsSSE3() { return ( cpuFlags() & 0x100000000 ) != 0; }
#elif defined _WIN64
  inline bool SupportsMMX() { return false; }
  inline bool SupportsSSE() { return true; }
  inline bool SupportsSSE2() { return true; }
#endif
}

#endif