# 从仓库根目录构建: docker build -f backend/zonesvr/Dockerfile .
# 或使用统一构建: docker build -f deploy/docker/Dockerfile --build-arg BUILD_TARGET=zonesvr -t swift/zonesvr:latest .

FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake git curl unzip tar pkg-config && rm -rf /var/lib/apt/lists/*
ENV VCPKG_ROOT=/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /vcpkg && /vcpkg/bootstrap-vcpkg.sh -disableMetrics
WORKDIR /app
COPY . .
RUN cd /app && /vcpkg/vcpkg install --triplet x64-linux \
    && mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    && make zonesvr -j$(nproc)

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates libstdc++6 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/build/backend/zonesvr/zonesvr .
RUN mkdir -p /data/logs && chmod +x /app/zonesvr
EXPOSE 9092
CMD ["./zonesvr"]
