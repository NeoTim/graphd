# Copyright 2015 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#!/bin/bash

B=`basename $0 .sh`
cd `dirname $0`
source ./rungraphd

rm -rf $D $B.pid

rungraphd -d${D} -p${B}.pid -bty << 'EOF'
write ("object" (-> "attribute" name="name" ("literal" value="my name")))
read (guid=00000012400034568000000000000009 (->()))
write (-> "attribute" left=00000012400034568000000000000009 guid~=0000001240003456800000000000000f ("literal" value="new name"))
read (guid="00000012400034568000000000000009" (->()))
EOF

rm -rf $D $B.pid
