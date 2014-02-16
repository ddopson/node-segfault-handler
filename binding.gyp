{
  "targets": [
    {
      "target_name": "segfault_handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "defines": [ "DEBUG", "_DEBUG" ],
      "cflags": [ "-O0", "-g" ],
      "xcode_settings": {
        "OTHER_CFLAGS": [ "-O0", "-g"],
        "OTHER_LFLAGS": [ "-O0", "-g"]
      }
    }
  ]
}

