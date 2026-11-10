/*
 * crepl.c —— 解耦重构版（参考副本）
 *
 * 说明：
 *   - 本文件放在 refactor/ 子目录下，Makefile 的 SRCS 只收集
 *     `find . -maxdepth 1 -name "*.c"`，因此不会参与 `make` 构建，
 *     也不会和上层的 crepl.c / tests.c 产生 main / 符号冲突。
 *   - 逻辑与上层 crepl.c 完全一致，只做结构上的拆分：
 *
 *       1. 临时文件层：创建 / 写入 / 清理，统一由 TempFile 管理
 *       2. 进程执行层：run_gcc() 封装 fork + exec + wait
 *       3. 动态加载层：load_symbol() 封装 dlopen + dlsym
 *       4. 业务层：compile_and_load_function / evaluate_expression
 *       5. 交互层：repl() 只负责读行、分类、打印
 *
 *   - 重复的 `unlink(path); unlink(so_path);` 用 CLEANUP_TEMPS 宏
 *     统一收口；每个函数用 `goto done` 保证只有一条清理路径。
 */

#define _GNU_SOURCE   /* mkstemps */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

#define TEMP_PATH_SIZE  64
#define NAME_BUF_SIZE   128
#define SRC_BUF_SIZE    1024
#define LINE_BUF_SIZE   256

/* ======================================================================
 * 1. 临时文件层
 * ====================================================================== */

/* 创建唯一临时文件：模板 /tmp/crepl-XXXXXX<suffix>，返回 fd（失败 -1）。 */
static int make_temp_file(char *path, size_t size, const char *suffix)
{
    int n = snprintf(path, size, "/tmp/crepl-XXXXXX%s", suffix);
    if (n < 0 || (size_t)n >= size) {
        path[0] = '\0';
        return -1;
    }

    int fd = mkstemps(path, (int)strlen(suffix));
    if (fd < 0)
        path[0] = '\0';   /* 失败时清空，便于清理函数判断 */
    return fd;
}

/* 完整写入，处理 write() 被信号打断或只写一部分的情况。 */
static bool write_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;
    while (off < len) {
        ssize_t n = write(fd, buf + off, len - off);
        if (n <= 0)
            return false;
        off += (size_t)n;
    }
    return true;
}

/* 删除一个临时文件；清空路径，保证重复调用是安全的。 */
static void discard_temp_file(char *path)
{
    if (path[0] == '\0')
        return;
    unlink(path);
    path[0] = '\0';
}

/*
 * 所有函数统一用这个宏收尾，替代散落在各处的
 *     unlink(path); unlink(so_path);
 */
#define CLEANUP_TEMPS(c_path, so_path)  \
    do {                                \
        discard_temp_file(c_path);      \
        discard_temp_file(so_path);     \
    } while (0)

/* ======================================================================
 * 2. 进程执行层
 * ====================================================================== */

/* fork + execvp("gcc", ...) + waitpid，返回 gcc 是否成功退出。 */
static bool run_gcc(const char *c_path, const char *so_path)
{
    pid_t pid = fork();
    if (pid < 0)
        return false;

    if (pid == 0) {
        /* gcc 的诊断信息不进入 REPL / 测试输出 */
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        char *args[] = {
            "gcc", "-shared", "-fPIC",
            "-Wno-implicit-function-declaration",
            "-x", "c",
            "-o", (char *)so_path,
            (char *)c_path,
            NULL
        };
        execvp("gcc", args);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        return false;

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/* ======================================================================
 * 3. 动态加载层
 * ====================================================================== */

/*
 * 加载 .so 并查找符号。
 * 注意：故意不 dlclose —— 之前定义的函数符号必须保留在全局作用域里，
 * 后续表达式才能解析到它们。
 */
static void *load_symbol(const char *so_path, const char *symbol)
{
    void *handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return NULL;
    }

    dlerror();                     /* 清除可能存在的旧错误 */
    void *sym = dlsym(handle, symbol);
    if (dlerror() != NULL)
        return NULL;

    return sym;
}

/* ======================================================================
 * 4. 源码解析辅助
 * ====================================================================== */

static const char *skip_space(const char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

/*
 * 判断一行是否是函数定义；若是，返回 "int" 关键字之后的内容。
 * 统一处理 "int ", "int(" 以及前导空白。
 */
static const char *after_int_keyword(const char *line)
{
    const char *p = skip_space(line);
    if (strncmp(p, "int", 3) != 0)
        return NULL;

    char c = p[3];
    if (c != '\0' && !isspace((unsigned char)c) && c != '(')
        return NULL;

    return skip_space(p + 3);
}

static bool is_function_definition(const char *line)
{
    return after_int_keyword(line) != NULL;
}

/* 从 "int gcd(int a, int b) {...}" 中取出 "gcd"。 */
static bool extract_function_name(const char *def, char *name, size_t size)
{
    const char *p = after_int_keyword(def);
    if (!p)
        return false;

    const char *start = p;
    while (isalnum((unsigned char)*p) || *p == '_')
        p++;

    size_t n = (size_t)(p - start);
    if (n == 0 || n >= size)
        return false;

    snprintf(name, size, "%.*s", (int)n, start);
    return true;
}

/* ======================================================================
 * 5. 业务层
 * ====================================================================== */

bool compile_and_load_function(const char *function_def)
{
    char c_path[TEMP_PATH_SIZE] = "";
    char so_path[TEMP_PATH_SIZE] = "";
    bool ok = false;
    int fd;

    fd = make_temp_file(c_path, sizeof c_path, ".c");
    if (fd < 0)
        goto done;

    if (!write_all(fd, function_def, strlen(function_def))) {
        close(fd);
        goto done;
    }
    close(fd);

    fd = make_temp_file(so_path, sizeof so_path, ".so");
    if (fd < 0)
        goto done;
    close(fd);

    if (!run_gcc(c_path, so_path))
        goto done;

    char func_name[NAME_BUF_SIZE];
    if (!extract_function_name(function_def, func_name, sizeof func_name))
        goto done;

    ok = load_symbol(so_path, func_name) != NULL;

done:
    CLEANUP_TEMPS(c_path, so_path);
    return ok;
}

/* 每个表达式使用唯一的 wrapper 名字，避免符号冲突。 */
static void next_wrapper_name(char *buf, size_t size)
{
    static int counter = 0;
    snprintf(buf, size, "__expr_wrapper_%d", counter++);
}

/* 把表达式包进 "int wrapper(void) { return (expr); }"。 */
static bool build_wrapper_source(const char *expression,
                                 const char *wrapper,
                                 char *src, size_t size)
{
    int n = snprintf(src, size,
                     "int %s(void) { return (%s); }\n",
                     wrapper, expression);
    return n > 0 && (size_t)n < size;
}

bool evaluate_expression(const char *expression, int *result)
{
    char wrapper[NAME_BUF_SIZE];
    char src[SRC_BUF_SIZE];

    next_wrapper_name(wrapper, sizeof wrapper);
    if (!build_wrapper_source(expression, wrapper, src, sizeof src))
        return false;

    char c_path[TEMP_PATH_SIZE] = "";
    char so_path[TEMP_PATH_SIZE] = "";
    bool ok = false;
    int fd;

    fd = make_temp_file(c_path, sizeof c_path, ".c");
    if (fd < 0)
        goto done;

    if (!write_all(fd, src, strlen(src))) {
        close(fd);
        goto done;
    }
    close(fd);

    fd = make_temp_file(so_path, sizeof so_path, ".so");
    if (fd < 0)
        goto done;
    close(fd);

    if (!run_gcc(c_path, so_path))
        goto done;

    int (*fn)(void) = (int (*)(void))load_symbol(so_path, wrapper);
    if (!fn)
        goto done;

    *result = fn();
    ok = true;

done:
    CLEANUP_TEMPS(c_path, so_path);
    return ok;
}

/* ======================================================================
 * 6. 交互层
 * ====================================================================== */

static void handle_line(char *line)
{
    if (is_function_definition(line)) {
        if (compile_and_load_function(line))
            printf("OK.\n");
        else
            fprintf(stderr, "error\n");
    } else {
        int value;
        if (evaluate_expression(line, &value))
            printf("= %d.\n", value);
        else
            fprintf(stderr, "error\n");
    }
}

static void repl(void)
{
    char line[LINE_BUF_SIZE];

    for (;;) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof line, stdin))
            break;

        /* 先去换行，再判断空行 */
        line[strcspn(line, "\r\n")] = '\0';
        if (*skip_space(line) == '\0')
            continue;

        handle_line(line);
    }
}

int main(void)
{
    repl();
    return 0;
}
