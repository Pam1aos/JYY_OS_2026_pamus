# M4: C Read-Eval-Print-Loop (crepl) 实验要求整理

> 来源：<https://jyywiki.cn/OS/2026/labs/M4.md>
> 框架代码：`git pull origin M4` 获取的 `crepl/` 目录
> 整理日期：2026-11-05

---

## 1. 实验背景

REPL（Read–Eval–Print Loop）即在已加载运行时上下文的前提下，逐行读取代码、立即执行并打印结果。常见于 Python/Node 交互模式、调试器控制台、Jupyter Notebook 等。

本实验要求用 C 实现一个「C 语言 REPL」：

- 每一行代码在运行时被 **gcc 编译**；
- 编译产物被 **动态加载** 到当前进程的地址空间；
- 然后立即调用、得到结果。

这本质上是 JIT（Just-in-Time）思想的最小实践，也是理解「编译 vs 解释」「动态链接」的好机会。

---

## 2. 实验目标（功能要求）

`crepl` 从 **stdin 逐行**读入，根据每行内容分两种情况处理：

| 输入 | 处理方式 |
|---|---|
| 以 `int` 开头的行 | 视为**函数定义**，编译并加载到当前进程地址空间 |
| 其他行 | 视为 **int 表达式**，求值并输出结果 |

要求：

1. 只使用 `int` 类型：函数参数、返回值、表达式结果都是 `int`；
2. 读到表达式后**立即输出结果**，并 `fflush(stdout)` 确保结果立即可见；
3. 函数可以调用**之前定义过的函数**；表达式也可以调用之前定义过的函数；
4. 不允许访问全局状态（变量），不允许调用标准 C 库函数；
5. 非法输入（语法错误、未定义函数等）应给出错误提示，**不能让程序 crash**；
6. 重复定义同名函数视为 undefined behavior，测试不会覆盖；
7. 输入的命令/表达式数量不超过 100 个。

### 示例交互

```text
$ ./crepl
>> int gcd(int a, int b) { return b ? gcd(b, a % b) : a; }
OK.
>> gcd(256, 144) * gcd(56, 84)
= 448.
```

> 课程只要求「为每个函数或表达式输出一行」，参考实现的 `OK.` / `= 448.` 是推荐格式。

---

## 3. 正确性标准（课程原文要点）

1. 能正确解析**单行函数**（以 `int` 开头），其余输入按表达式处理；
2. 可能输入不合法的 C 代码，程序应给出错误提示而不是 crash；
3. 允许函数/表达式调用**之前定义过**的函数；
4. 输入数量 ≤ 100；
5. **运行目录可能没有写权限**：临时文件应放在 `/tmp/`，建议使用 `mkstemp` 系列 API；
6. 主进程必须真正求出所有表达式的值（不能把求值丢给外部进程后不取结果）。

### ⚠️ 禁止事项

- **禁止使用 `system()` 和 `popen()`**——使用了会导致编译错误；
- **允许** `exec` 族：`execl`、`execlp`、`execle`、`execv`、`execvp`、`execvpe`；
- 允许 `fork` + `exec` + `waitpid` + `pipe` 等系统调用。

---

## 4. 课程给出的实现路线

### 4.1 读取一行

```c
printf("crepl> ");
fflush(stdout);
fgets(line, sizeof(line), stdin);
```

行编辑/历史命令（上下键、Ctrl-P/Ctrl-N）是可选功能，不是实验要求。

### 4.2 把函数编译成共享库

1. 把函数定义写入临时文件，例如 `/tmp/xxxx.c`：

   ```c
   int gcd(int a, int b) { return b ? gcd(b, a % b) : a; }
   ```

2. 调用 gcc 编译成共享库：

   ```bash
   gcc -shared -fPIC -o /tmp/xxxx.so /tmp/xxxx.c
   ```

3. 用 `dlopen` / `dlsym` 加载并取得函数入口。

**函数调用之前定义的函数**时，有两种做法：

- 为之前定义过的函数生成函数声明（推荐、最正确）；
- 或者使用 `-Wno-implicit-function-declaration` 忽略隐式声明错误。

**关键**：新的共享库需要能解析旧函数符号，因此 `dlopen` 时要使用：

```c
dlopen(path, RTLD_NOW | RTLD_GLOBAL);
```

`RTLD_GLOBAL` 让已加载库的符号对后续加载的库可见。**不要在加载后 `dlclose`**，否则之前定义的函数会失效。

### 4.3 把表达式编译成共享库

表达式本身不能直接编译成可调用函数，课程建议用 **wrapper function**：

```c
int __expr_wrapper_4() {
    return gcd(256, 144) * gcd(56, 84);
}
```

- 每次求值生成不同的名字（如加一个自增计数器），避免重名冲突；
- 该 wrapper 与函数定义一样编译成 `.so`；
- `dlopen` + `dlsym("__expr_wrapper_N")`，按 `int (*)(void)` 调用即可得到值。

### 4.4 共享库加载

- Makefile 已经加了 `-ldl`，可以直接使用 `dlopen` / `dlsym` / `dlerror`；
- 需要处理失败情况：编译失败 → `compile_and_load_function` 返回 `false`；`dlopen`/`dlsym` 失败 → 返回 `false`；
- 建议把 gcc 的 stdout/stderr 重定向掉，避免污染 REPL 输出。

---

## 5. 额外拓展（页面中的「想玩点花活」）

以下内容**不是功能要求**，属于加分/挑战向的探索。

### 5.1 自己实现 ELF 动态加载器（替代 `dlopen`）

不调用 `dlopen`，而是：

1. 用 gcc 把输入编译成 **PIC 可重定位目标文件**（`.o`，ELF 类型 `ET_REL`）：

   ```bash
   gcc -c -fPIC -o /tmp/xxxx.o /tmp/xxxx.c
   ```

2. 解析 ELF 文件：
   - section header：`.text`、`.rodata`、`.data`、`.bss`；
   - 符号表：`.symtab`、`.strtab`；
   - 重定位表：`.rela.text`、`.rela.data` 等；
3. 用 `mmap` 申请一块可执行内存作为「加载镜像」；
4. 按 section 对齐把内容拷贝进去，`.bss` 对应区域清零（本实验可以直接给足够高的权限）；
5. **重定位与符号解析**：
   - 未定义符号 → 在 crepl 自己维护的全局符号表（函数名 → 入口地址）中查找；
   - 重点处理函数调用的重定位（x86-64 常见 `R_X86_64_PLT32` / `R_X86_64_PC32` 等）；
6. 找到入口函数符号地址，注册到全局符号表，供后续函数/表达式使用。

完成后相当于实现了一个「迷你动态链接器 / JIT 加载器」。

### 5.2 其他可选扩展

- **语法高亮**：参考实现调用外部程序实现；必须做**运行时存在性检查**，不存在则输出无高亮版本，否则会 Compile Error / Wrong Answer；
- **行编辑与历史**：上下键浏览历史、Ctrl-P/Ctrl-N 等；
- 更完善的错误提示、表达式美化输出等。

> 注意：这些「花活」不是实验验收标准，先保证基础功能与测试全绿。

---

## 6. 测试与验收（`crepl/tests.c`）

框架里的 `tests.c` 已经给出了 9 个 `UnitTest`，直接测试两个核心函数：

```c
bool compile_and_load_function(const char* function_def);
bool evaluate_expression(const char* expression, int* result);
```

### 测试清单

| 测试 | 输入 | 期望 |
|---|---|---|
| `test_compile_valid_function` | `int test_func() { return 42; }` | 返回 `true` |
| `test_compile_function_using_previous` | 先定义 `test_func`，再定义 `int test_func2() { return test_func(); }` | 返回 `true` |
| `test_compile_invalid_syntax` | `int invalid_func() { return 42` | 返回 `false` |
| `test_evaluate_simple_constant` | `42` | `true`，结果 `42` |
| `test_evaluate_arithmetic` | `21 + 21` | `true`，结果 `42` |
| `test_evaluate_function_call` | 先定义 `int test_eval() { return 100; }`，再求值 `test_eval()` | `true`，结果 `100` |
| `test_evaluate_complex_expression` | 先定义 `test_eval`，再求值 `test_eval() / 2` | `true`，结果 `50` |
| `test_evaluate_undefined_function` | `undefined_function()` | 返回 `false` |
| `test_evaluate_syntax_error` | `21 +` | 返回 `false` |

### 如何运行测试

```bash
cd os2026/crepl
make all

# 注意：crepl 的 main 会从 stdin 读行，必须给 EOF，否则测试永远不开始
TK_RUN=1 ./crepl < /dev/null
TK_VERBOSE=1 ./crepl < /dev/null
```

> TestKit 的工作方式：先正常执行 `main()`，`main` 退出后才运行测试。
> `crepl` 的 `main` 默认阻塞在 `fgets(stdin)` 上，**不重定向 stdin 会导致测试卡住**。
> 用 `< /dev/null` 让 `fgets` 立刻读到 EOF，`main` 返回后再跑测试。

### 验收要点

- 9 个单元测试全部 PASS；
- 手工运行 `./crepl`，能正确完成上面的 `gcd` 示例；
- 非法输入不会 crash；
- 临时文件都创建在 `/tmp` 下。

---

## 7. 建议的实现步骤

1. **读行**：`fgets` + 去掉行尾 `\n`；
2. **分类**：跳过前导空白，判断是否以 `int ` 开头；
3. **函数分支**：
   - 写入 `/tmp` 临时 `.c` 文件（`mkstemp`）；
   - `fork` + `execvp("gcc", ...)` 编译成 `.so`；
   - 检查 gcc 退出码，失败返回 `false`；
   - `dlopen(path, RTLD_NOW | RTLD_GLOBAL)`；
   - 从符号表/`dlsym` 确认函数存在；
   - 保存 handle（不要立即 `dlclose`）；
4. **表达式分支**：
   - 生成唯一的 wrapper 函数名；
   - 写入 `int __expr_wrapper_N() { return (<expression>); }`；
   - 编译、`dlopen`、`dlsym`、按 `int (*)(void)` 调用；
   - 结果写入 `*result`，返回 `true`；
5. **错误处理**：任何一步失败都返回 `false`，并清理临时文件；
6. **main 循环**：对每行调用上述函数，输出 `OK.` 或 `= 值.`，表达式后 `fflush(stdout)`。

---

## 8. 常见坑

| 坑 | 说明 |
|---|---|
| 使用 `system()` / `popen()` | 直接编译失败，必须用 `fork` + `exec` |
| 在不可写目录创建临时文件 | 一律用 `/tmp` + `mkstemp` |
| 忘记 `RTLD_GLOBAL` | 新函数无法解析旧函数符号，`dlopen` 会失败 |
| 过早 `dlclose` | 旧函数符号失效，后续调用/加载出错 |
| wrapper 重名 | 每次表达式都用唯一名字（计数器） |
| 表达式未加括号 | `int __expr_wrapper_N() { return a, b; }` 会改变语义，应写 `return (expr);` |
| gcc 输出污染 REPL | 把 gcc 的 stdout/stderr 重定向到 `/dev/null` 或临时文件 |
| 函数间调用出现隐式声明错误 | 生成函数原型，或用 `-Wno-implicit-function-declaration` |
| 测试卡住 | 运行测试时必须 `< /dev/null` 给 `main` 一个 EOF |
| 忽略 gcc 退出码 | 编译失败时 `compile_and_load_function` 必须返回 `false` |
| 临时文件泄漏 | `unlink` 临时 `.c` / `.so`（注意 `.so` 被 dlopen 后可以 unlink） |

---

## 9. 提交

```bash
cd os2026/crepl
make submit
```

> 提交前确认 `oslab.mk` 中的 `TOKEN` 已配置。
> 页面显示的 Soft Deadline 为 2026-05-10 23:59:59，请以课程页面最新信息为准。
