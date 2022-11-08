CC     := g++
CLANG  := clang++
CFLAGS := -Og -std=c++17 -Wall -Wextra

TARGET   := viewer
BUILDDIR := build
SRCDIR   := src
SOURCES  := main.cpp
OBJECTS  := $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:.cpp=.o)))

ifeq ($(OS),Windows_NT)
	MKDIR    := if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	RUN      :=
	PLATFORM := win32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
		MKDIR    := mkdir -p $(BUILDDIR)
		RUN      := ./
		PLATFORM := macos
    endif
endif

all: $(PLATFORM)

prepare:
	@$(MKDIR)

macos: prepare $(OBJECTS)
	@$(CLANG) -o $(TARGET) -framework Cocoa $(CFLAGS) platform/macos.mm $(OBJECTS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CC) $(CFLAGS) -o $@ -c $<

run: $(PLATFORM)
	@$(RUN)$(TARGET)

