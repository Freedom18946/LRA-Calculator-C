# 快速开始指南 / Quick Start Guide

本指南将帮助您在 5 分钟内快速上手 LRA-Calculator-C，开始计算音频文件的响度范围。

This guide will help you get started with LRA-Calculator-C in 5 minutes and begin calculating loudness range for audio files.

## 系统要求 / System Requirements

### 必需软件 / Required Software
- **操作系统 / OS**: macOS 10.12+, Linux (Ubuntu 18.04+), Windows 10+ (WSL)
- **编译器 / Compiler**: GCC 7.0+ 或 Clang 6.0+
- **构建工具 / Build Tools**: CMake 3.10+
- **音频处理 / Audio Processing**: FFmpeg 4.0+

### 依赖库 / Dependencies
- **FFmpeg 开发库 / FFmpeg Development Libraries**:
  - libavformat-dev
  - libavcodec-dev  
  - libavutil-dev
  - libswresample-dev

## 安装步骤 / Installation Steps

### 1. 安装 FFmpeg / Install FFmpeg

#### macOS (使用 Homebrew / Using Homebrew)
```bash
# 安装 Homebrew (如果尚未安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 FFmpeg
brew install ffmpeg
```

#### Ubuntu/Debian
```bash
# 更新包管理器
sudo apt update

# 安装 FFmpeg 开发库
sudo apt install -y libavformat-dev libavcodec-dev libavutil-dev libswresample-dev

# 安装构建工具
sudo apt install -y build-essential cmake
```

#### CentOS/RHEL/Fedora
```bash
# Fedora
sudo dnf install -y ffmpeg-devel cmake gcc

# CentOS/RHEL (需要 EPEL 仓库)
sudo yum install -y epel-release
sudo yum install -y ffmpeg-devel cmake gcc
```

### 2. 获取源代码 / Get Source Code

```bash
# 克隆仓库 (如果使用 Git)
git clone <repository-url>
cd LRA-Calculator-C

# 或者解压源代码包
# tar -xzf LRA-Calculator-C.tar.gz
# cd LRA-Calculator-C
```

### 3. 编译项目 / Build Project

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake ..

# 编译 (使用所有可用 CPU 核心)
make -j$(nproc)

# 或者使用 CMake 构建命令
# cmake --build . --parallel
```

### 4. 验证安装 / Verify Installation

```bash
# 检查可执行文件
ls -la LRA-Calculator-C

# 显示帮助信息 (按 Ctrl+C 退出输入提示)
echo "" | ./LRA-Calculator-C
```

## 基础使用 / Basic Usage

### 准备测试文件 / Prepare Test Files

```bash
# 创建测试目录
mkdir -p ~/test-audio
cd ~/test-audio

# 下载测试音频文件 (示例)
# wget https://example.com/test.wav
# 或者复制您自己的音频文件到此目录
```

### 运行 LRA 计算 / Run LRA Calculation

```bash
# 返回到构建目录
cd /path/to/LRA-Calculator-C/build

# 运行程序
./LRA-Calculator-C
```

程序会提示您输入音频文件目录路径：
```
欢迎使用音频 LRA 计算器 (C语言版 V2.0 - GCD & NFTW)！
当前时间: 2025-08-07 10:07:37
请输入要递归处理的音乐顶层文件夹路径: 
```

输入您的音频文件目录路径，例如：
```
/Users/username/test-audio
```

### 查看结果 / View Results

程序完成后，结果将保存在指定目录下的 `lra_results.txt` 文件中：

```bash
# 查看结果文件
cat /Users/username/test-audio/lra_results.txt
```

结果格式示例：
```
文件路径 (相对) - LRA 数值 (LU)
music/song1.mp3 - 8.5
music/song2.flac - 12.3
music/song3.wav - 6.7
```

## 支持的文件格式 / Supported File Formats

LRA-Calculator-C 支持以下音频格式：

| 格式 / Format | 扩展名 / Extension | 说明 / Description |
|---------------|-------------------|-------------------|
| WAV | .wav | 无损音频格式 / Lossless audio |
| FLAC | .flac | 无损压缩音频 / Lossless compressed |
| MP3 | .mp3 | 有损压缩音频 / Lossy compressed |
| AAC | .aac, .m4a | 高效音频编码 / Advanced Audio Coding |
| OGG | .ogg | 开源音频格式 / Open source format |
| Opus | .opus | 现代音频编解码器 / Modern audio codec |
| WMA | .wma | Windows Media Audio |
| AIFF | .aiff | Apple 音频格式 / Apple audio format |
| ALAC | .alac | Apple 无损音频 / Apple Lossless |

## 常见问题 / Common Issues

### Q: 程序提示 "ffmpeg: command not found"
**A**: 确保 FFmpeg 已正确安装并在系统 PATH 中。运行 `ffmpeg -version` 验证安装。

### Q: 编译时出现 "libavformat not found" 错误
**A**: 安装 FFmpeg 开发库。在 Ubuntu 上运行：`sudo apt install libavformat-dev`

### Q: 程序运行很慢
**A**: 这是正常现象。LRA 计算需要分析整个音频文件，大文件需要更多时间。程序会显示进度信息。

### Q: 某些文件处理失败
**A**: 检查文件是否损坏或格式不受支持。查看程序输出的错误信息获取详细信息。

## 下一步 / Next Steps

- 📖 阅读 [详细使用指南](./usage-examples.md) 了解高级功能
- 🔧 查看 [API 参考文档](../api/) 了解内部实现
- 🐛 遇到问题？查看 [故障排除指南](./troubleshooting.md)
- 🤝 想要贡献代码？阅读 [贡献指南](./contributing.md)

---

**文档版本 / Document Version**: v1.0  
**最后更新 / Last Updated**: 2025-08-07  
**相关文档 / Related Documents**: [编译构建指南](./build-guide.md), [使用示例](./usage-examples.md)
