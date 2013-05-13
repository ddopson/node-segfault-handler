{
  "targets": [
    {
      "target_name": "segfault_handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "defines": [ "DEBUG", "_DEBUG" ],
      # DDOPSON-2013-04-16 - Some versions of node-gyp don't seem to respect "cflags" (eg, my v0.8.2)
      # I hacked the module src to thart the over-optimization issue that previously required -O0, so we work even w/o the flag
      # However, the -O0 flag is still preferable for this module where debuggability matters and code size / perf are non-issues
      "cflags": [ "-O0" ],
    }
  ]
}

