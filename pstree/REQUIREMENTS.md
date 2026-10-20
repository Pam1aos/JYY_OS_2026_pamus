# M2 `pstree` 测试需求拆解

> 依据：`pstree/tests.c`（9 个系统测试）+ TestKit 运行机制 + `pstree.c` 初始实现。
> 目的：把「测试实际检查什么」「隐含但未检查什么」「当前实现的差距」分开列清楚。

---

## 1. 测试环境约束（TestKit）

- 每个系统测试都会 **fork 一个子进程，直接调用 `main(argc, argv)`**（不经过 `exec`）。
- `argv[0]` 是 `"./pstree"`，但文件路径基于**运行测试时的当前目录**（必须在 `pstree/` 里跑）。
- `result->output` 捕获的是 **stdout + stderr**（通过 `printf` / `fprintf` 写出）。
- 每个用例限时 **5 秒**，超时判 FAIL。
- `main` 的**退出码**只在测试显式断言时才重要。
- 测试之间是独立进程，但文件状态会保留（`init`/`fini` 负责清理）。
- `assert` 被 TestKit 重定义为失败即 `abort()`，不适合用来做输入校验。

---

## 2. 逐条测试拆解

| # | 测试名 | 命令 | 断言 | 对应需求 |
|---|---|---|---|---|
| 1 | `basic_no_args` | `./pstree` | exit 0；输出非空 | 无参数时正常运行并输出进程信息 |
| 2 | `show_pids_short` | `./pstree -p` | exit 0；非空；输出含 `(` | 支持短选项 `-p` |
| 3 | `show_pids_long` | `./pstree --show-pids` | 同上 | 支持长选项 `--show-pids` |
| 4 | `numeric_sort_short` | `./pstree -n` | exit 0；非空 | 支持短选项 `-n` |
| 5 | `numeric_sort_long` | `./pstree --numeric-sort` | 同上 | 支持长选项 `--numeric-sort` |
| 6 | `version_short` | `./pstree -V` | exit 0；非空；含 `pstree` | 版本输出，字符串必须包含小写 `pstree` |
| 7 | `version_long` | `./pstree --version` | 同上 | 长选项版本 |
| 8 | `show_pids_and_numeric_sort` | `./pstree -p -n` | exit 0；非空；含 `(` | 短选项可以组合 |
| 9 | `all_options_long` | `./pstree --show-pids --numeric-sort` | 同上 | 长选项可以组合 |
| 10 | `invalid_option` | `./pstree --invalid-option` | **exit ≠ 0**；输出含 `usage`/`Usage`/`invalid`/`Invalid` 之一 | 非法参数要报错并给出提示 |

> 注：`tests.c` 实际有 10 个 `SystemTest`，没有 `UnitTest`。

---

## 3. 关键细节

### 3.1 关于 `-p`

测试只检查输出里有没有 `(`，**没有**检查「不加 `-p` 时不能有 PID」。但正确语义应该是：

- 默认输出：`bash───pstree`（只有名字）；
- 加 `-p`：`bash(1234)───pstree(5678)`。

> 当前初始实现**总是**打印 `名字(pid)`，所以 `-p` 相关用例会“碰巧通过”，但不符合选项语义。

### 3.2 关于 `-n`

测试**完全没有验证排序结果**，只要求 exit 0 且输出非空。实验意图通常是：

- 默认：同一层的兄弟进程按**名字**排序（`strcmp`）；
- `-n`：同一层的兄弟进程按 **PID 数值**排序。

即使不实现排序也能过测试，但评测可能有隐藏用例，建议按语义实现。

### 3.3 关于版本

`strstr(result->output, "pstree")` 是**区分大小写的小写匹配**：

- `printf("pstree 1.0\n")` ✅
- `printf("PSTREE ...")` ❌

另外，初始实现即使忽略参数，输出里也会出现 `pstree`（进程自己的 comm 就是 `pstree`），版本测试可能**碰巧通过**。不要依赖这个巧合，应真正实现 `-V` / `--version` 分支。

### 3.4 关于非法参数

`invalid_option` 要求：

1. **退出码非 0**（初始实现永远返回 0，必挂）；
2. 输出包含 `usage` / `Usage` / `invalid` / `Invalid`。

`getopt_long` 会自动往 stderr 打印类似：

```
./pstree: unrecognized option '--invalid-option'
```

其中包含 `invalid`，**理论上**已满足第 2 条；但稳妥起见应主动打印 usage，并返回非 0。

---

## 4. 测试未覆盖、但应该实现的需求

1. **真正的进程树**
   初始 `pstree.c` 只打印「父进程 + 父进程的直接子进程」，不是递归树。本意应是从根进程出发，递归遍历所有进程，用缩进或 `├─` / `└─` 画出层级。

2. **读取整个 `/proc`**
   应遍历 `/proc` 下所有数字目录，而不是只看 `getppid()` 的子进程。

3. **动态排序**
   默认按名字、`-n` 按 PID，且只对**同一层的兄弟**排序。

4. **`-p` 的开关语义**
   不加 `-p` 不输出 PID。

5. **usage**
   非法参数时输出用法并返回 1；建议同时支持 `-h` / `--help`（测试不要求）。

6. **`/proc/<pid>/stat` 解析的健壮性**
   初始代码：

   ```c
   sscanf(line, "%d (%255[^)]) %c %d", &id, comm, &state, &ppid)
   ```

   进程名里可能包含空格甚至 `)`（例如 `(sd-pam)`），`[^)]` 会截断，导致 ppid 解析错误。更稳的做法是**找最后一个 `')'`**，再从它后面解析 state 和 ppid。

7. **进程消失的竞态**
   扫描 `/proc` 时进程可能退出：`fopen` 失败就跳过，不应视为致命错误。

8. **空输出风险**
   如果根进程没有子进程且实现有 bug，可能输出为空导致 `basic_no_args` 挂。至少保证根进程本身会打印。

---

## 5. 当前 `pstree.c` 的差距对照

| 需求 | 现状 | 结论 |
|---|---|---|
| 参数解析 | `main(void)`，完全不解析 | 必须补 `getopt_long` |
| `-p` / `--show-pids` | 永远显示 PID | 语义不符（测试侥幸能过） |
| `-n` / `--numeric-sort` | 没有排序 | 测试不验证，但建议实现 |
| `-V` / `--version` | 没有版本分支 | 测试可能侥幸通过，应补 |
| 非法参数 | 永远 exit 0 | **必挂** |
| 递归进程树 | 只有父进程 + 直接子进程 | 测试不验证，但属于实验本意 |
| `/proc` 全扫描 | 只扫描 `getppid()` 的子进程 | 建议补 |
| stat 解析健壮性 | `%[^)]` 解析 | 建议改成找最后一个 `)` |

---

## 6. 验收自测命令

```bash
cd os2026/pstree
make all

./pstree                                # 应输出非空
./pstree -p                             # 应有 (pid)
./pstree --show-pids                    # 同上
./pstree -n                             # 应按 PID 排序
./pstree --numeric-sort                 # 同上
./pstree -V                             # 应包含 pstree
./pstree --version                      # 同上
./pstree -p -n                          # 组合
./pstree --show-pids --numeric-sort     # 长选项组合
./pstree --invalid-option; echo $?      # 应非 0，且有 usage/invalid

TK_RUN=1 ./pstree                       # 跑完整测试
```

---

## 7. 建议的实现顺序

1. 用 `getopt_long` 解析 `-p` / `-n` / `-V` + 非法选项，先让 10 条测试全绿；
2. 完整扫描 `/proc`，构建 `pid → (ppid, comm)` 表；
3. 从根进程递归打印，处理兄弟排序和缩进；
4. 加固 `/proc` 解析（最后一个 `)`、进程消失、缓冲区长度）。
