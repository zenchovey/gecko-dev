# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
{
  'includes': [
    '../../../coreconf/config.gypi'
  ],
  'targets': [
    {
      'target_name': 'lib_dbm_include_exports',
      'type': 'none',
      'copies': [
        {
          'files': [
            'mcom_db.h',
            'ncompat.h',
            'winfile.h'
          ],
          'destination': '<(nss_dist_dir)/public/<(module)'
        },
        {
          'files': [
            'extern.h',
            'hash.h',
            'hsearch.h',
            'page.h',
            'queue.h',
            'search.h'
          ],
          'destination': '<(nss_dist_dir)/private/<(module)'
        }
      ]
    }
  ],
  'variables': {
    'module': 'dbm'
  }
}
