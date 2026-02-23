# Windows 11 x64 构建说明（本机构建客户端）

本文说明在 **Windows 11 64 位** 上，从零准备环境并构建 SwiftChat 桌面客户端（仅客户端，不包含后端服务）。

---

## 路径与目录说明（项目放哪儿、依赖放哪儿）

- **项目仓库（你的代码）**  
  **放哪里都行**，不要求固定盘符。例如可以放在：
  - `D:\SwiftChatSystem`
  - `E:\dev\SwiftChatSystem`
  - `C:\Users\你的用户名\Projects\SwiftChatSystem`  
  只要保证仓库根目录下有 **`scripts`** 文件夹（里面有 `setup-env-windows.ps1`、`build-client-windows.ps1` 等），双击运行 **`scripts\build-client-windows.bat`** 或 **`scripts\setup-env-windows.bat`** 时，脚本会自动把当前目录识别为项目根目录。

- **依赖（由准备脚本安装，全部在 E 盘）**
  - **vcpkg**：`E:\vcpkg`（内含 Protobuf 等）
  - **Qt**：`E:\Qt\5.15.2\mingw81_64`（及 `E:\Qt\Tools\mingw810_64` 等）

- **构建输出**  
  在**项目根目录**下生成 **`build-windows`**，可执行文件在：  
  `build-windows\client\desktop\SwiftChat.exe`

**目录关系示意：**

```
E:\
├── vcpkg\                    ← 环境准备脚本安装
│   └── installed\x64-mingw-dynamic\...
└── Qt\
    ├── 5.15.2\mingw81_64\   ← Qt 库（环境准备脚本或手动安装）
    └── Tools\mingw810_64\    ← MinGW 编译器（随 Qt 安装）

D:\SwiftChatSystem\           ← 项目仓库（路径可自选，不必在 E 盘）
├── scripts\
│   ├── setup-env-windows.ps1
│   ├── setup-env-windows.bat
│   ├── build-client-windows.ps1
│   └── build-client-windows.bat
├── client\
├── backend\
└── build-windows\            ← 构建后生成
    └── client\desktop\SwiftChat.exe
```

---

## 一、构建机上需要准备的环境

### 1. 系统与权限

- **系统**：Windows 11，64 位。
- **权限**：环境准备脚本需**管理员权限**（用于安装软件）；构建阶段不需要管理员。
- **磁盘**：至少约 5GB 可用（Qt、vcpkg、Protobuf、构建产物）。

### 2. 必需组件一览

| 组件 | 用途 | 推荐获取方式 |
|------|------|----------------|
| **Git** | 克隆 vcpkg、仓库 | winget 或 [git-scm.com](https://git-scm.com/) |
| **CMake** | 配置与生成构建文件 | winget 或 [cmake.org](https://cmake.org/download/) |
| **Ninja** | 并行编译 | winget 或 [GitHub](https://github.com/ninja-build/ninja/releases) |
| **vcpkg** | 安装 Protobuf | 克隆到 `E:\vcpkg` 并执行 bootstrap |
| **Protobuf** | 客户端协议库 | vcpkg：`protobuf:x64-mingw-dynamic` |
| **Qt 5.15.x** | 客户端 UI（Core/Gui/Widgets/Network/WebSockets） | Qt 在线安装器 或 aqtinstall |
| **MinGW 8.1 64-bit** | 与 Qt MinGW 配套的编译器 | 随 Qt 安装（在 `E:\Qt\Tools\mingw810_64`） |

### 3. 可选（用于自动安装 Qt）

- **Python 3.9+** 与 **pip**：用于执行 `aqtinstall` 自动安装 Qt（若不用 aqt，可手动装 Qt，则不需要 Python）。

---

## 二、使用脚本一键准备环境（推荐）

### 步骤 1：用管理员打开 PowerShell

- 按 `Win + X`，选择 **“终端(管理员)”** 或 **“Windows PowerShell(管理员)”**。
- 或：在开始菜单搜索 **PowerShell** → 右键 → **以管理员身份运行**。

### 步骤 2：允许执行脚本（若尚未设置）

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

若提示确认，输入 `Y` 回车。

### 步骤 3：进入项目根目录并执行准备脚本

将仓库拷贝到 Windows 后，例如在 `D:\SwiftChatSystem`：

```powershell
cd D:\SwiftChatSystem
.\scripts\setup-env-windows.ps1
```

脚本将依次：

1. 检查/安装 **winget**（若无则提示）。
2. 用 winget 安装 **Git、CMake、Ninja、Python 3.12**（已安装则跳过）。
3. 克隆 **vcpkg** 到 `E:\vcpkg` 并执行 `bootstrap-vcpkg.bat`。
4. 执行 **vcpkg install protobuf:x64-mingw-dynamic**。
5. 设置用户环境变量 **VCPKG_ROOT=E:\vcpkg**。
6. 若未检测到 Qt 5.15.2 MinGW，且本机有 **pip**，则用 **aqtinstall** 安装 Qt 5.15.2 MinGW 到 `E:\Qt`；若无 pip，会提示你**手动安装 Qt**。

### 步骤 4：无 pip 时手动安装 Qt（若脚本提示）

1. 打开 [Qt 在线安装器](https://www.qt.io/download-qt-installer)。
2. 安装时在 **Qt 5.15.2** 下勾选 **MinGW 8.1.0 64-bit**（或名称中含 `mingw` 的 64 位套件）。
3. 安装路径建议：**E:\Qt**（与脚本默认一致，依赖全部安装到 E 盘）。
4. 安装完成后，Qt 路径通常为：`E:\Qt\5.15.2\mingw81_64`。

---

## 三、环境变量说明（脚本会写入，也可手动设置）

在**新开**的 PowerShell 或命令提示符中构建时，若未自动生效，可手动设置：

```powershell
$env:VCPKG_ROOT = "E:\vcpkg"
$env:Qt5_DIR    = "E:\Qt\5.15.2\mingw81_64"   # 与你的 Qt 安装路径一致
```

- **VCPKG_ROOT**：vcpkg 根目录，构建脚本用来找 Protobuf。
- **Qt5_DIR**：Qt 5 的 MinGW 64 位目录（其下存在 `lib\cmake\Qt5`）。

---

## 四、如何进行构建

### 前提

- 已完成 **“二、使用脚本一键准备环境”**，且至少：
  - vcpkg 与 Protobuf 已安装；
  - Qt 5.15.2 MinGW 已安装在 `E:\Qt\5.15.2\mingw81_64`（或已设置 `Qt5_DIR`）。

### 步骤 1：打开终端并进入项目根目录

```powershell
cd D:\SwiftChatSystem
```

（路径按你实际拷贝位置修改。）

### 步骤 2：执行构建脚本

```powershell
.\scripts\build-client-windows.ps1
```

或通过 cmd 调用 PowerShell：

```cmd
scripts\build-client-windows.bat
```

脚本会：

1. 自动在常见路径查找 Qt5（含 `E:\Qt\5.15.2\mingw81_64` 和 `Qt5_DIR`）。
2. 若使用 MinGW 版 Qt，自动把 **Qt 自带的 MinGW**（如 `E:\Qt\Tools\mingw810_64\bin`）加入 PATH，供 CMake 使用 g++。
3. 使用 vcpkg 的 **x64-mingw-dynamic** 或 **x64-windows**（与 Qt 类型匹配）下的 Protobuf。
4. 在项目下创建 **build-windows**，用 CMake + Ninja 编译，生成 **SwiftChat.exe**。

### 步骤 3：构建成功后的输出位置

- 可执行文件：**`build-windows\client\desktop\SwiftChat.exe`**

---

## 五、运行与部署客户端

### 1. 部署 Qt 运行时 DLL

首次运行前，需要把 Qt 的 DLL 放到 exe 同目录（或系统 PATH），推荐用 **windeployqt**：

```powershell
E:\Qt\5.15.2\mingw81_64\bin\windeployqt.exe build-windows\client\desktop\SwiftChat.exe
```

（若 Qt 不在 `E:\Qt\5.15.2\mingw81_64`，请改为你的 `Qt5_DIR\bin\windeployqt.exe`。）

### 2. Protobuf DLL（使用动态库时）

若 vcpkg 安装的是 **x64-mingw-dynamic**，需将 **libprotobuf.dll** 等复制到 exe 同目录：

```powershell
copy E:\vcpkg\installed\x64-mingw-dynamic\bin\*.dll build-windows\client\desktop\
```

### 3. 运行

- 双击 **SwiftChat.exe**，或在该目录打开命令行执行：

```powershell
.\build-windows\client\desktop\SwiftChat.exe
```

---

## 六、常见问题

### 1. “未找到 Qt5”

- 确认已安装 Qt 5.15.x，且为 **MinGW 64-bit** 或 **MSVC 64-bit**。
- 设置：`$env:Qt5_DIR = "E:\Qt\5.15.2\mingw81_64"`（路径按实际修改）。

### 2. “请先执行: vcpkg install protobuf:x64-mingw-dynamic”

- 使用 **MinGW 版 Qt** 时，必须安装 **x64-mingw-dynamic** 的 Protobuf：
  ```powershell
  C:\vcpkg\vcpkg install protobuf:x64-mingw-dynamic
  ```
- 若使用 **MSVC 版 Qt**，则安装：`vcpkg install protobuf:x64-windows`，构建脚本会按 Qt 类型自动选 triplet。

### 3. CMake 报错找不到编译器

- 使用 MinGW Qt 时，构建脚本会尝试把 **Qt\Tools\mingw810_64\bin** 加入 PATH；若仍报错，请从开始菜单打开 **“Qt 5.15.2 64-bit for Desktop (MinGW 8.1.0)”**，在该终端中再执行：
  ```powershell
  cd D:\SwiftChatSystem
  .\scripts\build-client-windows.ps1
  ```

### 4. 脚本无法执行（ExecutionPolicy）

- 当前用户允许脚本：
  ```powershell
  Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
  ```
- 或单次绕过：  
  `powershell -ExecutionPolicy Bypass -File .\scripts\build-client-windows.ps1`

### 5. 网络问题导致 vcpkg / aqt 失败

- vcpkg：可配置代理或使用国内镜像（如设置 `X_VCPKG_*` 等，见 vcpkg 文档）。
- aqt：若下载 Qt 很慢，可改为使用 **Qt 在线安装器** 手动安装 Qt，再重新运行准备脚本或直接构建。

---

## 七、构建方式说明

当前仅保留 **在 Windows 本机构建**：在 Win11 x64 上安装 Qt、vcpkg、Protobuf、CMake、Ninja 后，执行 `scripts\build-client-windows.ps1`（或 `scripts\build-client-windows.bat`），得到 `build-windows\client\desktop\SwiftChat.exe`。
