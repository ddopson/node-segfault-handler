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

static Persistent<Function> callback;
static bool handlersSet = false;

static void segfault_handler(int sig, siginfo_t *si, void *unused) {
  void    *array[32]; // Array to store backtrace symbols
  size_t  size;       // To store the size of the stack backtrace
  char    sbuff[128];
  int     n;          // chars written to buffer
  int     pid;

  pid = getpid();

  n = snprintf(sbuff, sizeof(sbuff), "PID %d received SIGSEGV/SIGBUS (%i) for address: 0x%lx\n", pid, si->si_signo, (long)si->si_addr);
  write(STDERR_FD, sbuff, n);

  size = backtrace(array, 32);
  backtrace_symbols_fd(array, size, STDERR_FD);

  if (!callback.IsEmpty()) {
    char **stack = backtrace_symbols(array, size);
    Local<Array> argStack = Local<Array>::New(Array::New(size));
    for (size_t i = 0; i < size; i++) {
      argStack->Set(i, String::New(stack[i]));
    }
    Local<Value> argv[3] = {argStack, Local<Value>::New(Number::New(si->si_signo)), Local<Value>::New(Number::New((long)si->si_addr))};
    callback->Call(Context::GetCurrent()->Global(), 3, argv);
    free(stack);
  }

  // Re-send the signal, this time a default handler will be called
  kill(pid, si->si_signo);
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
  printf("NodeSegfaultHandlerNative: about to dereference NULL (will cause a SIGSEGV)\n");
  *foo = 78; // trigger a SIGSEGV
}

__attribute__ ((noinline)) 
void segfault_stack_frame_2(void) {
  // use a function pointer to thwart inlining
  void (*fn_ptr)() = segfault_stack_frame_1;
  fn_ptr();
}

Handle<Value> CauseSegfault(const Arguments& args) {
  // use a function pointer to thwart inlining
  void (*fn_ptr)() = segfault_stack_frame_2;
  fn_ptr();
  return Undefined();  // this line never runs
}

Handle<Value> RegisterHandler(const Arguments& args) {
  HandleScope scope;

  if (args.Length() > 0) {
    if (!args[0]->IsFunction()) {
      ThrowException(Exception::TypeError(String::New("Invalid callback argument")));
      return scope.Close(Undefined());
    }

    if (!callback.IsEmpty()) {
      callback.Dispose();
      callback.Clear();
    }
    callback = Persistent<Function>::New(Handle<Function>::Cast(args[0]));
  }

  // Set our handler only once
  if (!handlersSet) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND; // We set SA_RESETHAND so that our handler is called only once
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    handlersSet = true;
  }

  return scope.Close(Undefined());
}

extern "C" {
  void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "registerHandler", RegisterHandler);
    NODE_SET_METHOD(target, "causeSegfault", CauseSegfault);
  }
  NODE_MODULE(segfault_handler, init);
}
