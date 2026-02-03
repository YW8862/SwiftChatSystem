/**
 * @file base_system.h
 * @brief System 基类定义
 *
 * 所有子系统（AuthSystem, ChatSystem 等）都继承自此基类。
 * System 是 ZoneSvr 的接口层，负责接收请求并转发到对应的后端服务。
 *
 * 设计思想（API Gateway 模式）：
 * - ZoneSvr 作为统一入口，所有请求从这里进入
 * - 每个 System 对应一个业务领域，负责 RPC 转发
 * - System 持有对应的 RPC Client，调用后端服务
 * - 实际的业务逻辑在后端服务（AuthSvr, ChatSvr 等）中实现
 * - System 共享 SessionStore，用于消息路由和会话管理
 *
 * 注意：跨服务调用（如 ChatSvr 调用 FileSvr）在后端服务层完成，
 * 而不是在 System 层，System 只做单一服务的 RPC 转发。
 */

#pragma once

#include <memory>
#include <string>

namespace swift {
namespace zone {

// 前向声明
class SessionStore;

/**
 * @class BaseSystem
 * @brief 子系统基类
 *
 * 所有 System 必须实现：
 * - Name(): 返回系统名称
 * - Init(): 初始化（建立 RPC 连接等）
 * - Shutdown(): 关闭（清理资源）
 */
class BaseSystem {
public:
  virtual ~BaseSystem() = default;

  /// 系统名称（用于日志和调试）
  virtual std::string Name() const = 0;

  /// 初始化系统（建立 RPC 连接、加载配置等）
  virtual bool Init() = 0;

  /// 关闭系统（清理资源）
  virtual void Shutdown() = 0;

  /// 设置 SessionStore（由 SystemManager 注入）
  void SetSessionStore(std::shared_ptr<SessionStore> store) {
    session_store_ = store;
  }

protected:
  /// 共享的会话存储，用于消息路由
  std::shared_ptr<SessionStore> session_store_;
};

} // namespace zone
} // namespace swift
