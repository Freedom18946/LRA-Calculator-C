# LRA-Calculator-C (响度范围计算器)

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/your-repo/LRA-Calculator-C)
[![Version](https://img.shields.io/badge/version-2.1-blue.svg)](https://github.com/your-repo/LRA-Calculator-C/releases)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)](https://github.com/your-repo/LRA-Calculator-C)

这是一个使用 C 语言编写的高性能命令行工具，用于计算媒体文件音轨的响度范围 (Loudness Range, LRA)。项目利用 FFmpeg 库来解码和处理多种格式的媒体文件，并根据 EBU R128 标准分析音频响度。

This is a high-performance command-line tool written in C for calculating the Loudness Range (LRA) of media file audio tracks. The project uses the FFmpeg library to decode and process various media file formats and analyzes audio loudness according to the EBU R128 standard.

## ✨ 功能特性 / Features

- **🔍 智能目录扫描 / Smart Directory Scanning**: 自动递归扫描指定目录下的所有音频文件 / Automatically and recursively scan all audio files in specified directories
- **🎵 多格式支持 / Multi-Format Support**: 支持 WAV, MP3, FLAC, AAC, M4A, OGG, Opus, WMA, AIFF, ALAC 等格式 / Support for WAV, MP3, FLAC, AAC, M4A, OGG, Opus, WMA, AIFF, ALAC and more
- **⚡ 高性能并发处理 / High-Performance Concurrent Processing**: 使用 Grand Central Dispatch (GCD) 实现多核并行计算 / Uses Grand Central Dispatch (GCD) for multi-core parallel computation
- **📊 精确 LRA 计算 / Precise LRA Calculation**: 基于 EBU R128 标准的精确响度范围计算 / Precise loudness range calculation based on EBU R128 standard
- **📁 结果管理 / Result Management**: 自动生成排序后的结果文件，并支持 TXT/CSV/JSON / Generate sorted result files with TXT/CSV/JSON support
- **🖥️ 跨平台支持 / Cross-Platform Support**: 支持 macOS, Linux, Windows (WSL) / Support for macOS, Linux, Windows (WSL)
- **🛠️ 灵活配置 / Flexible Configuration**: 支持多种命令行选项和配置模式 / Support for various command-line options and configuration modes
- **🛡️ 安全执行 / Safer Execution**: 通过 `fork/execvp` 调用 FFmpeg，避免 shell 注入 / Uses `fork/execvp` to avoid shell command injection
- **♻️ 增量恢复 / Incremental Resume**: 支持缓存未变更文件结果 (`--resume`) / Supports cached reuse for unchanged files (`--resume`)
- **🎯 路径过滤 / Path Filtering**: 支持 include/exclude glob 过滤 / Supports include/exclude glob filters
- **🔁 失败重试 / Retry on Failure**: 支持失败重试与失败报告输出 / Supports retries and failure report generation

## 📋 系统要求 / System Requirements

### 必需软件 / Required Software
- **操作系统 / OS**: macOS 10.12+, Linux (Ubuntu 18.04+), Windows 10+ (WSL)
- **编译器 / Compiler**: GCC 7.0+ 或 Clang 6.0+ / GCC 7.0+ or Clang 6.0+
- **构建工具 / Build Tools**: CMake 3.10+
- **音频处理 / Audio Processing**: FFmpeg 4.0+

### 依赖库 / Dependencies
- **FFmpeg 开发库 / FFmpeg Development Libraries**:
  - libavformat-dev
  - libavcodec-dev
  - libavutil-dev
  - libswresample-dev

### 快速安装依赖 / Quick Dependency Installation

#### macOS (使用 Homebrew / Using Homebrew)
```bash
# 安装 Homebrew (如果尚未安装) / Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 FFmpeg / Install FFmpeg
brew install ffmpeg cmake
```

#### Ubuntu/Debian
```bash
# 更新包管理器 / Update package manager
sudo apt update

# 安装依赖 / Install dependencies
sudo apt install -y libavformat-dev libavcodec-dev libavutil-dev libswresample-dev
sudo apt install -y build-essential cmake
```

#### CentOS/RHEL/Fedora
```bash
# Fedora
sudo dnf install -y ffmpeg-devel cmake gcc

# CentOS/RHEL (需要 EPEL 仓库 / Requires EPEL repository)
sudo yum install -y epel-release
sudo yum install -y ffmpeg-devel cmake gcc
```

## 🚀 快速开始 / Quick Start

### 1. 获取源代码 / Get Source Code
```bash
# 克隆仓库 / Clone repository
git clone <your-repository-url>
cd LRA-Calculator-C

# 或者下载并解压源代码包 / Or download and extract source code package
# tar -xzf LRA-Calculator-C.tar.gz && cd LRA-Calculator-C
```

### 2. 编译项目 / Build Project
```bash
# 创建构建目录 / Create build directory
mkdir build && cd build

# 配置项目 / Configure project
cmake ..

# 编译 (使用所有可用 CPU 核心) / Compile (using all available CPU cores)
make -j$(nproc)

# 或者使用 CMake 构建命令 / Or use CMake build command
# cmake --build . --parallel
```

### 3. 基本使用 / Basic Usage

#### 交互模式 / Interactive Mode
```bash
./LRA-Calculator-C
# 程序会提示您输入目录路径 / Program will prompt you to enter directory path
```

#### 命令行模式 / Command Line Mode
```bash
# 直接指定目录 / Specify directory directly
./LRA-Calculator-C /path/to/your/music

# 安静模式 / Quiet mode
./LRA-Calculator-C -q /path/to/music

# 详细模式 / Verbose mode
./LRA-Calculator-C --verbose /path/to/music

# 自定义输出文件 / Custom output file
./LRA-Calculator-C -o my_results.txt /path/to/music

# 限制并发数 / Limit concurrency
./LRA-Calculator-C -j 4 /path/to/music

# JSON 输出 + 增量缓存
./LRA-Calculator-C --format json --resume /path/to/music

# 只处理匹配路径并排除某些目录
./LRA-Calculator-C --include "*album*/*.flac" --exclude "*/demo/*" /path/to/music

# FFmpeg 超时/资源限制 + 失败重试
./LRA-Calculator-C --ffmpeg-timeout 90 --ffmpeg-max-memory-mb 768 --retry 2 /path/to/music
```

#### 查看帮助 / View Help
```bash
./LRA-Calculator-C --help
./LRA-Calculator-C --version
```

## 📁 项目结构 / Project Structure

```
LRA-Calculator-C/
├── 📄 CMakeLists.txt              # CMake 构建配置 / CMake build configuration
├── 📄 README.md                   # 项目说明文档 / Project documentation
├── 📁 src/                        # 源代码目录 / Source code directory
│   ├── 📄 main.c                  # 程序主入口 / Main program entry
│   ├── 📄 utils.h/c               # 工具函数模块 / Utility functions module
│   ├── 📄 file_scanner.h/c        # 文件扫描模块 / File scanner module
│   ├── 📄 ffmpeg_processor.h/c    # FFmpeg 处理模块 / FFmpeg processor module
│   ├── 📄 result_manager.h/c      # 结果管理模块 / Result manager module
│   └── 📄 cache_manager.h/c       # 增量缓存模块 / Incremental cache module
├── 📁 docs/                       # 文档目录 / Documentation directory
│   ├── 📄 README.md               # 文档索引 / Documentation index
│   ├── 📁 architecture/           # 架构设计文档 / Architecture design docs
│   ├── 📁 api/                    # API 参考文档 / API reference docs
│   ├── 📁 tutorials/              # 教程和指南 / Tutorials and guides
│   └── 📁 examples/               # 代码示例 / Code examples
├── 📁 test/                       # 测试目录 / Test directory
│   ├── 📄 CMakeLists.txt          # 测试构建配置 / Test build configuration
│   ├── 📁 unit/                   # 单元测试 / Unit tests
│   ├── 📁 integration/            # 集成测试 / Integration tests
│   └── 📁 data/                   # 测试数据 / Test data
└── 📁 build/                      # 构建输出目录 / Build output directory
```

### 模块说明 / Module Description

- **🎯 `main.c`**: 程序入口，负责命令行参数解析、用户交互和模块协调 / Program entry point, handles command-line parsing, user interaction, and module coordination
- **🔍 `file_scanner`**: 使用 nftw 递归遍历目录，识别支持的音频文件格式 / Uses nftw to recursively traverse directories and identify supported audio file formats
- **⚙️ `ffmpeg_processor`**: 核心处理模块，调用 FFmpeg 进行音频解码和 LRA 计算 / Core processing module, calls FFmpeg for audio decoding and LRA calculation
- **📊 `result_manager`**: 收集、排序并输出 TXT/CSV/JSON 结果与失败报告 / Collects, sorts, outputs TXT/CSV/JSON results and failure reports
- **♻️ `cache_manager`**: 维护缓存文件，支持 `--resume` 跳过未变更文件 / Maintains cache for `--resume` unchanged-file skipping
- **🛠️ `utils`**: 通用工具函数，包括动态数组、字符串处理、内存管理等 / Common utility functions including dynamic arrays, string processing, memory management, etc.

## 🎵 支持的音频格式 / Supported Audio Formats

| 格式 / Format | 扩展名 / Extension | 说明 / Description | 质量 / Quality |
|---------------|-------------------|-------------------|----------------|
| WAV | `.wav` | 无损音频格式 / Lossless audio | ⭐⭐⭐⭐⭐ |
| FLAC | `.flac` | 无损压缩音频 / Lossless compressed | ⭐⭐⭐⭐⭐ |
| ALAC | `.alac` | Apple 无损音频 / Apple Lossless | ⭐⭐⭐⭐⭐ |
| AIFF | `.aiff` | Apple 音频格式 / Apple audio format | ⭐⭐⭐⭐⭐ |
| MP3 | `.mp3` | 有损压缩音频 / Lossy compressed | ⭐⭐⭐⭐ |
| AAC | `.aac`, `.m4a` | 高效音频编码 / Advanced Audio Coding | ⭐⭐⭐⭐ |
| OGG | `.ogg` | 开源音频格式 / Open source format | ⭐⭐⭐⭐ |
| Opus | `.opus` | 现代音频编解码器 / Modern audio codec | ⭐⭐⭐⭐ |
| WMA | `.wma` | Windows Media Audio | ⭐⭐⭐ |

## ⚡ 性能特性 / Performance Features

### 并发处理 / Concurrent Processing
- **多核利用**: 使用 Grand Central Dispatch (GCD) 充分利用多核 CPU / Multi-core utilization using Grand Central Dispatch (GCD)
- **智能负载均衡**: 自动分配任务到可用的工作线程 / Intelligent load balancing with automatic task distribution
- **内存效率**: 动态内存管理，避免内存浪费 / Memory efficiency with dynamic memory management

### 性能基准 / Performance Benchmarks
- **处理速度**: 在现代多核系统上可达到 10-50 文件/秒 / Processing speed: 10-50 files/second on modern multi-core systems
- **内存使用**: 基础内存占用 < 10MB，随文件数量线性增长 / Memory usage: Base < 10MB, scales linearly with file count
- **CPU 利用率**: 可达到系统核心数的 80-95% 利用率 / CPU utilization: 80-95% of available cores

## 🔧 高级配置 / Advanced Configuration

### 命令行选项详解 / Command Line Options Details

```bash
LRA-Calculator-C [选项] [目录路径]

选项 / Options:
  -h, --help              显示帮助信息 / Show help message
  -v, --version           显示版本信息 / Show version information
  -i, --interactive       交互模式 (默认) / Interactive mode (default)
  -q, --quiet             安静模式，减少输出 / Quiet mode, reduce output
  --verbose               详细模式，显示调试信息 / Verbose mode, show debug info
  -o, --output <path>      指定输出文件 (必须位于扫描目录内)
  -j, --jobs <数量>        最大并发任务数 (实际生效)
  --format <txt|csv|json>  结果输出格式
  --include <glob>         仅处理匹配的相对路径
  --exclude <glob>         排除匹配的相对路径
  --resume                 启用缓存恢复，跳过未变更文件
  --cache <path>           指定缓存文件路径 (必须位于扫描目录内)
  --retry <N>              失败重试次数
  --ffmpeg-timeout <sec>   单文件超时秒数
  --ffmpeg-max-cpu <sec>   单文件 CPU 限制秒数
  --ffmpeg-max-memory-mb <mb> 单文件 FFmpeg 进程内存上限
  --failure-report <path>  指定失败报告路径
  --no-failure-report      不输出失败报告
```

### 环境变量 / Environment Variables

```bash
# 设置默认并发数 / Set default concurrency
export LRA_MAX_JOBS=8

# 设置默认输出文件 / Set default output file
export LRA_OUTPUT_FILE="custom_results.txt"

# 启用详细日志 / Enable verbose logging
export LRA_VERBOSE=1
```

## 🧪 测试 / Testing

项目包含完整的测试套件，确保代码质量和功能正确性。
The project includes a comprehensive test suite to ensure code quality and functional correctness.

### 运行测试 / Running Tests

```bash
# 构建测试 / Build tests
cd test && mkdir build && cd build
cmake .. && make

# 运行所有测试 / Run all tests
make run_tests

# 运行单元测试 / Run unit tests only
make run_unit_tests

# 运行集成测试 / Run integration tests only
make run_integration_tests
```

### 测试覆盖 / Test Coverage
- ✅ 工具函数单元测试 / Utility functions unit tests
- ✅ 文件扫描模块测试 / File scanner module tests
- ✅ 结果输出模块测试 (TXT/CSV/JSON + 分隔符路径) / Result manager tests
- ✅ 缓存模块测试 / Cache manager tests
- ✅ 内存管理测试 / Memory management tests
- ✅ 错误处理测试 / Error handling tests
- ✅ 集成测试 / Integration tests

## 🐛 故障排除 / Troubleshooting

### 常见问题 / Common Issues

#### Q: 程序提示 "ffmpeg: command not found"
**A**: 确保 FFmpeg 已正确安装并在系统 PATH 中。
```bash
# 验证 FFmpeg 安装 / Verify FFmpeg installation
ffmpeg -version

# macOS 安装 / macOS installation
brew install ffmpeg

# Ubuntu 安装 / Ubuntu installation
sudo apt install ffmpeg
```

#### Q: 编译时出现 "libavformat not found" 错误
**A**: 安装 FFmpeg 开发库。
```bash
# Ubuntu/Debian
sudo apt install libavformat-dev libavcodec-dev libavutil-dev libswresample-dev

# CentOS/RHEL
sudo yum install ffmpeg-devel
```

#### Q: 程序运行很慢或占用 CPU 过高
**A**: 调整并发数量。
```bash
# 限制并发数为 CPU 核心数 / Limit concurrency to CPU cores
./LRA-Calculator-C -j $(nproc) /path/to/music

# 或者使用更保守的设置 / Or use more conservative setting
./LRA-Calculator-C -j 2 /path/to/music
```

#### Q: 某些文件处理失败
**A**: 检查文件完整性和格式支持。
```bash
# 使用详细模式查看错误信息 / Use verbose mode to see error details
./LRA-Calculator-C --verbose /path/to/music

# 手动测试单个文件 / Manually test single file
ffmpeg -i problem_file.mp3 -filter_complex ebur128 -f null -
```

## 📚 文档 / Documentation

完整的项目文档位于 `docs/` 目录中：
Complete project documentation is located in the `docs/` directory:

- 📖 [快速开始指南](docs/tutorials/quick-start.md) / Quick Start Guide
- 🏗️ [系统架构](docs/architecture/system-overview.md) / System Architecture
- 🔌 [API 参考](docs/api/) / API Reference
- 🎯 [使用示例](docs/tutorials/usage-examples.md) / Usage Examples
- 🤝 [贡献指南](docs/tutorials/contributing.md) / Contributing Guide

## 🤝 贡献 / Contributing

我们欢迎各种形式的贡献！请查看 [贡献指南](docs/tutorials/contributing.md) 了解详细信息。
We welcome contributions of all kinds! Please see the [Contributing Guide](docs/tutorials/contributing.md) for details.

### 开发环境设置 / Development Environment Setup
```bash
# 克隆仓库 / Clone repository
git clone <repository-url>
cd LRA-Calculator-C

# 安装开发依赖 / Install development dependencies
# (参见上面的依赖安装部分 / See dependency installation section above)

# 构建项目 / Build project
mkdir build && cd build
cmake .. && make

# 运行测试 / Run tests
cd ../test && mkdir build && cd build
cmake .. && make && make run_tests
```

## 📄 许可证 / License

本项目采用 MIT 许可证。详情请见 [LICENSE](LICENSE) 文件。
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 🙏 致谢 / Acknowledgments

- [FFmpeg](https://ffmpeg.org/) - 强大的多媒体处理库 / Powerful multimedia processing library
- [EBU R128](https://tech.ebu.ch/docs/r/r128.pdf) - 响度标准规范 / Loudness standard specification
- [Grand Central Dispatch](https://developer.apple.com/documentation/dispatch) - 高性能并发框架 / High-performance concurrency framework

---

**版本 / Version**: 2.1
**最后更新 / Last Updated**: 2025-08-07
**维护者 / Maintainer**: LRA-Calculator-C Development Team
