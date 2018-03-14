i686-w64-mingw32-gcc -std=gnu99 -O2 -Wall -Wextra -Wno-unused-parameter -Wsign-conversion -s -D_WINDOWS -D_FILE_OFFSET_BITS=64 src/*.c src/filetype/*.c src/encoding/*.c -lshell32 -lgdi32 -o tippse.exe

