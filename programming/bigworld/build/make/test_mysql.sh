# Tests whether we can compile a MySQL program. Exit code is 0 if we can.
mysql_config --include 1>/dev/null 2>&1 &&
(
gcc -E -x c++-header `mysql_config --include` - << EOF
#include <mysql/mysql.h>
EOF
) 1>/dev/null 2>&1

