NAME := $(shell basename $(PWD))
export MODULE := M5

CFLAGS  := -I.
LDFLAGS += -lpthread

all: $(NAME) check

check:
	gcc -DFREESTANDING -I. \
	    -ffreestanding -static -nostdlib -nostartfiles -no-pie \
	    -o malloc-check *.c && \
	        rm -f malloc-check

include ../oslab.mk
include ../.shadow/oslab.mk

# .shadow/oslab.mk 会用 `find . -maxdepth 1` 覆盖 SRCS，
# 这里重新设置成：顶层 .c（mymalloc.c、start.c） + tests/ 下的测试。
SRCS := $(shell find . -maxdepth 1 -name "*.c") \
        $(shell find tests/ -maxdepth 1 -name "*.c")

# 让 tests/ 下的文件变化也能触发重新编译
$(NAME): $(SRCS)
