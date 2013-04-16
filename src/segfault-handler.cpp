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
#include <node.h>
#include <time.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <v8-debug.h>
using namespace v8;
using namespace node;

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
  if(fd > 0) write(fd, sbuff, n);
  write(STDERR_FD, sbuff, n);

  // Write the Backtrace
  size = backtrace(array, 32);
  if(fd > 0) backtrace_symbols_fd(array, size, fd);
  backtrace_symbols_fd(array, size, STDERR_FD);

  // Exit violently
  close(fd);
  exit(-1);
}

// create some stack frames to inspect from CauseSegfault
void segfault_stack_frame_1()
{
  int *foo = 0;
  printf("Hello World22\n");
  *foo = 78; // trigger a SIGSEGV
  printf("Hello World3\n");
}

void segfault_stack_frame_2(void) {
  segfault_stack_frame_1();
}

Handle<Value> CauseSegfault(const Arguments& args) {
  segfault_stack_frame_2();
  return Undefined();  // this line never runs
}



Handle<Value> RegisterHandler(const Arguments& args) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = segfault_handler;
  sa.sa_flags   = SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
  return Undefined();
}

extern "C" {
  void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "registerHandler", RegisterHandler);
    NODE_SET_METHOD(target, "causeSegfault", CauseSegfault);
  }
  NODE_MODULE(segvhandler, init);
}
