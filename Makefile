.PHONY: build test docker-all docker-clean client client-windows help
.PHONY: authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr

# 默认目标
help:
	@echo "SwiftChatSystem Build System"
	@echo "============================="
	@echo ""
	@echo "后端构建:"
	@echo "  make build              - 构建 Linux 后端"
	@echo "  make test               - 运行全部单测"
	@echo ""
	@echo "Docker 镜像构建 (单个):"
	@echo "  make authsvr            - 构建 authsvr:latest 镜像"
	@echo "  make onlinesvr          - 构建 onlinesvr:latest 镜像"
	@echo "  make friendsvr          - 构建 friendsvr:latest 镜像"
	@echo "  make chatsvr            - 构建 chatsvr:latest 镜像"
	@echo "  make filesvr            - 构建 filesvr:latest 镜像"
	@echo "  make zonesvr            - 构建 zonesvr:latest 镜像"
	@echo "  make gatesvr            - 构建 gatesvr:latest 镜像"
	@echo ""
	@echo "Docker 镜像构建 (全部):"
	@echo "  make docker/all         - 构建所有后端服务镜像"
	@echo "  make build/all          - 同 docker/all"
	@echo ""
	@echo "客户端:"
	@echo "  make client             - 构建 Linux 客户端"
	@echo "  make client-windows     - 构建 Windows 客户端"
	@echo "  make docker/client      - 构建客户端 Docker 镜像"
	@echo ""
	@echo "清理:"
	@echo "  make clean              - 清理构建产物"
	@echo "  make docker-clean       - 删除所有 SwiftChat 相关镜像"
	@echo ""

build:
	@./scripts/build-server.sh

test:
	@./scripts/run-tests.sh

# ============================================================================
# Docker 镜像构建 - 单个服务
# ============================================================================

authsvr:
	@echo "Building authsvr:latest..."
	docker build -f backend/authsvr/Dockerfile -t swift/authsvr:latest .

onlinesvr:
	@echo "Building onlinesvr:latest..."
	docker build -f backend/onlinesvr/Dockerfile -t swift/onlinesvr:latest .

friendsvr:
	@echo "Building friendsvr:latest..."
	docker build -f backend/friendsvr/Dockerfile -t swift/friendsvr:latest .

chatsvr:
	@echo "Building chatsvr:latest..."
	docker build -f backend/chatsvr/Dockerfile -t swift/chatsvr:latest .

filesvr:
	@echo "Building filesvr:latest..."
	docker build -f backend/filesvr/Dockerfile -t swift/filesvr:latest .

zonesvr:
	@echo "Building zonesvr:latest..."
	docker build -f backend/zonesvr/Dockerfile -t swift/zonesvr:latest .

gatesvr:
	@echo "Building gatesvr:latest..."
	docker build -f backend/gatesvr/Dockerfile -t swift/gatesvr:latest .

# ============================================================================
# Docker 镜像构建 - 所有服务
# ============================================================================

docker/all: authsvr onlinesvr friendsvr chatsvr filesvr zonesvr gatesvr
	@echo "All service images built successfully!"

build/all: docker/all

# ============================================================================
# 客户端构建
# ============================================================================

client:
	@echo "Building Linux client..."
	mkdir -p build-client && cd build-client \
		&& cmake .. -DCMAKE_BUILD_TYPE=Release \
		&& make -j$(nproc)

client-windows:
	@echo "Building Windows client (cross-compile)..."
	@./scripts/build-client-windows.bat

docker/client:
	@echo "Building client Docker image..."
	docker build -f deploy/docker/Dockerfile.win-client -t swift/win-client-builder .

# ============================================================================
# 清理
# ============================================================================

clean:
	@echo "Cleaning build artifacts..."
	rm -rf build/* build-client/*

docker-clean:
	@echo "Removing all SwiftChat Docker images..."
	docker rmi swift/authsvr:latest 2>/dev/null || true
	docker rmi swift/onlinesvr:latest 2>/dev/null || true
	docker rmi swift/friendsvr:latest 2>/dev/null || true
	docker rmi swift/chatsvr:latest 2>/dev/null || true
	docker rmi swift/filesvr:latest 2>/dev/null || true
	docker rmi swift/zonesvr:latest 2>/dev/null || true
	docker rmi swift/gatesvr:latest 2>/dev/null || true
	docker rmi swift/win-client-builder:latest 2>/dev/null || true
	@echo "Done."
