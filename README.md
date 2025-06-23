# OBS WebSocket SDK (CMake+Conan)

C++ SDK for interacting with OBS Studio via WebSocket protocol, built with CMake and Conan.

## 构建指南

### 前置要求
- CMake ≥ 3.15
- Conan ≥ 2.0
- C++17兼容编译器

### 开发环境配置
1. 安装Conan:
```bash
pip install conan
conan profile detect
```

2. 配置Conan远程仓库(可选):
```bash
conan remote add bincrafters https://bincrafters.jfrog.io/artifactory/api/conan/public-conan
```

### 构建步骤
```bash
# 创建构建目录
mkdir build && cd build

# Conan安装依赖并生成工具链
conan install .. --build=missing

# CMake配置项目
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake

# 编译项目
cmake --build .
```

### 运行示例程序
```bash
./build/obs_websocket_demo
```

## 项目结构
```
.
├── src/              # 源代码
├── CMakeLists.txt    # CMake构建配置
├── conanfile.txt     # Conan依赖配置
└── README.md         # 项目文档
```

## 使用示例
```cpp
#include "obs_websocket.h"
#include <iostream>

int main() {
    // 创建客户端实例
    auto client = CreateOBSWebSocketClient("ws://localhost:45600");

    client->set_password("hrf123456");
    // 连接OBS
    client->connect(std::function<void(bool)>([](bool r) {
        std::cout << "send connect: " << r << std::endl;
    }));
}
```

## 许可证
MIT License
