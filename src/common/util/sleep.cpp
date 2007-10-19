#include <psconfig.h>

#ifdef USE_WINTASK
#include <windows.h>
void psSleep (unsigned long msec)
{
  Sleep (msec);
}
#elif defined(CS_COMPILER_GCC) && defined(CS_PLATFORM_WIN32)
void psSleep (unsigned long msec)
{
  Sleep (msec);
}
#else
#include <unistd.h>
void psSleep (unsigned long msec)
{
  usleep (msec*1000);
}
#endif
