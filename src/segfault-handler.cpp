#include <string.h>
#include <execinfo.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <v8-debug.h>

#ifdef __APPLE__
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

using namespace v8;
using namespace node;

#define STDERR_FD 2

static Persistent<Function> callback;
static bool handlersSet = false;

static void emptyCallback(siginfo_t *si) {
  Local<Array> argStack = Local<Array>::New(Array::New(0));
  Local<Value> argv[3] = {argStack, Local<Value>::New(Number::New(si->si_signo)), Local<Value>::New(Number::New((unsigned long long)si->si_addr))};
  callback->Call(Context::GetCurrent()->Global(), 3, argv);
}

static void segfaultHandler(int sig, siginfo_t *si, void *unused) {
  char buffer[1024 * 10];

  int pid = getpid();

  snprintf(buffer, sizeof(buffer), "PID %d received SIGSEGV/SIGBUS (%i) for address: 0x%llx\n", pid, si->si_signo, (unsigned long long)si->si_addr);
  write(STDERR_FD, buffer, strlen(buffer));

  if (!callback.IsEmpty()) {
    void *stack[32];
    size_t stackSize;

    stackSize = backtrace(stack, 32);

    if (stackSize > 0) {
      char **stackSymbols = backtrace_symbols(stack, stackSize);
      Local<Array> argStack = Local<Array>::New(Array::New(stackSize));
      for (size_t i = 0; i < stackSize; i++) {
        argStack->Set(i, String::New(stackSymbols[i]));
      }
      free(stackSymbols);

      Local<Value> argv[3] = {argStack, Local<Value>::New(Number::New(si->si_signo)), Local<Value>::New(Number::New((unsigned long long)si->si_addr))};
      callback->Call(Context::GetCurrent()->Global(), 3, argv);
    }
    else {
#ifdef __APPLE__
      unw_cursor_t cursor;
      unw_context_t context;
      unw_word_t pc;

      unw_getcontext(&context);
      unw_init_local(&cursor, &context);

      snprintf(buffer, sizeof(buffer), "atos -p %i", pid);

      int frames = 0;
      while (unw_step(&cursor) > 0) {
        frames++;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        int len = strlen(buffer);
        snprintf(buffer + len, sizeof(buffer) - len, " 0x%llx", pc);
      }

      if (frames > 0) {
        FILE* program = popen(buffer, "r");
        if (program != NULL) {
          Local<Array> argStack = Local<Array>::New(Array::New(frames));
          for (int i = 0; i < frames; i++) {
            if (fgets(buffer, sizeof(buffer), program) == NULL) {
              buffer[0] = '\0';
            }
            int len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
              buffer[len - 1] = '\0';
            }
            argStack->Set(i, String::New(buffer));
          }
          pclose(program);

          Local<Value> argv[3] = {argStack, Local<Value>::New(Number::New(si->si_signo)), Local<Value>::New(Number::New((unsigned long long)si->si_addr))};
          callback->Call(Context::GetCurrent()->Global(), 3, argv);
        }
        else {
          emptyCallback(si);
        }
      }
      else {
        emptyCallback(si);
      }
#else
      emptyCallback(si);
#endif
    }
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
    sa.sa_sigaction = segfaultHandler;
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
