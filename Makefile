ifeq ($(OS),windows)
	CC=i686-w64-mingw32-gcc
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_WINDOWS -D_FILE_OFFSET_BITS=64
	LIBS=-lshell32 -lgdi32 -Wl,--subsystem,windows
	TARGET=tippse.exe
else
	CC=gcc
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-padded -Wno-shadow -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_FILE_OFFSET_BITS=64
	LIBS=
	TARGET=tippse
endif

CFLAGSEXTRA=-s
OBJDIR=tmp
SRCS=$(wildcard src/lib/*.c) $(wildcard src/*.c) $(wildcard src/filetype/*.c) $(wildcard src/encoding/*.c)
OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS))))

tmp/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LIBS) -c $< -o $@

$(TARGET): $(OBJS)
	@echo LD $(TARGET)
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(OBJS) $(LIBS) -o $(TARGET)

all: $(TARGET)

debug: CFLAGSEXTRA=-g
debug: $(TARGET)

minify: CFLAGSEXTRA=-s -flto -Os -D__SMALLEST__
minify: $(TARGET)

perf: CFLAGSEXTRA=-D_PERFORMANCE
perf: $(TARGET)

clean:
	@rm -rf $(OBJDIR)

install:
	@cp tippse /usr/bin

depend:
	@makedepend -- $(CFLAGS) -- $(SRCS)
