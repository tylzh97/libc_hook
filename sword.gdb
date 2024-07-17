

# LD_PRELOAD=./libmymalloc.so ../../../Github/AFLplusplus/unicorn_mode/samples/python_sword/sword.linked.elf
target exec ../../../Github/AFLplusplus/unicorn_mode/samples/python_sword/sword.linked.elf
set environment LD_PRELOAD=./libmymalloc.so
