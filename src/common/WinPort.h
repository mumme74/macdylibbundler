#ifndef _WIN_PORT_
#define _WIN_PORT_

#i fdef _WIN32
#include <windows.h>
#include <iostream>
#include <cassert>

#define WEXITSTATUS(status) (((status) & 0xFF00) >> 8)

#define WIFEXITED(status) (((status) & 0x7F) == 0)


pid_t waitpid(pid_t pid, int *status, int options) {
  assert(options == 1 && "Only support no hanging wait");

  DWORD result = WaitForSingleObject(OpenProcess(SYNCHRONIZE, false, pid), 0);
  if (result == WAIT_TIMEOUT) {
      return 0; // Child is still running
  } else if (result == WAIT_OBJECT_0) {
      if (status) {
          DWORD exitCode;
          if (GetExitCodeProcess(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid), &exitCode)) {
              *status = exitCode;
          } else {
              std::cerr << "Failed to get exit code" << std::endl;
              return -1;
          }
      }
      return pid; // Child has terminated
  } else {
      std::cerr << "WaitForSingleObject failed" << std::endl;
      return -1;
  }
}

pid_t wait(int status)
{
  DWORD res = WaitForSingleObject()
}

#endif

#endif // _WIN_PORT_
