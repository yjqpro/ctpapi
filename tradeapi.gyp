﻿{
  'targets' : [
    {
      'variables': {
        'variables': {
          'conditions': [
            ['OS=="win" and target_arch=="x64"', {
              'lib_dir_for_target_arch%': 'win/x64',
            }, {
              'lib_dir_for_target_arch%': 'win/x86',
            },]
            ['OS=="linux"', {
              'lib_dir_for_target_arch%': 'linux',
            }]
          ]
        },
        'lib_dir_for_target_arch%': '<(lib_dir_for_target_arch)',
      },
      'target_name' : 'tradeapi',
      'type' : 'none',
      'variables' : {
      },
      'sources' : [
        '<!@(python <(DEPTH)/build/glob_files.py inlcude/tradeapi *.h)',
      ],
      'dependencies' : [
      ],
      'defines' : [
      ],
      'includes' : [
      ],
      'include_dirs' : [
      ],
      'link_settings': {
        'libraries': [
          '-lthosttraderapi',
          '-lthostmduserapi',
        ],
        'library_dirs': [
          'lib/<(lib_dir_for_target_arch)',
        ]
      },
    },
  ]
}
