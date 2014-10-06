{
  "targets": [
    {
      "target_name": "segfault-handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "defines": [ "DEBUG", "_DEBUG" ],
      "cflags": [ "-O0", "-g" ],
      "xcode_settings": {
        "OTHER_CFLAGS": [ "-O0", "-g" ],
        "OTHER_LFLAGS": [ "-O0", "-g" ]
      },
      "include_dirs": [
               "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}

