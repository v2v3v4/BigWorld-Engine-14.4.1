# Tests whether we can compile a Mongo DB program. Exit code is 0 if we can.
(
gcc -E -x c++-header - << EOF
#include <mongo/client/dbclient.h>
EOF
) 1>/dev/null 2>&1

