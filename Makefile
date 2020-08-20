CC_HOST_AUTO:=$(CC)
CC_HOST?=$(CC_HOST_AUTO)

ifeq ($(OS),emscripten)
	CC=emcc
	CFLAGS=-std=gnu11 -O3 -Oz -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_EMSCRIPTEN -D_FILE_OFFSET_BITS=64 -s WASM=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1
	LIBS=
	TARGET=tippse.js
else ifeq ($(OS),windows)
	CC=i686-w64-mingw32-gcc
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_WINDOWS -D_FILE_OFFSET_BITS=64
	LIBS=-lshell32 -lgdi32 -Wl,--subsystem,windows
	TARGET=tippse.exe
else
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-padded -Wno-shadow -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_ANSI_POSIX -D_FILE_OFFSET_BITS=64
	LIBS=
	TARGET=tippse
endif

CFLAGSEXTRA=-s
OBJDIR=tmp
SRCS_LIB=$(wildcard src/library/encoding/*.c) $(wildcard src/library/unicode/*.c) $(wildcard src/library/*.c)
SRCS=$(wildcard src/*.c) $(wildcard src/filetype/*.c) $(SRCS_LIB)
OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS))))
DOCS=$(wildcard doc/*.md) LICENSE.md
COMPILED_DOCS=$(addprefix $(OBJDIR)/,$(addsuffix .h,$(basename $(DOCS))))
TESTS=$(addsuffix .output,$(addprefix $(OBJDIR)/,$(wildcard test/*)))

src/editor.c: $(COMPILED_DOCS)
	@rm -rf tmp/editor.o

src/config.c: tmp/src/config.default.h
	@rm -rf tmp/config.o

tmp/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LIBS) -c $< -o $@

tmp/tools/%.o: src/tools/%.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC_HOST) -c $< -o $@

tmp/tools/convert: tmp/tools/convert.o
	@echo LD $@
	@$(CC_HOST) $< -o $@

tmp/%.h: %.md tmp/tools/convert
	@echo MN $<
	@mkdir -p $(dir $@)
	@tmp/tools/convert --bin2c $< $@ file_$(notdir $(basename $@)) keep

tmp/src/config.default.h: src/config.default.txt tmp/tools/convert
	@echo CO $<
	@mkdir -p $(dir $@)
	@tmp/tools/convert --bin2c $< $@ file_$(subst .,_,$(notdir $(basename $@))) reduce

$(TARGET): $(OBJS)
	@echo LD $(TARGET)
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(OBJS) $(LIBS) -o $(TARGET)

all: $(TARGET)
	@echo OK

debug: CFLAGSEXTRA=-g -O0
debug: $(TARGET)
	@echo OK

sanitize: CFLAGSEXTRA=-g -O0 -fno-omit-frame-pointer -fsanitize=address
sanitize: $(TARGET)
	@echo OK

minify: CFLAGSEXTRA=-s -flto -Os -D__SMALLEST__
minify: $(TARGET)
	@echo OK

perf: CFLAGSEXTRA=-D_PERFORMANCE
perf: $(TARGET)
	@echo OK

tmp/test/%.output: $(TARGET)
	@echo TS $(notdir $(basename $@))
	@mkdir -p tmp/test
	@./$(TARGET) test/$(notdir $(basename $@))/script test/$(notdir $(basename $@))/verify $@

test: CFLAGSEXTRA=-D_TESTSUITE
test: $(TESTS)
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
