CC_HOST:=$(CC)

ifeq ($(OS),windows)
	CC=i686-w64-mingw32-gcc
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_WINDOWS -D_FILE_OFFSET_BITS=64
	LIBS=-lshell32 -lgdi32 -Wl,--subsystem,windows
	TARGET=tippse.exe
else
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-padded -Wno-shadow -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_FILE_OFFSET_BITS=64
	LIBS=
	TARGET=tippse
endif

CFLAGSEXTRA=-s
OBJDIR=tmp
SRCS=$(wildcard src/lib/*.c) $(wildcard src/*.c) $(wildcard src/filetype/*.c) $(wildcard src/encoding/*.c)
OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS))))
DOCS=$(wildcard doc/*.md) LICENSE.md
COMPILED_DOCS=$(addprefix $(OBJDIR)/,$(addsuffix .h,$(basename $(DOCS))))

src/editor.c: $(COMPILED_DOCS)
	@rm -rf tmp/editor.o

tmp/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LIBS) -c $< -o $@

tmp/tools/convert.o: src/tools/convert.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC_HOST) -c $< -o $@

tmp/tools/convert: tmp/tools/convert.o
	@echo LD $@
	@$(CC_HOST) $< -o $@

tmp/%.h: %.md tmp/tools/convert
	@echo MN $<
	@mkdir -p $(dir $@)
	@tmp/tools/convert --bin2c $< $@ file_$(notdir $(basename $@))

$(TARGET): $(OBJS)
	@echo LD $(TARGET)
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(OBJS) $(LIBS) -o $(TARGET)

all: $(TARGET)
	@echo OK

debug: CFLAGSEXTRA=-g
debug: $(TARGET)
	@echo OK

minify: CFLAGSEXTRA=-s -flto -Os -D__SMALLEST__
minify: $(TARGET)
	@echo OK

perf: CFLAGSEXTRA=-D_PERFORMANCE
perf: $(TARGET)
	@echo OK

download-unicode:
	@mkdir -p tmp/tools/unicode/download
	@mkdir -p tmp/tools/unicode/output
	@wget "https://www.unicode.org/Public/10.0.0/ucd/UCD.zip" -O tmp/tools/unicode/download/UCD.zip
	@unzip tmp/tools/unicode/download/UCD.zip -d tmp/tools/unicode/download

generate-unicode: tmp/tools/convert
	@echo GN Unicode
	@cd tmp/tools/unicode && ../convert --unicode

clean:
	@rm -rf $(OBJDIR)

install:
	@cp tippse /usr/bin

depend:
	@makedepend -- $(CFLAGS) -- $(SRCS)
