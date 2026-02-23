# SwiftChatSystem 根 Makefile：构建与单测入口
# - make build   : 构建 Linux 后端（等同 ./scripts/build-server.sh）
# - make test    : 运行全部单测（配置加载、AuthSvr、ZoneSvr、GateSvr）

.PHONY: build test

build:
	@./scripts/build-server.sh

test:
	@./scripts/run-tests.sh
