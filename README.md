# LRA-Calculator-C (响度范围计算器)

这是一个使用 C 语言编写的命令行工具，用于计算媒体文件音轨的响度范围 (Loudness Range, LRA)。项目利用 FFmpeg 库来解码和处理多种格式的媒体文件，并根据 EBU R128 标准分析音频响度。

## 功能特性

- **目录扫描**: 自动扫描指定目录下的所有媒体文件。
- **多格式支持**: 借助 FFmpeg，支持几乎所有主流的音频和视频文件格式。
- **响度范围计算**: 精确计算音频的 LRA 值。
- **结果管理**: 统一处理和展示计算结果。
- **跨平台**: 使用 CMake 构建，理论上可以轻松地在 Windows, macOS 和 Linux 上编译和运行。

## 依赖环境

在编译和运行本项目之前，请确保您的系统已安装以下软件和库：

- **CMake** (版本 >= 3.10)
- **C 编译器** (例如 GCC, Clang)
- **FFmpeg 开发库** (包括 libavcodec, libavformat, libavutil, libswresample 等)

在 macOS 上，您可以使用 Homebrew 安装 FFmpeg:
```bash
brew install ffmpeg
```

在 Debian/Ubuntu 上，您可以使用 apt:
```bash
sudo apt-get update
sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev libswresample-dev
```

## 如何编译与运行

1.  **克隆仓库** (如果这是一个 git 仓库)
    ```bash
    # git clone <your-repository-url>
    # cd LRA-Calculator-C
    ```

2.  **创建构建目录**
    ```bash
    mkdir build
    cd build
    ```

3.  **配置项目**
    使用 CMake 生成构建系统文件。
    ```bash
    cmake ..
    ```

4.  **编译项目**
    ```bash
    make
    ```
    或者使用 CMake 的构建命令：
    ```bash
    cmake --build .
    ```

5.  **运行程序**
    可执行文件 `LRA-Calculator-C` 将会生成在 `build` 目录下。

    **使用方法:**
    ```bash
    ./LRA-Calculator-C <要扫描的媒体文件目录路径>
    ```
    例如:
    ```bash
    ./LRA-Calculator-C /path/to/your/media/files
    ```

## 项目结构

```
LRA-Calculator-C/
├── CMakeLists.txt          # CMake 构建配置文件
├── src/                    # 源代码目录
│   ├── main.c              # 程序主入口
│   ├── file_scanner.h/c    # 负责扫描媒体文件
│   ├── ffmpeg_processor.h/c# 核心模块，使用 FFmpeg 解码和处理音频
│   ├── result_manager.h/c  # 负责管理和展示计算结果
│   └── utils.h/c           # 提供一些通用辅助函数
└── ...
```

- **`main.c`**: 程序的入口，负责解析命令行参数和协调其他模块的工作。
- **`file_scanner`**: 遍历用户指定的目录，识别出可处理的媒体文件，并将它们交给 `ffmpeg_processor`。
- **`ffmpeg_processor`**: 项目的核心。调用 FFmpeg 库的 API 来打开媒体文件，读取音频流，进行解码和重采样，并执行计算响度范围所需的核心算法。
- **`result_manager`**: 收集每个文件的计算结果，并以友好的格式（例如打印到控制台）进行输出。
- **`utils`**: 包含一些被多个模块共用的辅助函数，例如内存管理、错误处理等。

## 许可证

本项目采用 MIT 许可证。详情请见 `LICENSE` 文件（如果存在）。
