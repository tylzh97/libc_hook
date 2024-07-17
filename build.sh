
rm -rf *.o *.so
gcc -g -ggdb -O0 -fPIC -c mymalloc.c -o mymalloc.o
gcc -shared -o libmymalloc.so mymalloc.o

LD_PRELOAD=./libmymalloc.so ls -a
# LD_PRELOAD=./libmymalloc.so ../../../Github/AFLplusplus/unicorn_mode/samples/python_sword/sword.linked.elf
