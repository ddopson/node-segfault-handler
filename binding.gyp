{
  "targets": [
    {
      "target_name": "segfault-handler",
      "sources": [
        "src/segfault-handler.cpp"
      ],
      "conditions": [
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
