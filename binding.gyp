{
  "targets": [
    {
      "target_name": "segfault_handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "defines": [ "DEBUG", "_DEBUG" ],
      "cflags": [ "-O0" ],
      "xcode_settings": {
        "OTHER_CFLAGS": [ "-O0" ]
      }
    }
  ]
}

