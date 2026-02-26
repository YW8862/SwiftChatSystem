# 从仓库根目录构建: docker build -f backend/gatesvr/Dockerfile .
# 或使用统一构建: docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=gatesvr -t swift/gatesvr:latest .

FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake git curl unzip tar pkg-config && rm -rf /var/lib/apt/lists/*
ENV VCPKG_ROOT=/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /vcpkg && /vcpkg/bootstrap-vcpkg.sh -disableMetrics
WORKDIR /app
COPY . .
RUN cd /app && /vcpkg/vcpkg install --triplet x64-linux \
    && mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && make gatesvr -j$(nproc)

FROM ubuntu:22.04
# 运行时 + 轻量调试工具（容器内 kubectl exec 排障用）
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    curl \
    iproute2 \
    procps \
    netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/build/backend/gatesvr/gatesvr .
RUN mkdir -p /data/logs && chmod +x /app/gatesvr
EXPOSE 9090 9091
CMD ["./gatesvr"]
