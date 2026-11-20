#!/bin/bash
# Q1 build: static, dynamic and run-time-loaded versions of the same program.
set -e
cd "$(dirname "$0")"
rm -f *.o *.a *.so q1_app_static q1_app_dynamic q1_dlopen q1_maps q1_hold q1_time
gcc -std=c11 -Wall -Wextra -fPIC -c enc.c -o enc.o
gcc -shared enc.o -o libenc.so
ar rcs libenc.a enc.o
gcc -std=c11 -Wall -Wextra -static q1_app.c libenc.a -o q1_app_static
gcc -std=c11 -Wall -Wextra q1_app.c -L. -lenc -Wl,-rpath,'$ORIGIN' -o q1_app_dynamic
gcc -std=c11 -Wall -Wextra q1_dlopen.c -ldl -o q1_dlopen
gcc -std=c11 -Wall -Wextra q1_lazy.c -ldl -o q1_lazy
gcc -std=c11 -Wall -Wextra q1_maps.c -L. -lenc -Wl,-rpath,'$ORIGIN' -o q1_maps
gcc -std=c11 -Wall -Wextra q1_hold.c -L. -lenc -Wl,-rpath,'$ORIGIN' -o q1_hold
gcc -std=c11 -Wall -Wextra q1_time.c -o q1_time
ls -l q1_app_static q1_app_dynamic q1_dlopen libenc.so
echo "--- file types ---"
file q1_app_static q1_app_dynamic
echo "--- ldd dynamic ---"
ldd ./q1_app_dynamic
echo "--- ldd static ---"
ldd ./q1_app_static || true
