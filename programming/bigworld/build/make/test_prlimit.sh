# Tests whether the system supports the prlimit function
# Exit code is 0 if we can.
(
gcc -c -o /dev/null -x c++ - << EOF
#include <sys/time.h>
#include <sys/resource.h>

void test() { prlimit( 0, RLIMIT_CORE, 0, 0 ); }
EOF
) 1>/dev/null 2>&1

# test_prlimit.sh
