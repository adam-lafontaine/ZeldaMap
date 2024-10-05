GPP := g++

GPP += -std=c++20
GPP += -mavx
GPP += -O3
GPP += -DNDEBUG
GPP += -mwindows

# directories
SDL := C:\SDL2\SDL2-2.30.5\x86_64-w64-mingw32

# include directory flags
ISDL := -I$(SDL)\include -I$(SDL)\include\SDL2

# link directory flags
LDSDL := -L$(SDL)\lib

# dll files
SDL_DLL := $(SDL)/bin/SDL2.dll

# flags
IFLAGS := $(ISDL)

LSDL := $(LDSDL) -lmingw32 -lsdl2 -lSDL2main

ALL_LFLAGS := $(LSDL)


root := ../../..

build := $(root)/build/win

src := $(root)/src
libs := $(root)/libs

exe := zelda_map.exe

program_exe := $(build)/$(exe)

main_c := $(src)/zelda_map_main.cpp


#**************


$(program_exe): $(main_c)
	@echo "  program_exe"
	$(GPP) $(IFLAGS) -o $@ $+ $(ALL_LFLAGS)
	cp $(SDL_DLL) $(build)


build: $(program_exe)


run: build
	$(program_exe)
	@echo "\n"


clean:
	rm -fv $(build)/*


setup:
	mkdir -p $(build)