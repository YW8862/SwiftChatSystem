# SwiftChatSystem Service Dockerfile
# 用法: docker build --build-arg SERVICE=authsvr -t swift/authsvr .

ARG SERVICE=authsvr

FROM ubuntu:22.04

ARG SERVICE

# 安装运行时依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /app

# 复制可执行文件
COPY bin/${SERVICE} /app/${SERVICE}

# 创建日志目录
RUN mkdir -p /app/logs

# 设置权限
RUN chmod +x /app/${SERVICE}

# 暴露端口（根据服务不同）
# authsvr: 9094, chatsvr: 9098, friendsvr: 9096
# filesvr: 9100+8080, gatesvr: 9090+9091, zonesvr: 9092
EXPOSE 9090-9100 8080

# 启动服务
ENV SERVICE=${SERVICE}
CMD ["/bin/sh", "-c", "/app/${SERVICE}"]
