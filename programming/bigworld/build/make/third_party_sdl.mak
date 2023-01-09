# This Makefile describes how to use the system SDL libraries

# useSDL

SDL_CONFIG_PATH = /usr/bin/sdl-config

SYSTEM_HAS_SDL := $(shell $(BW_BLDDIR)/test_sdl.sh && echo $$?)

SYSTEM_SDL_CFLAGS   := `$(SDL_CONFIG_PATH) --cflags`
SYSTEM_SDL_LDLIBS   := `$(SDL_CONFIG_PATH) --libs`

# third_party_sdl.mak
