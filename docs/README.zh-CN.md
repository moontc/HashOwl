# 🦉 HashOwl 中文说明

[English README](../readme.md) | 简体中文

你是否经常怀疑你硬盘里宝贵的文件被来自宇宙的射线破坏了，如果是，那么HashOwl就是一个绝佳的检测工具。

HashOwl 是一个使用现代 C++ 编写的高性能多线程命令行工具。
它可以对单个文件或整个目录进行哈希计算，导出 JSON 快照，并在后续基于该快照做完整性校验。

## 主要功能

- 多线程目录扫描，适合大规模文件集。
- 支持文件级与目录级哈希。
- 导出结构化 JSON 快照，便于留档与复验。
- 校验模式可报告：通过、修改、缺失、未跟踪文件。
- 内置进度与吞吐显示。

## 支持算法

- crc32（默认）
- crc32c（运行时需 CPU 支持 SSE4.2）
- crc64
- md5
- sha1
- sha256
- sha384
- sha512
- blake3

## 构建环境（当前）

- 操作系统：Windows
- 编译器：MSVC
- 构建系统：CMake + Ninja 预设
- CMake：3.21 及以上

## 构建命令

```bash
cmake --preset x64-debug
cmake --build out/build/x64-debug

cmake --preset x64-release
cmake --build out/build/x64-release
```

## 基本用法

```bash
./out/build/x64-release/src/HashOwl.exe <path> [--algo <md5|sha1|sha256|sha384|sha512|crc32|crc32c|crc64|blake3>] [-o [output_path]] [--verify <snapshot.json>]
```

- `--algo`：指定算法，默认 CRC32。
- `-o [output_path]`：导出 JSON 快照（可选指定路径）。
- `--verify` 或 `-v`：使用已有快照进行校验。
- `--help` 或 `-h`：查看帮助。

## 返回码

- 0：成功
- 1：参数错误
- 2：运行时错误
- 3：校验失败（存在修改或缺失）
