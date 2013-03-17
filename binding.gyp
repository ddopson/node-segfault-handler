{
  "targets": [
    {
      "target_name": "segvhandler",
      'include_dirs': [
      ],
      'conditions': [
        ['OS=="mac"', {
          'cflags_cc': [
            "-Wall",
            "-Werror",
            '-DDEBUG',
            '-O0',
            '-mmacosx-version-min=10.4'
          ],
          'link_settings': {
            'ldflags': [
              '-mmacosx-version-min=10.4'
            ]
          }
        }],
        ['OS!="mac"', {
          'cflags_cc': [
            "-Wall",
            '-DDEBUG',
            '-O0'
          ],
          'link_settings': {
            'ldflags': [
            ]
          }
        }]
      ],
      'cflags!': [
        '-fno-exceptions'
      ],
      'cflags_cc!': [
        '-fno-exceptions'
      ],
      "sources": [ 
        "src/segvhandler.cpp"
      ]
    }
  ]
}
 
