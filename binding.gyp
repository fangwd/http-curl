{
  'targets': [
    {
      'target_name': 'http-curl',
      'include_dirs': [ "<!(node -e \"require('nan')\")" ],
      'sources': [ 'lib/http_client.cc', 'lib/http_client_v8.cc', ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'ldflags': [ ],
      'conditions': [
        [
          'OS=="linux"',
           {
            'link_settings': {
              'libraries': [
                '-lcurl'
              ],
            }          
          }
        ],
      ],
    }
  ]
}
