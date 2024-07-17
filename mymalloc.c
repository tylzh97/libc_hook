// https://stackoverflow.com/a/6083624


#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>

static FILE* fd_output = NULL;
static size_t isWrapperInit = 0;

static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static size_t (*real_sleep)(size_t) = NULL;
static size_t (*real_read)(int, void*, size_t) = NULL;
static void (*real_exit)(int) = NULL;
static unsigned int (*real_alarm)(unsigned int) = NULL;

#define LOAD_SYMBOL(name)                                                   \
  ({                                                                        \
    void* sym = dlsym(RTLD_NEXT, name);                                     \
    if (NULL == sym) {                                                      \
      fprintf(fd_output, "Error in `dlsym` for %s: %s\n", name, dlerror()); \
    }                                                                       \
    sym;                                                                    \
  })

#define CHECK_MTRACE_INIT() \
  ({                        \
    if (0 == isWrapperInit) \
      mtrace_init();        \
  })

static void mtrace_init(void) {
  fd_output = stderr;

  //   real_malloc = dlsym(RTLD_NEXT, "malloc");
  //   if (NULL == real_malloc) {
  //     fprintf(fd_output, "Error in `dlsym`: %s\n", dlerror());
  //   }
  real_malloc = LOAD_SYMBOL("malloc");
  real_free = LOAD_SYMBOL("free");
  real_sleep = LOAD_SYMBOL("sleep");
  real_read = LOAD_SYMBOL("read");
  real_exit = LOAD_SYMBOL("exit");
  real_alarm = LOAD_SYMBOL("alarm");

  isWrapperInit = 1;
}

// 全局变量来保存 argc 和 argv
int g_argc;
char** g_argv;

// 定义 __libc_start_main 函数原型
typedef int (*libc_start_main_fn)(int (*main)(int, char**, char**),
                                  int argc,
                                  char** ubp_av,
                                  void (*init)(void),
                                  void (*fini)(void),
                                  void (*rtld_fini)(void),
                                  void(*stack_end));

// 我们自己的 __libc_start_main 实现
int __libc_start_main(int (*main)(int, char**, char**),
                      int argc,
                      char** ubp_av,
                      void (*init)(void),
                      void (*fini)(void),
                      void (*rtld_fini)(void),
                      void(*stack_end)) {
  CHECK_MTRACE_INIT();

  // 保存 argc 和 argv
  g_argc = argc;
  g_argv = ubp_av;

  fprintf(fd_output, "[HOOK]: main() argc: %d argv[][]: %p. \n", g_argc, g_argv);
  for (int i = 0; i < argc; ++i) {
    fprintf(fd_output, "==> [index:%02d] %s \n", i, g_argv[i]);
  }

  // 获取原始的 __libc_start_main 函数
  libc_start_main_fn orig_libc_start_main =
      (libc_start_main_fn)dlsym(RTLD_NEXT, "__libc_start_main");
  if (!orig_libc_start_main) {
    fprintf(fd_output, "Error: unable to find original __libc_start_main\n");
    real_exit(-1);
  }

  // 调用原始的 __libc_start_main 函数
  return orig_libc_start_main(main, argc, ubp_av, init, fini, rtld_fini,
                              stack_end);
}

void* malloc(size_t size) {
  CHECK_MTRACE_INIT();

  void* p = NULL;
  fprintf(fd_output, "malloc(%ld) = ", size);
  p = real_malloc(size);
  fprintf(fd_output, "%p\n", p);
  return p;
}

void free(void* __address) {
  CHECK_MTRACE_INIT();

  fprintf(fd_output, "[HOOK]: free -- %p \n", __address);
  real_free(__address);
//   if (NULL == __address) {
//     // gdb debug line
//     printf("%d \n", 0);
//   }
}

size_t sleep(size_t __seconds) {
  CHECK_MTRACE_INIT();

  fprintf(fd_output, "[HOOK]: sleep -- for %ld seconds. \n", __seconds);
  return 0;
}

size_t read(int fd, void* buf, size_t count) {
  CHECK_MTRACE_INIT();

  fprintf(fd_output, "[HOOK]: read(%p) -- fd: %d, buf: %p, count: %ld   \n",
          real_read, fd, buf, count);
  return real_read(fd, buf, count);
}

void exit(int __status) {
  // void exit(int status);
  fprintf(fd_output, "[HOOK]: exit -- with code %d. \n", __status);
  real_exit(__status);
}

unsigned int alarm(unsigned int __seconds) {
  // unsigned int alarm(unsigned int seconds);
  fprintf(fd_output, "[HOOK]: alarm -- Will send SIGALRM after %u seconds. \n",
          __seconds);
  return 0;
}
