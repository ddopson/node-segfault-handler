#include <fcntl.h>
#include <nan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <v8-debug.h>
#include <time.h>

#ifdef _WIN32
#include "../includes/StackWalker.h"
#include <io.h>
#include <process.h>
#include <windows.h>
#else
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#endif

using namespace v8;
using namespace Nan;

#define STDERR_FD 2

#ifdef _WIN32
#define CLOSE _close
#define GETPID _getpid
#define O_FLAGS _O_CREAT | _O_APPEND | _O_WRONLY
#define OPEN _open
#define S_FLAGS _S_IWRITE
#define SEGFAULT_HANDLER LONG CALLBACK segfault_handler(PEXCEPTION_POINTERS exceptionInfo)
#define SNPRINTF _snprintf
#define WRITE _write
#else
#define CLOSE close
#define GETPID getpid
#define O_FLAGS O_CREAT | O_APPEND | O_WRONLY
#define OPEN open
#define S_FLAGS S_IRUSR | S_IRGRP | S_IROTH
#define SEGFAULT_HANDLER static void segfault_handler(int sig, siginfo_t *si, void *unused)
#define SNPRINTF snprintf
#define WRITE write
#endif

#define BUFF_SIZE 128

char logPath[BUFF_SIZE];

static void buildFileName(char sbuff[BUFF_SIZE], int pid) {
  time_t  now;

  // Construct a filename
  time(&now);
  if (logPath[0] != '\0') {
    SNPRINTF(sbuff, BUFF_SIZE, "%s", logPath);
  } else {
    SNPRINTF(sbuff, BUFF_SIZE, "stacktrace-%d-%d.log", (int)now, pid);
  }
}

SEGFAULT_HANDLER {
  long    address;
  #ifndef _WIN32
    void    *array[32]; // Array to store backtrace symbols
    size_t  size;       // To store the size of the stack backtrace
    int     signal;     // Which signal was received?
  #endif
  char    sbuff[BUFF_SIZE];
  int     n;          // chars written to buffer
  int     fd;
  int     pid;

  pid = GETPID();

  buildFileName(sbuff, pid);
  fd = OPEN(sbuff, O_FLAGS, S_FLAGS);

  #ifdef _WIN32
    address = (long)exceptionInfo->ExceptionRecord->ExceptionAddress;
  #else
    address = (long)si->si_addr;
    signal = si->si_signo;
  #endif

  // Write the header line
  n = SNPRINTF(
    sbuff,
    BUFF_SIZE,
    "PID %d received "
  #ifdef _WIN32
    "segfault"
  #else
    "signal %d"
  #endif
    " for address: 0x%lx\n",
    pid,
  #ifndef _WIN32
    signal,
  #endif
    address
  );

  if(fd > 0) {
    if (WRITE(fd, sbuff, n) != n) {
      fprintf(stderr, "NodeSegfaultHandlerNative: Error writing to file\n");
    }
  }
  fprintf(stderr, "%s", sbuff);

  #ifdef _WIN32
    // will generate the stack trace and write to fd and stderr
    StackWalker sw(fd);
    sw.ShowCallstack();
  #else
    // Write the Backtrace
    size = backtrace(array, 32);
    if(fd > 0) backtrace_symbols_fd(array, size, fd);
    backtrace_symbols_fd(array, size, STDERR_FD);
  #endif

  // Exit violently
  CLOSE(fd);
  exit(-1);

  #ifdef _WIN32
    return EXCEPTION_EXECUTE_HANDLER;
  #endif
}

// create some stack frames to inspect from CauseSegfault
#ifndef _WIN32
__attribute__ ((noinline))
#else
__declspec(noinline)
#endif
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

#ifndef _WIN32
__attribute__ ((noinline))
#else
__declspec(noinline)
#endif
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
  int sigArg = -1, signal = SIGSEGV;
  // if passed a path, we'll set the log name to whatever is provided
  // this will allow users to use the logs in error reporting without
  // redirecting stderr
  if (info.Length() >= 1) {
    if (info[0]->IsString()) {
        v8::String::Utf8Value utf8Value(info[0]->ToString());

        // need to do a copy to make sure the string doesn't become a dangling pointer
        int len = utf8Value.length();
        len = len > BUFF_SIZE ? BUFF_SIZE : len;

        strncpy(logPath, *utf8Value, len);
        logPath[BUFF_SIZE-1] = '\0';
        if (info.Length() >= 2 && info[1]->IsNumber()) {
          sigArg = 1;
        }
    } else if (info[0]->IsNumber()) {
      sigArg = 0;
    } else {
      return ThrowError("First argument must be a string.");
    }
  }
  if (sigArg >= 0) {
    signal = Nan::To<int32_t>(info[sigArg]).FromMaybe(SIGSEGV);
  }

  #ifdef _WIN32
    AddVectoredExceptionHandler(1, segfault_handler);
  #else
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags   = SA_SIGINFO;
    sigaction(signal, &sa, NULL);
  #endif
}

extern "C" {
  NAN_MODULE_INIT(init) {
    logPath[0] = '\0';
    Nan::SetMethod(target, "registerHandler", RegisterHandler);
    Nan::SetMethod(target, "causeSegfault", CauseSegfault);
    // Export signal names and values.
#define EXPORT(signal) \
    Nan::ForceSet(target, Nan::New<v8::String>(#signal).ToLocalChecked(), Nan::New(signal), v8::ReadOnly)
    // Not all of these make sense to register handlers on, but we'll let
    // the user decide that.  Presumably you're using this package because
    // you're seeing an unexpected signal of some sort.  Hopefully it's
    // included below.  (And if not, just pass it by integer value.)
    EXPORT(SIGHUP);
    EXPORT(SIGINT);
    EXPORT(SIGQUIT);
    EXPORT(SIGILL);
    EXPORT(SIGTRAP);
    EXPORT(SIGABRT);
    EXPORT(SIGBUS);
    EXPORT(SIGFPE);
    EXPORT(SIGKILL);
    EXPORT(SIGUSR1);
    EXPORT(SIGUSR2);
    EXPORT(SIGSEGV);
    EXPORT(SIGUSR2);
    EXPORT(SIGPIPE);
    EXPORT(SIGALRM);
    EXPORT(SIGTERM);
    //EXPORT(SIGSTKFLT); // not present on OSX
    EXPORT(SIGCHLD);
    EXPORT(SIGCONT);
    EXPORT(SIGSTOP);
    EXPORT(SIGTSTP);
    EXPORT(SIGTTIN);
    EXPORT(SIGTTOU);
    EXPORT(SIGURG);
    EXPORT(SIGXCPU);
    EXPORT(SIGXFSZ);
    EXPORT(SIGVTALRM);
    EXPORT(SIGPROF);
    EXPORT(SIGWINCH);
    EXPORT(SIGIO);
    //EXPORT(SIGPWR); // not present on OSX
    EXPORT(SIGSYS);
  }

  NODE_MODULE(segfault_handler, init)
}
