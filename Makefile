CC_HOST_AUTO:=$(CC)
CC_HOST?=$(CC_HOST_AUTO)

ifeq ($(OS),emscripten)
	CC=emcc
	CFLAGS=-std=gnu11 -pthread -O3 -Oz -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_EMSCRIPTEN -D_FILE_OFFSET_BITS=64 -s WASM=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1
	LIBS=
	TARGET=tippse.js
else ifeq ($(OS),windows)
	CC=x86_64-w64-mingw32-gcc
	CFLAGS=-std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_WINDOWS -D_FILE_OFFSET_BITS=64
	LIBS=-lshell32 -lgdi32 -Wl,--subsystem,windows
	TARGET=tippse.exe
else
    ifeq ($(CC),tcc)
		CFLAGS=-std=gnu11 -O2 -pthread -Wall -Wextra -Wno-padded -Wno-shadow -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_ANSI_POSIX -D_FILE_OFFSET_BITS=64 -D_TINYC_
		LIBS=
		TARGET=tippse
	else
		CFLAGS=-std=gnu11 -O2 -pthread -Wall -Wextra -Wno-padded -Wno-shadow -Wno-unused-parameter -Wsign-conversion -fstrict-aliasing -D_ANSI_POSIX -D_FILE_OFFSET_BITS=64
        ifeq ($(shell uname -s),Darwin)
			LIBS=
		else
			LIBS=-latomic
		endif
		TARGET=tippse
	endif
endif

ifeq ($(CC),tcc)
	LDFLAGS=
else
	LDFLAGS=-Wl,-s
endif

CFLAGSEXTRA=
OBJDIR=tmp
SRCS_LIB=$(wildcard src/library/encoding/*.c) $(wildcard src/library/unicode/*.c) $(wildcard src/library/*.c)
SRCS_TESTLESS=$(wildcard src/filetype/*.c) $(SRCS_LIB)
SRCS=$(wildcard src/*.c) $(SRCS_TESTLESS)
OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS))))
DOCS=$(wildcard doc/*.md) LICENSE.md
COMPILED_DOCS=$(addprefix $(OBJDIR)/,$(addsuffix .h,$(basename $(DOCS))))
TESTS=$(addsuffix .output,$(addprefix $(OBJDIR)/,$(wildcard test/*)))
TEST_OBJDIR=$(addsuffix /test,$(OBJDIR))
TEST_TARGET=$(addprefix test_,$(TARGET))
TEST_SRCS=$(wildcard src/*.c)
TEST_OBJS=$(addprefix $(TEST_OBJDIR)/,$(addsuffix .o,$(basename $(TEST_SRCS))))
TEST_OBJS+=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS_TESTLESS))))
TEST_CFLAGSEXTRA=-D_TESTSUITE

src/editor.c: $(COMPILED_DOCS)
	@rm -rf tmp/editor.o

src/config.c: tmp/src/config.default.h
	@rm -rf tmp/config.o

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LIBS) -c $< -o $@

$(TEST_OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(TEST_CFLAGSEXTRA) $(LIBS) -c $< -o $@

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
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	@echo LD $(TEST_TARGET)
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(LDFLAGS) $(TEST_CFLAGSEXTRA) $(TEST_OBJS) $(LIBS) -o $(TEST_TARGET)

all: $(TARGET) $(TEST_TARGET)
	@echo OK

debug: CFLAGSEXTRA=-g -O0
debug: $(TARGET) $(TEST_TARGET)
	@echo OK

sanitize: CFLAGSEXTRA=-g -O0 -fno-omit-frame-pointer -fsanitize=address
sanitize: LDFLAGS=
sanitize: $(TARGET) $(TEST_TARGET)
	@echo OK

minify: CFLAGSEXTRA=-fno-unwind-tables -fno-asynchronous-unwind-tables -flto -Os -D__SMALLEST__
minify: $(TARGET) $(TEST_TARGET)
	@echo OK

perf: CFLAGSEXTRA=-D_PERFORMANCE
perf: $(TARGET) $(TEST_TARGET)
	@echo OK

freestanding: LDFLAGS+=-static
freestanding: CFLAGSEXTRA=-ffreestanding
freestanding: $(TARGET) $(TEST_TARGET)
	@echo OK

tmp/test/%.output: $(TEST_TARGET)
	@echo TS $(notdir $(basename $@))
	@mkdir -p tmp/test
	@./$(TEST_TARGET) test/$(notdir $(basename $@))/script test/$(notdir $(basename $@))/verify $@

test: $(TESTS)
	@echo OK

download-unicode:
	@mkdir -p tmp/tools/unicode/download
	@mkdir -p tmp/tools/unicode/output
	@wget "https://www.unicode.org/Public/13.0.0/ucd/UCD.zip" -O tmp/tools/unicode/download/UCD.zip
	@unzip tmp/tools/unicode/download/UCD.zip -d tmp/tools/unicode/download

generate-unicode: tmp/tools/convert
	@echo GN Unicode
	@cd tmp/tools/unicode && ../convert --unicode

clean:
	@rm -rf $(OBJDIR) $(TEST_OBJDIR) $(TARGET) $(TEST_TARGET)

install:
	@cp tippse /usr/bin

depend:
	@makedepend -- $(CFLAGS) -- $(SRCS)
