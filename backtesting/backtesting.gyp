﻿{
  'includes':[
    '../build/win_precompile.gypi',
  ],
  'targets' : [
    {
      'target_name' : 'backtesting',
      'type' : 'executable',
      'variables' : {
      },
      'sources' : [
        'main.cc',
      ],
      'dependencies' : [
        '<(DEPTH)/third_party/actor-framework/libcaf_io/libcaf_io.gyp:*',
        '../hpt_core/hpt_core.gyp:hpt_core',
        '../strategies/strategies.gyp:strategies',
        '../follow_strategy/follow_strategy.gyp:follow_strategy',
      ],
      'defines' : [
      ],
      'includes' : [
      ],
      'include_dirs' : [
        '..',
      ],

      },
  ]
}
