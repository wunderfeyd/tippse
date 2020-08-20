# -O0 -g -fsanitize=address
gcc -O2 src/library/*.c src/library/encoding/*.c src/tools/dedup.c -o dedup

