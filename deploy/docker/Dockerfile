# SwiftChatSystem 后端服务统一镜像构建
# 从仓库根目录构建: docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=authsvr -t swift/authsvr:latest .
# 支持: authsvr, onlinesvr, friendsvr, chatsvr, filesvr, zonesvr, gatesvr
# 默认使用国内镜像源；海外或已配 daemon 镜像加速时可 --build-arg BASE_IMAGE=ubuntu:22.04

ARG BUILD_TARGET=authsvr
ARG BASE_IMAGE=docker.1ms.run/library/ubuntu:22.04

# ========== 构建阶段 ==========
FROM ${BASE_IMAGE} AS builder

ARG BUILD_TARGET

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 安装 vcpkg（Gitee 镜像）；bootstrap 使用系统工具并重试，避免下载中断
ENV VCPKG_ROOT=/vcpkg
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN git clone --depth 1 https://gitee.com/mirrors/vcpkg.git /vcpkg \
    && for i in 1 2 3 4 5; do /vcpkg/bootstrap-vcpkg.sh -disableMetrics && break || { echo "Bootstrap attempt $i failed, retry in 20s..."; sleep 20; }; done

# 复制仓库（构建上下文需为仓库根目录）
WORKDIR /app
COPY . .

# 安装依赖并编译（vcpkg manifest 使用项目根 vcpkg.json）
RUN cd /app && /vcpkg/vcpkg install --triplet x64-linux \
    && mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && make ${BUILD_TARGET} -j$(nproc) \
    && cp /app/build/backend/${BUILD_TARGET}/${BUILD_TARGET} /app/outbin

# ========== 运行阶段 ==========
ARG BASE_IMAGE=docker.1ms.run/library/ubuntu:22.04
FROM ${BASE_IMAGE}

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/outbin ./app

# 日志与数据目录
RUN mkdir -p /data/logs /data/files && chmod +x /app/app

# 端口由各服务定义，此处仅示意
EXPOSE 9090 9091 9092 9094 9095 9096 9098 9100 8080

CMD ["./app"]
