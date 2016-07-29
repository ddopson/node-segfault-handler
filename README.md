# Overview

This module is a critical tool for debugging Node.js C/C++ native code modules, and is safe to use in production environments.  Normally, when a bug is triggered in native code, the node process simply ends with no helpful information.  In production, this can manifest as worker processes restarting for seemingly no reason.  Running node in gdb is messy and infeasible for a production environment.  Instead this module will sit unobtrusively doing nothing (zero perf impact) as long as Node is well-behaved.  If a SIGSEGV signal is raised, the module will print a native stack trace to both STDERR and to a timestamped file (STDERR is usually ignored in production environments.  files are better).

Using the module is as simple as:

```javascript

var SegfaultHandler = require('segfault-handler');

SegfaultHandler.registerHandler("crash.log"); // With no argument, SegfaultHandler will generate a generic log file name

// Optionally specify a callback function for custom logging. This feature is currently only supported for Node.js >= v0.12 running on Linux.
SegfaultHandler.registerHandler("crash.log", function(signal, address, stack) {
	// Do what you want with the signal, address, or stack (array)
	// This callback will execute before the signal is forwarded on.
});

SegfaultHandler.causeSegfault(); // simulates a buggy native module that dereferences NULL

```

Obviously, you would only include the first two lines in your own code; the third is for testing purposes and to demonstrate functionality.

After running the above sample, you will see a stacktrace on OSX and Linux that looks like:

```
PID 67926 received SIGSEGV for address: 0x0
0   segfault-handler-native.node        0x00000001007e83d6 _ZL16segfault_handleriP9__siginfoPv + 235
1   libSystem.B.dylib                   0x00007fff844d11ba _sigtramp + 26
2   ???                                 0x00007fff5fc404a0 0x0 + 140734800069792
3   segfault-handler-native.node        0x00000001007e80fd _Z22segfault_stack_frame_2v + 9
4   segfault-handler-native.node        0x00000001007e82d4 _Z13CauseSegfaultRKN2v89ArgumentsE + 17
5   node                                0x00000001000a45de _ZN2v88internalL21Builtin_HandleApiCallENS0_12_GLOBAL__N_116BuiltinArgumentsILNS0_21BuiltinExtraArgumentsE1EEEPNS0_7IsolateE + 430
```

And on Windows:

```
PID 11880 received SIGSEGV for address: 0xfe101419
SymInit: Symbol-SearchPath: '.;c:\github\node-segfault-handler;c:\Program Files\nodejs;C:\Windows;C:\Windows\system32;SRV*C:\websymbols*http://msdl.microsoft.com/download/symbols;', symOptions: 530, UserName: 'tylerw'
OS-Version: 6.3.9600 () 0x100-0x1
c:\github\node-segfault-handler\src\stackwalker.cpp (941): StackWalker::ShowCallstack
c:\github\node-segfault-handler\src\segfault-handler.cpp (114): segfault_handler
00007FFF0A2622C7 (ntdll): (filename not available): RtlNormalizeString
00007FFF0A2138FE (ntdll): (filename not available): RtlWalkFrameChain
00007FFF0A29544A (ntdll): (filename not available): KiUserExceptionDispatcher
c:\github\node-segfault-handler\src\segfault-handler.cpp (138): segfault_stack_frame_1
c:\github\node-segfault-handler\node_modules\nan\nan_callbacks_12_inl.h (175): Nan::imp::FunctionCallbackWrapper
00007FF64489D4A9 (node): (filename not available): v8::Unlocker::~Unlocker
00007FF644865E90 (node): (filename not available): v8::Unlocker::~Unlocker
00007FF644863D79 (node): (filename not available): v8::Unlocker::~Unlocker
00000000C3D060A6 ((module-name not available)): (filename not available): (function-name not available)
000000E36B69E3F8 ((module-name not available)): (filename not available): (function-name not available)
00000000C3D43D02 ((module-name not available)): (filename not available): (function-name not available)
```

Be aware that in production environments, pdb files must be included as part of your install to resolve names / lines in Windows stack traces.

Now you can start debugging using tools like "objdump -dS module.node" to try and sort out what the stack actually means.  Sometimes, just identifying _which_ native module is causing problems is the biggest win.

Cheers, enjoy.  And happy hunting.

# License

This software is licensed for use under the BSD license. If you make good use of this or any of my other tools, I'd appreciate an email letting me know what you used it for or how you stumbled across it.

We are using the callstack walker project from [Walking the Callstack](http://www.codeproject.com/Articles/11132/Walking-the-callstack) which is also BSD licensed.

I previously licensed this library under the tounge-in-cheek [WTFPL](http://en.wikipedia.org/wiki/WTFPL), but none of my lawyers thought it was funny.

   ---  Dave
