# =============================================================================
# Makefile for TreeViz (Linux, macOS, Windows MSYS2/MinGW)
#   make            build ./TreeViz (TreeViz.exe on Windows)
#   make run        build and start
#   make test       run the built-in invariant self-test
#   make clean
#   make web        browser build with Emscripten -> web/treeviz.js (open web/ via a local web server)
# =============================================================================
CXX      ?= g++
CXXFLAGS ?= -O2
CXXFLAGS += -std=c++17 -Wall -Wextra -Iinclude
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2)
SDL_LIBS   := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2)

ifeq ($(OS),Windows_NT)
    TARGET := TreeViz.exe
    # remove -mwindows from SDL libs if you want a console for printf output
else
    TARGET := TreeViz
endif

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(SDL_LIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) --selftest

clean:
	rm -rf build $(TARGET)

# WebAssembly build (needs the Emscripten SDK: https://emscripten.org)
web:
	em++ -std=c++17 -O2 -Wall -Wextra -Iinclude $(SRC) -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 \
	    -sSINGLE_FILE=1 -sSINGLE_FILE_BINARY_ENCODE=0 -sENVIRONMENT=web -o web/treeviz.js

.PHONY: all run test clean web
-include $(DEP)
