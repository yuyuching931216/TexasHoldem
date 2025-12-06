# 依賴安裝指南

## 系統需求

### 伺服器端 (C++)
- **作業系統**: Windows 10+, Linux (Ubuntu 18.04+), macOS 10.14+
- **編譯器**: 
  - Windows: Visual Studio 2019+ 或 MinGW-w64
  - Linux: GCC 7+ 或 Clang 5+
  - macOS: Xcode 10+ (Clang)
- **C++ 標準**: C++17 或更高
- **Boost**: 1.70 或更高版本

### 客戶端 (Python)
- **Python**: 3.6 或更高版本
- **依賴**: 標準庫（無需額外安裝）

## 詳細安裝步驟

### Windows

#### 方法1: 使用 vcpkg (推薦)

vcpkg 是 Microsoft 的 C++ 包管理器，最方便。

```powershell
# 1. 安裝 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 2. 安裝 Boost
.\vcpkg install boost-asio:x64-windows

# 3. 整合到 Visual Studio
.\vcpkg integrate install

# 4. 回到專案目錄編譯
cd path\to\TexasHoldem
# 打開 TexasHoldem.sln 並編譯
```

#### 方法2: 手動安裝 Boost

```powershell
# 1. 下載 Boost
# 訪問 https://www.boost.org/users/download/
# 下載預編譯版本或源代碼

# 2. 解壓到 C:\boost_1_81_0 (或其他位置)

# 3. 設置環境變量
[Environment]::SetEnvironmentVariable("BOOST_ROOT", "C:\boost_1_81_0", "User")

# 4. 如果需要編譯 Boost (可選，Asio 是 header-only)
cd C:\boost_1_81_0
.\bootstrap.bat
.\b2
```

#### 安裝 Python (如果尚未安裝)

```powershell
# 使用 Windows Store
# 或從 https://www.python.org/downloads/ 下載

# 驗證安裝
python --version
```

### Linux (Ubuntu/Debian)

```bash
# 更新包列表
sudo apt-get update

# 安裝 C++ 編譯器和工具
sudo apt-get install -y build-essential cmake

# 安裝 Boost
sudo apt-get install -y libboost-all-dev

# 或只安裝需要的部分
sudo apt-get install -y libboost-system-dev libboost-thread-dev

# 安裝 Python 3 (通常已預裝)
sudo apt-get install -y python3 python3-pip

# 驗證安裝
g++ --version
cmake --version
python3 --version
```

### Linux (CentOS/RHEL)

```bash
# 安裝開發工具
sudo yum groupinstall "Development Tools"
sudo yum install cmake

# 安裝 Boost
sudo yum install boost-devel

# 安裝 Python 3
sudo yum install python3

# 驗證安裝
g++ --version
python3 --version
```

### macOS

```bash
# 安裝 Homebrew (如果尚未安裝)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安裝 Xcode Command Line Tools
xcode-select --install

# 安裝 Boost
brew install boost

# 安裝 CMake
brew install cmake

# Python 3 通常已預裝，如果沒有：
brew install python3

# 驗證安裝
clang++ --version
cmake --version
python3 --version
```

## 驗證安裝

### 驗證 Boost

創建測試文件 `test_boost.cpp`:

```cpp
#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <iostream>

int main() {
    std::cout << "Boost version: " << BOOST_VERSION / 100000 << "."
              << BOOST_VERSION / 100 % 1000 << "."
              << BOOST_VERSION % 100 << std::endl;
    
    boost::asio::io_context io;
    std::cout << "Boost.Asio is working!" << std::endl;
    
    return 0;
}
```

編譯並運行：

```bash
# Linux/Mac
g++ -std=c++17 test_boost.cpp -o test_boost -lboost_system -lpthread
./test_boost

# Windows (使用 Visual Studio Command Prompt)
cl /EHsc /std:c++17 test_boost.cpp
test_boost.exe
```

預期輸出：
```
Boost version: 1.81.0
Boost.Asio is working!
```

### 驗證 Python

```bash
python3 --version
# 或 Windows
python --version

# 測試 socket 模塊
python3 -c "import socket; print('Socket module OK')"
```

## 常見問題

### Q: Boost 找不到

**Windows:**
```powershell
# 設置環境變量
$env:BOOST_ROOT = "C:\path\to\boost"

# 或在 Visual Studio 中設置
# Project Properties > C/C++ > General > Additional Include Directories
# 添加: C:\path\to\boost
```

**Linux/Mac:**
```bash
# 檢查 Boost 安裝位置
dpkg -L libboost-dev | grep boost/version.hpp
# 或
brew info boost

# 如果 CMake 找不到，設置環境變量
export BOOST_ROOT=/usr/local/opt/boost
```

### Q: 編譯器不支持 C++17

**解決方案:**

更新編譯器：

```bash
# Ubuntu
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-9 g++-9

# CentOS
sudo yum install centos-release-scl
sudo yum install devtoolset-9
scl enable devtoolset-9 bash
```

### Q: 鏈接錯誤

**Windows:**
```
error LNK2019: unresolved external symbol
```

**解決:** 確保在專案設置中包含了必要的庫文件。

**Linux:**
```bash
# 添加鏈接選項
g++ ... -lboost_system -lpthread
```

### Q: Python 模塊導入錯誤

```bash
# 確保使用正確的 Python 版本
python3 -m pip install --upgrade pip

# 如果需要虛擬環境
python3 -m venv venv
source venv/bin/activate  # Linux/Mac
# 或
venv\Scripts\activate  # Windows
```

## 最小依賴版本測試

已測試的最小版本組合：

| 組件 | 最小版本 | 推薦版本 |
|------|---------|---------|
| C++ 編譯器 | GCC 7.0, MSVC 19.14 | GCC 10+, MSVC 19.28+ |
| Boost | 1.70.0 | 1.81.0+ |
| CMake | 3.10 | 3.20+ |
| Python | 3.6 | 3.9+ |

## Docker 安裝 (可選)

如果你想使用 Docker:

```dockerfile
# Dockerfile (伺服器)
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev

WORKDIR /app
COPY . .
RUN mkdir build && cd build && cmake .. && make

EXPOSE 8888
CMD ["./build/TexasHoldemServer", "8888"]
```

構建並運行：

```bash
docker build -t poker-server .
docker run -p 8888:8888 poker-server
```

## 下一步

安裝完成後：

1. 閱讀 [QUICKSTART.md](QUICKSTART.md) 快速開始
2. 閱讀 [VISUAL_STUDIO_SETUP.md](VISUAL_STUDIO_SETUP.md) 了解 Visual Studio 配置
3. 開始編譯專案

## 獲取幫助

如果遇到安裝問題：

1. 查看編譯器和 Boost 版本是否符合要求
2. 檢查環境變量設置
3. 查看錯誤日誌
4. 在 GitHub 上提交 issue

## 參考鏈接

- [Boost 官網](https://www.boost.org/)
- [Boost.Asio 文檔](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [vcpkg GitHub](https://github.com/Microsoft/vcpkg)
- [CMake 官網](https://cmake.org/)
- [Python 官網](https://www.python.org/)
