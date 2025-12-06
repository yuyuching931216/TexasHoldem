# Visual Studio 專案配置說明

由於 `.vcxproj` 文件目前可能被 Visual Studio 打開，你需要手動添加新的源文件到專案中。

## 方法1: 在 Visual Studio 中手動添加

1. 在 Visual Studio 中打開 `TexasHoldem.sln`
2. 在 Solution Explorer 中，右鍵點擊 `Source Files`
3. 選擇 `Add > Existing Item...`
4. 添加以下新文件：
   - `server\Room.cpp`
   - `server\Server.cpp`
   - `server\Session.cpp`

5. 在 Solution Explorer 中，右鍵點擊 `Header Files`
6. 選擇 `Add > Existing Item...`
7. 添加以下新文件：
   - `server\Room.h`
   - `server\Server.h`
   - `server\Session.h`

8. 刪除這些測試文件（如果存在）：
   - `server\smartptr_test.cpp`
   - `server\test.cpp`
   - `test.cpp`

## 方法2: 編輯 .vcxproj 文件

如果 Visual Studio 未打開，你可以直接編輯 `TexasHoldem.vcxproj`：

找到 `<ItemGroup>` 中的 `<ClCompile>` 部分，修改為：

```xml
  <ItemGroup>
    <ClCompile Include="server\Card.cpp" />
    <ClCompile Include="server\Game.cpp" />
    <ClCompile Include="server\HandEvaluator.cpp" />
    <ClCompile Include="server\main.cpp" />
    <ClCompile Include="server\Player.cpp" />
    <ClCompile Include="server\Room.cpp" />
    <ClCompile Include="server\Server.cpp" />
    <ClCompile Include="server\Session.cpp" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Include="server\Card.h" />
    <ClInclude Include="server\Game.h" />
    <ClInclude Include="server\HandEvaluator.h" />
    <ClInclude Include="server\Player.h" />
    <ClInclude Include="server\Room.h" />
    <ClInclude Include="server\Server.h" />
    <ClInclude Include="server\Session.h" />
  </ItemGroup>
```

## 配置 Boost 庫

### Windows - Visual Studio

1. 下載並安裝 Boost: https://www.boost.org/
2. 解壓到例如 `C:\boost_1_81_0`
3. 在 Visual Studio 中：
   - 右鍵點擊專案 > Properties
   - C/C++ > General > Additional Include Directories
   - 添加: `C:\boost_1_81_0`
   - Linker > General > Additional Library Directories
   - 添加: `C:\boost_1_81_0\lib`

或者設置環境變量：
```
BOOST_ROOT=C:\boost_1_81_0
```

### 使用 vcpkg (推薦)

```powershell
# 安裝 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安裝 Boost
.\vcpkg install boost-asio:x64-windows

# 整合到 Visual Studio
.\vcpkg integrate install
```

## 編譯設置

確保專案設置：
- **C++ Language Standard**: C++17 或更高
- **Platform**: x64
- **Configuration**: Release (推薦) 或 Debug

### 在專案屬性中設置 C++17

1. 右鍵點擊專案 > Properties
2. C/C++ > Language > C++ Language Standard
3. 選擇 `ISO C++17 Standard (/std:c++17)` 或更高

## 編譯

1. 選擇 **Release** 和 **x64** 配置
2. 按 `F7` 或 **Build > Build Solution**
3. 編譯成功後，可執行文件位於：
   - `x64\Release\TexasHoldem.exe`

## 常見編譯錯誤

### 找不到 Boost 頭文件

**錯誤**: `fatal error C1083: Cannot open include file: 'boost/asio.hpp'`

**解決**: 
- 確保已安裝 Boost
- 檢查 Include Directories 設置
- 或使用 vcpkg 安裝並整合

### C++17 特性錯誤

**錯誤**: `error C2429: language feature 'nested-namespace-definition' requires compiler flag '/std:c++17'`

**解決**: 
- 在專案屬性中設置 C++ Language Standard 為 C++17

### 鏈接錯誤

**錯誤**: `unresolved external symbol`

**解決**: 
- 確保所有 `.cpp` 文件都已添加到專案
- 檢查 Boost 庫路徑設置

## 運行

編譯成功後，運行：

```powershell
.\x64\Release\TexasHoldem.exe 8888
```

或使用啟動腳本：

```powershell
.\start_server.bat
```
