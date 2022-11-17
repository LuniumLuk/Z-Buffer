#### MAKEFILE
#### Generated by myself.

#### Currently support two platforms:
####  - MacOS
####  - Windows

## Compiler settings.
CC     := g++
CLANG  := clang++
CFLAGS := -std=c++17 $(EXTRA) -O3 # -Og -Wall -Wextra
## Basic settings.
TARGET   := viewer
BUILDDIR := build
ICDDIR   := include
SRCDIR   := src
## Add your *.cpp file here, make sure they are under SRCDIR folder.
SOURCES  := $(wildcard $(addprefix $(SRCDIR)/, *.cpp))
INCLUDES := $(wildcard $(addprefix $(ICDDIR)/, *.h))
OBJECTS  := $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:.cpp=.o)))

ifeq ($(OS),Windows_NT)
	MKDIR    := if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	RUN      :=
	PLATFORM := win32
	CLEAN    := if exist $(BUILDDIR) rmdir /s /q $(BUILDDIR)
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
		MKDIR    := mkdir -p $(BUILDDIR)
		RUN      := ./
		PLATFORM := macos
		CLEAN    := rm -rf $(BUILDDIR)
    endif
endif

all: $(PLATFORM)

prepare:
	@$(MKDIR)

macos: prepare $(OBJECTS)
	@$(CLANG) -o $(TARGET) -framework Cocoa $(CFLAGS) platform/macos.mm $(OBJECTS)

win32: prepare $(OBJECTS)
	@$(CC) -o $(TARGET).exe $(CFLAGS) platform/win32.cpp $(OBJECTS) -lgdi32

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(INCLUDES)
	@$(CC) $(CFLAGS) -I$(ICDDIR) -o $@ -c $<

clean:
	@$(CLEAN)

run: $(PLATFORM)
	@$(RUN)$(TARGET)

