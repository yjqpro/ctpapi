﻿{
  'targets' : [
    {
      'target_name' : 'ctpapi',
      'type' : 'none',
      'variables': {
        'variables': {
          'conditions': [
            ['OS=="win" and target_arch=="x64"', {
              'lib_dir_for_target_arch%': 'win/x64',
            }, {
              'lib_dir_for_target_arch%': 'win/x86',
            },],
            ['OS=="linux"', {
              'lib_dir_for_target_arch%': 'linux',
            }],
          ]
        },
        'lib_dir_for_target_arch%': '<(lib_dir_for_target_arch)',
      },
      'sources' : [
        '<!@(python <(DEPTH)/build/glob_files.py inlcude/ctpapi *.h)',
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
      'copies': [
        {
          'destination': '<(PRODUCT_DIR)',
          'files': [
            'lib/<(lib_dir_for_target_arch)/thosttraderapi.dll',
            'lib/<(lib_dir_for_target_arch)/thostmduserapi.dll'
          ],
        },
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'include',
        ],
      },
    },
  ]
}
