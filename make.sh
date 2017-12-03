gcc -std=gnu99 -O2 -flto -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -s -D_FILE_OFFSET_BITS=64 src/*.c src/filetype/*.c src/encoding/*.c -o tippse

