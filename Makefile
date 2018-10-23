CC=gcc
CFLAGS=-std=gnu99 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -D_FILE_OFFSET_BITS=64
CFLAGSEXTRA=-s
OBJDIR=tmp
SRCS=$(wildcard src/*.c) $(wildcard src/filetype/*.c)  $(wildcard src/encoding/*.c)
OBJS=$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SRCS))))
TARGET=tippse

tmp/%.o: %.c
	@mkdir -p $(dir $@)
	@echo CC $<
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) -c $< -o $@

$(TARGET): $(OBJS)
	@echo LD $(TARGET)
	@$(CC) $(CFLAGS) $(CFLAGSEXTRA) $(OBJS) -o $(TARGET)

all: $(TARGET)

debug: CFLAGSEXTRA=-g
debug: $(TARGET)

minify: CFLAGSEXTRA=-s -flto
minify: $(TARGET)

perf: CFLAGSEXTRA=-D_PERFORMANCE
perf: $(TARGET)

clean:
	@rm -rf $(OBJDIR)

install:
	@cp tippse /usr/bin

depend:
	@makedepend -- $(CFLAGS) -- $(SRCS)
