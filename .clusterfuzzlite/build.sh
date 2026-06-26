#!/bin/bash -eu
for src_file in $SRC/src/*.c; do
    $CC $CFLAGS -c -I$SRC/src "$src_file" -o "$(basename ${src_file%.c}.o)"
done
for fuzzer in $SRC/fuzz/*_fuzzer.cc; do
    name=$(basename ${fuzzer%.cc})
    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE "$fuzzer" *.o -I$SRC/src -lm -o $OUT/$name
done
for cd in $SRC/fuzz/corpus/*/; do
    [ -d "$cd" ] && [ "$(ls -A $cd 2>/dev/null)" ] && cp -r "$cd" "$OUT/$(basename $cd)_seed_corpus" || true
done
[ -f "$SRC/fuzz/corpus/dictionary.txt" ] && cp "$SRC/fuzz/corpus/dictionary.txt" "$OUT/" || true
