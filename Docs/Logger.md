# Logger 类技术实现说明

## 1. 核心功能

Logger 类是一个功能强大的日志系统，提供了以下核心功能：

- **多级别日志**：支持 Trace、Debug、Info、Warning、Error、Fatal 六个级别的日志
- **异步日志**：通过后台线程处理日志，减少对主线程的影响
- **日志文件滚动**：自动管理日志文件大小，支持历史日志保留
- **时间戳**：每条日志都包含精确到毫秒的时间信息
- **位置信息**：通过 `std::source_location` 自动记录日志的文件、行号和函数名
- **跨平台兼容**：处理不同平台的彩色输出差异
- **线程安全**：使用互斥锁确保多线程环境下的安全操作
- **多输出流支持**：可以同时输出到多个流（如控制台和文件）

## 2. 技术实现

### 2.1 异步日志

- **实现方式**：使用 `std::thread` 创建后台线程，`std::queue` 存储日志消息，`std::condition_variable` 实现线程间通信
- **优势**：将日志处理与主线程分离，避免日志操作阻塞主线程，提高程序性能

### 2.2 日志文件滚动

- **实现方式**：当日志文件达到指定大小时（默认 10MB），自动创建新文件并保留历史日志
- **配置**：可通过 `setLogFileSizeLimit` 和 `setLogFileCountLimit` 配置文件大小和保留数量
- **优势**：避免单个日志文件过大，方便日志管理和分析

### 2.3 时间戳

- **实现方式**：使用 `std::chrono` 库获取当前时间，并格式化为 "YYYY-MM-DD HH:MM:SS.ms" 格式
- **优势**：每条日志都包含精确的时间信息，便于问题定位和日志分析

### 2.4 位置信息

- **实现方式**：利用 C++20 的 `std::source_location` 自动获取日志调用的文件、行号和函数名
- **优势**：无需手动传入位置信息，自动记录日志来源，方便调试

### 2.5 跨平台兼容

- **实现方式**：通过条件编译处理 Windows 和 Unix-like 系统的彩色输出差异
- **优势**：在不同平台上都能正常工作，提供一致的用户体验

### 2.6 线程安全

- **实现方式**：使用 `std::mutex` 保护共享资源，确保多线程环境下的安全操作
- **优势**：可在多线程环境中安全使用，避免并发问题

### 2.7 多输出流支持

- **实现方式**：使用 `std::vector<std::reference_wrapper<std::ostream>>` 存储多个输出流
- **优势**：可以同时将日志输出到多个目标，如控制台和文件

## 3. 使用方式

### 3.1 基本使用

通过宏即可快速记录不同级别的日志：

```cpp
LOG_TRACE("This is a trace message: {}", 42);
LOG_DEBUG("This is a debug message: {}", "hello");
LOG_INFO("This is an info message: {} + {} = {}", 1, 2, 3);
LOG_WARNING("This is a warning message");
LOG_ERROR("This is an error message");
LOG_FATAL("This is a fatal message");
```

### 3.2 配置示例

```cpp
// 获取全局日志器
auto& logger = globalLogger();

// 启用异步日志
logger.setAsyncLogging(true);

// 设置日志文件大小限制
logger.setLogFileSizeLimit(5 * 1024 * 1024); // 5MB

// 设置日志文件数量限制
logger.setLogFileCountLimit(3);

// 设置最低日志级别
logger.setLowestOutputLevel(Logger::Debug);
```

## 4. 性能优化

- **异步处理**：将日志处理移至后台线程，减少对主线程的影响
- **条件日志**：根据日志级别和阻塞状态决定是否处理日志，避免不必要的操作
- **批量处理**：后台线程批量处理日志消息，减少线程切换开销

## 5. 代码结构

- **头文件**：`tchMain/inc/debug/Logger.h` - 包含 Logger 类的声明和宏定义
- **源文件**：`tchMain/src/debug/Logger.cpp` - 包含 Logger 类的实现

## 6. 技术亮点

1. **现代 C++ 特性**：充分利用 C++20 的 `std::source_location` 和 `std::format` 等现代特性
2. **高性能设计**：异步日志处理，减少对主线程的影响
3. **灵活配置**：支持多种配置选项，适应不同场景需求
4. **跨平台兼容**：处理不同平台的差异，提供一致的用户体验
5. **易用性**：简洁的宏接口，使用方便

Logger 类的设计充分考虑了性能、可靠性和易用性，为项目提供了一个强大而灵活的日志系统。