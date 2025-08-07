# 系统架构概述 / System Architecture Overview

## 项目简介 / Project Introduction

LRA-Calculator-C 是一个高性能的音频响度范围 (Loudness Range, LRA) 计算工具，基于 EBU R128 标准实现。项目采用模块化设计，使用 C 语言开发，支持多种音频格式的批量处理。

LRA-Calculator-C is a high-performance audio Loudness Range (LRA) calculation tool based on the EBU R128 standard. The project uses modular design, developed in C language, and supports batch processing of various audio formats.

## 核心架构 / Core Architecture

### 整体设计原则 / Overall Design Principles

1. **模块化设计 (Modular Design)**: 将功能分解为独立的模块，便于维护和扩展
2. **高性能并发 (High-Performance Concurrency)**: 使用 Grand Central Dispatch (GCD) 实现高效的并行处理
3. **跨平台兼容 (Cross-Platform Compatibility)**: 基于标准 C 和 POSIX API，支持多平台编译
4. **内存安全 (Memory Safety)**: 严格的内存管理和错误处理机制

### 系统组件图 / System Components Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    LRA-Calculator-C                        │
├─────────────────────────────────────────────────────────────┤
│  main.c - 主程序入口 / Main Entry Point                     │
│  ├─ 用户交互 / User Interaction                            │
│  ├─ 全局状态管理 / Global State Management                  │
│  └─ 并发任务协调 / Concurrent Task Coordination            │
├─────────────────────────────────────────────────────────────┤
│  file_scanner - 文件扫描模块 / File Scanner Module          │
│  ├─ 目录递归遍历 (nftw) / Directory Recursive Traversal    │
│  ├─ 文件类型过滤 / File Type Filtering                     │
│  └─ 路径规范化 / Path Normalization                        │
├─────────────────────────────────────────────────────────────┤
│  ffmpeg_processor - FFmpeg 处理模块 / FFmpeg Processor     │
│  ├─ FFmpeg 命令构建 / FFmpeg Command Building              │
│  ├─ 进程管道通信 / Process Pipeline Communication          │
│  └─ LRA 数值解析 / LRA Value Parsing                       │
├─────────────────────────────────────────────────────────────┤
│  result_manager - 结果管理模块 / Result Manager Module      │
│  ├─ 结果收集 / Result Collection                           │
│  ├─ 文件输出 / File Output                                 │
│  └─ 数据排序 / Data Sorting                               │
├─────────────────────────────────────────────────────────────┤
│  utils - 工具函数模块 / Utility Functions Module           │
│  ├─ 动态数组管理 / Dynamic Array Management                │
│  ├─ 字符串处理 / String Processing                         │
│  ├─ 文件路径操作 / File Path Operations                    │
│  └─ 时间格式化 / Time Formatting                           │
└─────────────────────────────────────────────────────────────┘
```

## 数据流程 / Data Flow

### 主要处理流程 / Main Processing Flow

1. **初始化阶段 / Initialization Phase**
   - 用户输入目录路径 / User inputs directory path
   - 初始化全局数据结构 / Initialize global data structures
   - 创建互斥锁和原子计数器 / Create mutex and atomic counters

2. **文件扫描阶段 / File Scanning Phase**
   - 使用 nftw 递归遍历目录 / Use nftw for recursive directory traversal
   - 过滤支持的音频文件格式 / Filter supported audio file formats
   - 构建待处理文件列表 / Build file processing list

3. **并行处理阶段 / Parallel Processing Phase**
   - 创建 GCD 任务队列 / Create GCD task queue
   - 为每个文件创建处理任务 / Create processing task for each file
   - 并发执行 FFmpeg 分析 / Concurrent FFmpeg analysis execution

4. **结果收集阶段 / Result Collection Phase**
   - 线程安全的结果聚合 / Thread-safe result aggregation
   - 错误处理和状态跟踪 / Error handling and status tracking
   - 进度显示更新 / Progress display updates

5. **输出生成阶段 / Output Generation Phase**
   - 结果文件写入 / Result file writing
   - 数据排序处理 / Data sorting processing
   - 统计信息汇总 / Statistics summary

## 并发模型 / Concurrency Model

### Grand Central Dispatch (GCD) 架构

```
主线程 (Main Thread)
├─ 用户交互处理 / User Interaction Handling
├─ 全局状态管理 / Global State Management
└─ 任务调度协调 / Task Scheduling Coordination

全局并发队列 (Global Concurrent Queue)
├─ 工作线程池 / Worker Thread Pool
├─ 任务负载均衡 / Task Load Balancing
└─ 自动资源管理 / Automatic Resource Management

同步机制 (Synchronization Mechanisms)
├─ pthread_mutex_t - 结果列表保护 / Result List Protection
├─ atomic_size_t - 原子计数器 / Atomic Counter
└─ dispatch_group_t - 任务组同步 / Task Group Synchronization
```

### 线程安全设计 / Thread Safety Design

- **共享数据保护**: 使用互斥锁保护共享的结果数组
- **原子操作**: 使用原子计数器跟踪处理进度
- **任务隔离**: 每个文件处理任务独立运行，避免数据竞争
- **资源管理**: GCD 自动管理线程生命周期和资源分配

## 外部依赖 / External Dependencies

### 系统依赖 / System Dependencies
- **POSIX API**: nftw, pthread, 文件系统操作
- **Grand Central Dispatch**: macOS/iOS 并发框架
- **标准 C 库**: stdio, stdlib, string, math

### 第三方工具 / Third-Party Tools
- **FFmpeg**: 音频解码和 LRA 计算的核心引擎
- **CMake**: 跨平台构建系统

## 性能特性 / Performance Characteristics

### 优化策略 / Optimization Strategies
1. **并发处理**: 充分利用多核 CPU 资源
2. **内存效率**: 动态数组管理，避免内存浪费
3. **I/O 优化**: 批量文件操作，减少系统调用
4. **进程复用**: 高效的 FFmpeg 进程管道通信

### 扩展性考虑 / Scalability Considerations
- 支持大规模文件批处理 / Support large-scale file batch processing
- 内存使用随文件数量线性增长 / Memory usage scales linearly with file count
- CPU 利用率可达到系统核心数上限 / CPU utilization can reach system core limit

---

**文档版本 / Document Version**: v1.0  
**最后更新 / Last Updated**: 2025-08-07  
**相关文档 / Related Documents**: [模块设计详解](./module-design.md), [数据流程图解](./data-flow.md)
