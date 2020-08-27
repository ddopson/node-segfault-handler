{
  "variables": {
    "BUILD_WITH_LIBEXEC": "<!(python -c 'from ctypes.util import find_library;print int(find_library(\"execinfo\")!=None)')",
  },
  "targets": [
    {
      "target_name": "segfault-handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "conditions": [
        [ "<(BUILD_WITH_LIBEXEC)==1", {
          "libraries": [
            "-lexecinfo"
          ]
        }],
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "DisableSpecificWarnings": ["4996"]
            }
          },
          "sources": [
            "src/StackWalker.cpp",
            "includes/StackWalker.h"
          ]
        }],
        ["OS == 'linux'", {
          "cflags_cc": [
            "-Wno-cast-function-type" # nan issue: https://github.com/nodejs/nan/issues/807
          ]
        }]
      ],
      "cflags": [ "-O0", "-funwind-tables" ],
      "xcode_settings": {
        "MACOSX_DEPLOYMENT_TARGET": "10.9",
        "OTHER_CFLAGS": [ "-O0", "-funwind-tables" ],
        "CLANG_CXX_LIBRARY": "libc++"
      },
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
