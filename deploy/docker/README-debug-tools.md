# 容器内轻量调试工具说明

为便于在 Pod 内用 `kubectl exec` 排障，运行阶段镜像中加入了少量轻量工具，无需单独维护 debug 镜像。

## 已安装工具（Ubuntu 系）

| 包名             | 提供命令 | 用途           |
|------------------|----------|----------------|
| curl             | curl     | HTTP/WebSocket 探测 |
| iproute2         | ss       | 查看监听/连接（替代 netstat） |
| procps           | ps       | 查看进程        |
| netcat-openbsd   | nc       | 端口连通性测试（如 `nc -zv localhost 9090`） |

## 在其他 Dockerfile 中复用

若某服务使用独立 Dockerfile（非统一 `deploy/docker/Dockerfile`），可在**运行阶段**的 `RUN apt-get install` 中追加：

```dockerfile
# 轻量调试工具（可选）
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    iproute2 \
    procps \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*
```

与现有 `apt-get install` 合并到同一层即可，避免多一层 RUN。

## 常用示例

```bash
# 看本机监听端口
kubectl exec -n <ns> <pod> -- ss -tlnp

# 探测 HTTP/WS
kubectl exec -n <ns> <pod> -- curl -s -o /dev/null -w "%{http_code}" http://localhost:9090

# 端口是否通
kubectl exec -n <ns> <pod> -- nc -zv localhost 9090
```

## 生产环境可选

若希望生产镜像更小、不包含调试工具，可：

- 使用构建参数在统一 Dockerfile 中条件安装；或  
- 维护“带调试工具”的 tag（如 `:debug`），仅排查时使用。
