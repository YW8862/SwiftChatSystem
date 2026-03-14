# AuthSvr 服务 Docker 镜像构建
# 使用多阶段构建优化镜像大小
# 从仓库根目录构建：docker build -f backend/authsvr/Dockerfile -t swift/authsvr:latest .

ARG BASE_IMAGE=docker.1ms.run/library/ubuntu:22.04

# ========== 编译阶段 ==========
FROM ${BASE_IMAGE} AS builder

# 安装构建工具和依赖
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
    libsnappy-dev \
    liblz4-dev \
    libbz2-dev \
    libboost-all-dev \
    libssl-dev \
    libhiredis-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# jwt-cpp（头文件库）
RUN git clone --depth 1 https://gitee.com/mirrors/jwt-cpp.git /opt/jwt-cpp \
    || git clone --depth 1 https://ghproxy.com/https://github.com/Thalhammer/jwt-cpp.git /opt/jwt-cpp

WORKDIR /app
COPY . .

# 编译 authsvr
RUN mkdir -p build && cd build \
    && cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-I/opt/jwt-cpp/include" \
        -DBUILD_AUTHSVR_TESTS=OFF \
        -DBUILD_FRIENDSVR_TESTS=OFF \
        -DBUILD_CHATSVR_TESTS=OFF \
        -DBUILD_FILESVR_TESTS=OFF \
    && make authsvr -j$(nproc) \
    && cp /app/build/backend/authsvr/authsvr /app/outbin

# ========== 运行阶段 ==========
FROM ${BASE_IMAGE}

# 运行时依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    curl \
    iproute2 \
    procps \
    netcat-openbsd \
    libgrpc++-dev \
    libprotobuf-dev \
    librocksdb-dev \
    libsnappy-dev \
    liblz4-dev \
    libbz2-dev \
    libboost-all-dev \
    libssl-dev \
    libhiredis-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/outbin ./authsvr
COPY config/authsvr.conf.example ./config/authsvr.conf

RUN mkdir -p /data/logs /data/data && chmod +x /app/authsvr

EXPOSE 9094

CMD ["./authsvr"]
