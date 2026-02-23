# SwiftChatSystem 构建环境与构建步骤

## 一、Linux 下构建后台

在 **Ubuntu 22.04** 上需自行安装：gcc、cmake、gRPC、Protobuf、RocksDB、Boost、OpenSSL、nlohmann-json、spdlog、hiredis 等（或使用系统包 / vcpkg）。

构建服务端：

```bash
./scripts/build-server.sh
```

- 输出目录默认：`build/`
- 可执行文件：`build/backend/authsvr`、`onlinesvr`、`gatesvr`、`zonesvr`、`friendsvr`、`chatsvr`、`filesvr`

指定输出目录示例：

```bash
./scripts/build-server.sh build-release
```

也可在项目根目录执行 `make build`，与上述等价。

---

## 二、构建后台 Docker 镜像

在仓库根目录执行（需已安装 Docker）：

```bash
./scripts/build-backend-images.sh
```

会构建全部 7 个后端镜像（authsvr、onlinesvr、friendsvr、chatsvr、filesvr、zonesvr、gatesvr）。默认使用国内镜像源，海外可 `export BASE_IMAGE=ubuntu:22.04` 后执行。

---

## 三、Windows 下构建客户端

- **准备环境（需管理员权限）**：`scripts\setup-env-windows.ps1` 或 `scripts\setup-env-windows.bat`
- **执行构建**：`scripts\build-client-windows.ps1` 或 `scripts\build-client-windows.bat`

详见 [docs/windows-build.md](windows-build.md)。

---

## 四、单测

在项目根目录执行：

```bash
make test
```

会依次运行：配置加载测试、AuthSvr、ZoneSvr、GateSvr 的 gRPC 验收（依赖 grpcurl，未安装时仅启动服务并打印手动命令）。
