# Tests whether we can compile an SDL program. Exit code is 0 if we can.
sdl-config --cflags 1>/dev/null 2>&1 &&
(
gcc -E -x c++-header `sdl-config --cflags` - << EOF
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
EOF
) 1>/dev/null 2>&1

