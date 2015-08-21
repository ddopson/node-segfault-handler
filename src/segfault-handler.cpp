#include <string.h>
#include <execinfo.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <nan.h>
#include <v8-debug.h>

using namespace v8;

#define STDERR_FD 2

static void segfault_handler(int sig, siginfo_t *si, void *unused) {
  void    *array[32]; // Array to store backtrace symbols
  size_t  size;       // To store the size of the stack backtrace
  char    sbuff[128];
  int     n;          // chars written to buffer
  int     fd;
  time_t  now;
  int     pid;

  // Construct a filename
  time(&now);
  pid = getpid();
  snprintf(sbuff, sizeof(sbuff), "stacktrace-%d-%d.log", (int)now, pid );

  // Open the File
  fd = open(sbuff, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
  // Write the header line
  n = snprintf(sbuff, sizeof(sbuff), "PID %d received SIGSEGV for address: 0x%lx\n", pid, (long) si->si_addr);
  if(fd > 0) if (write(fd, sbuff, n) != n) fprintf(stderr, "NodeSegfaultHandlerNative: Error writing to file\n");
  fprintf(stderr, "%s", sbuff);

  // Write the Backtrace
  size = backtrace(array, 32);
  if(fd > 0) backtrace_symbols_fd(array, size, fd);
  backtrace_symbols_fd(array, size, STDERR_FD);

  // Exit violently
  close(fd);
  exit(-1);
}

// create some stack frames to inspect from CauseSegfault
__attribute__ ((noinline))
void segfault_stack_frame_1()
{
  // DDOPSON-2013-04-16 using the address "1" instead of "0" prevents a nasty compiler over-optimization
  // When using "0", the compiler will over-optimize (unless using -O0) and generate a UD2 instruction
  // UD2 is x86 for "Invalid Instruction" ... (yeah, they have a valid code that means invalid)
  // Long story short, we don't get our SIGSEGV.  Which means no pretty demo of stacktraces.
  // Instead, you see "Illegal Instruction: 4" on the console and the program stops.

  int *foo = (int*)1;
  fprintf(stderr, "NodeSegfaultHandlerNative: about to dereference NULL (will cause a SIGSEGV)\n");
  *foo = 78; // trigger a SIGSEGV

}

__attribute__ ((noinline))
void segfault_stack_frame_2(void) {
  // use a function pointer to thwart inlining
  void (*fn_ptr)() = segfault_stack_frame_1;
  fn_ptr();
}

NAN_METHOD(CauseSegfault) {
  // use a function pointer to thwart inlining
  void (*fn_ptr)() = segfault_stack_frame_2;
  fn_ptr();
}

NAN_METHOD(RegisterHandler) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = segfault_handler;
  sa.sa_flags   = SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
}

extern "C" {
  NAN_MODULE_INIT(init) {
    Nan::SetMethod(target, "registerHandler", RegisterHandler);
    Nan::SetMethod(target, "causeSegfault", CauseSegfault);
  }

  NODE_MODULE(segfault_handler, init)
}
