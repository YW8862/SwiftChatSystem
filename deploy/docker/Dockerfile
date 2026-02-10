# SwiftChatSystem 后端服务统一镜像构建（使用系统 apt 依赖，无需 vcpkg/GitHub）
# 从仓库根目录构建: docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=authsvr -t swift/authsvr:latest .
# 支持: authsvr, onlinesvr, friendsvr, chatsvr, filesvr, zonesvr, gatesvr

ARG BUILD_TARGET=authsvr
ARG BASE_IMAGE=docker.1ms.run/library/ubuntu:22.04

# ========== 构建阶段 ==========
FROM ${BASE_IMAGE} AS builder

ARG BUILD_TARGET

# 构建工具 + 全部运行时依赖（apt 源，不依赖 GitHub）
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
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    librocksdb-dev \
    libboost-all-dev \
    libssl-dev \
    libhiredis-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# jwt-cpp（头文件库）：优先 Gitee 镜像，失败则尝试 GitHub 代理
RUN git clone --depth 1 https://gitee.com/mirrors/jwt-cpp.git /opt/jwt-cpp \
    || git clone --depth 1 https://ghproxy.com/https://github.com/Thalhammer/jwt-cpp.git /opt/jwt-cpp

# 复制仓库（构建上下文需为仓库根目录）
WORKDIR /app
COPY . .

# 使用系统依赖编译（不用 vcpkg），关闭单元测试以省去 GTest 依赖
RUN mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-I/opt/jwt-cpp/include" \
        -DBUILD_AUTHSVR_TESTS=OFF \
        -DBUILD_FRIENDSVR_TESTS=OFF \
        -DBUILD_CHATSVR_TESTS=OFF \
        -DBUILD_FILESVR_TESTS=OFF \
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

RUN mkdir -p /data/logs /data/files && chmod +x /app/app

EXPOSE 9090 9091 9092 9094 9095 9096 9098 9100 8080

CMD ["./app"]
