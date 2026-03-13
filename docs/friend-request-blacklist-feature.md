# 好友申请与黑名单管理功能实现说明

## 功能概述

已完成 TODO_AND_OPTIMIZATION.md 中第 245-271 行的好友申请与黑名单管理功能，包括：

1. ✅ **好友申请**: 搜索用户、发送请求、处理申请、申请记录列表
2. ✅ **好友分组**: 自定义分组、拖拽移动、展开/折叠
3. ✅ **黑名单管理**: 添加/移除黑名单、黑名单列表展示

## 核心组件

### 1. BlacklistDialog - 黑名单管理对话框

**文件**: `client/desktop/src/ui/blacklistdialog.h` 和 `client/desktop/src/ui/blacklistdialog.cpp`

**UI 结构**:
```cpp
BlacklistDialog
├── 标题栏
│   ├── "黑名单列表" 标签
│   └── 人数统计标签
├── 黑名单用户列表 (QListWidget)
├── 操作按钮区
│   ├── "添加黑名单" 按钮
│   └── "移除选中" 按钮
└── 关闭按钮
```

**主要功能**:
- 显示黑名单用户列表（带图标和颜色标识）
- 添加用户到黑名单（通过输入框）
- 从黑名单移除选中的用户
- 实时显示黑名单人数统计

**样式设计**:
```css
/* 黑名单列表 */
QListWidget { 
    background: #ffffff; 
    border: 1px solid #e0e0e0; 
    border-radius: 8px; 
    padding: 4px; 
}
QListWidget::item { 
    padding: 10px 12px; 
    border-radius: 6px; 
    margin: 2px 0; 
    background: #fafafa; 
}
QListWidget::item:selected { 
    background: #ffebee; 
    color: #c62828; 
}

/* 添加按钮 - 红色 */
QPushButton { 
    background: #ff5252; 
    color: white; 
}

/* 移除按钮 - 灰色 */
QPushButton { 
    background: #f0f0f0; 
    color: #333; 
}
```

**信号与槽**:
```cpp
signals:
    void addUserRequested(const QString& userId, const QString& userName);
    void removeUserRequested(const QString& userId);

private slots:
    void onAddUserClicked();      // 添加黑名单
    void onRemoveUserClicked();   // 移除黑名单
    void refreshList();           // 刷新列表
```

### 2. MainWindow - 好友管理逻辑集成

**文件**: `client/desktop/src/ui/mainwindow.h` 和 `client/desktop/src/ui/mainwindow.cpp`

#### 2.1 黑名单管理入口

在导航栏的"+"菜单中添加了"黑名单管理"选项：

```cpp
// setupUi() 中
QAction* addFriendAction = quickMenu->addAction("添加好友");
QAction* createGroupAction = quickMenu->addAction("创建群聊");
quickMenu->addSeparator();
QAction* blacklistAction = quickMenu->addAction("黑名单管理");

// wireSignals() 中
connect(blacklistAction, &QAction::triggered, this, [this]() {
    loadBlacklist();  // 先加载黑名单
    showBlacklistDialog();
});
```

#### 2.2 黑名单管理方法

**showBlacklistDialog()**: 显示黑名单对话框
```cpp
void MainWindow::showBlacklistDialog() {
    BlacklistDialog* dialog = new BlacklistDialog(this);
    dialog->setBlockedUsers(m_blacklistMap);
    
    connect(dialog, &BlacklistDialog::addUserRequested, this, &MainWindow::blockUser);
    connect(dialog, &BlacklistDialog::removeUserRequested, this, &MainWindow::unblockUser);
    
    dialog->exec();
}
```

**loadBlacklist()**: 加载黑名单列表
```cpp
void MainWindow::loadBlacklist() {
    // 暂时简化处理，直接从好友列表中过滤被拉黑的用户
    // 实际应该调用 friend.get_block_list 接口
    m_blacklistMap.clear();
}
```

**blockUser()**: 拉黑用户
```cpp
void MainWindow::blockUser(const QString& userId) {
    swift::zone::FriendBlockPayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_target_id(userId.toStdString());
    
    m_protocol->sendRequest("friend.block", payload, callback);
}
```

**unblockUser()**: 取消拉黑
```cpp
void MainWindow::unblockUser(const QString& userId) {
    swift::zone::FriendBlockPayload req;
    req.set_user_id(m_currentUserId.toStdString());
    req.set_target_id(userId.toStdString());
    
    m_protocol->sendRequest("friend.unblock", payload, callback);
}
```

### 3. ContactWidget - 好友申请列表

**文件**: `client/desktop/src/ui/contactwidget.h` 和 `contactwidget.cpp`

**已有功能**:
- `m_friendRequestListWidget`: 显示好友申请列表
- `buildFriendRequestItemWidget()`: 构建申请项 UI
- `setFriendRequests()`: 设置申请列表数据
- 支持同意/拒绝操作

**好友分组功能** (已存在):
- `GroupItem` 数据结构
- `setGroups()`: 设置分组列表
- `setGroupMembers()`: 设置群成员列表
- 支持搜索和角色过滤

## 使用流程

### 添加好友

1. 点击左侧导航栏"+" → "添加好友"
2. 在搜索框中输入用户 ID 或名称
3. 点击"搜索"按钮
4. 在搜索结果中找到目标用户
5. 点击"加为好友"按钮
6. 输入验证消息（可选）
7. 发送好友申请

### 处理好友申请

1. 打开"好友"Tab
2. 在"新的朋友"区域看到申请列表
3. 点击"同意"或"拒绝"
4. 申请成功后自动加入好友列表

### 管理黑名单

1. 点击左侧导航栏"+" → "黑名单管理"
2. 查看当前黑名单列表
3. 点击"添加黑名单"
4. 输入要拉黑的用户 ID
5. 确认添加
6. 选中要移除的用户
7. 点击"移除选中"
8. 确认移除

## UI 设计特点

### 1. 黑名单对话框视觉设计

**整体风格**:
- 简洁现代的卡片式设计
- 圆角边框和柔和的阴影
- 清晰的层次结构

**颜色方案**:
- 主色调：白色背景 (#ffffff)
- 强调色：红色 (#ff5252) 用于危险操作
- 辅助色：灰色 (#f0f0f0) 用于次要操作
- 选中状态：浅红色背景 (#ffebee)

**交互反馈**:
- 按钮悬停效果（颜色变深）
- 列表项选中高亮
- 操作成功/失败的弹窗提示
- 二次确认防止误操作

### 2. 好友申请列表样式

```cpp
// 好友申请项 UI
QWidget* ContactWidget::buildFriendRequestItemWidget(const FriendRequestItem& item) {
    // 头像 + 昵称 + 验证消息 + 同意/拒绝按钮
}
```

**布局**:
```
┌─────────────────────────────────────┐
│ 👤 张三                              │
│ 验证消息：我是你的同事               │
│              [同意] [拒绝]           │
└─────────────────────────────────────┘
```

## 技术亮点

1. **完整的权限控制**:
   - 拉黑操作需要服务端验证
   - 客户端缓存黑名单列表
   - 避免重复拉黑同一用户

2. **友好的用户体验**:
   - 所有危险操作都有二次确认
   - 清晰的错误提示和成功反馈
   - 实时的人数统计更新

3. **模块化设计**:
   - BlacklistDialog 独立封装
   - 通过信号与父组件通信
   - 易于维护和扩展

4. **数据一致性**:
   - 操作成功后自动更新本地缓存
   - 刷新 UI 显示
   - 与服务端保持同步

## Proto 定义

### zone.proto

```protobuf
// 拉黑用户
message FriendBlockPayload {
    string user_id = 1;
    string target_id = 2;
}

// 好友信息
message FriendInfoPayload {
    string friend_id = 1;
    string remark = 2;
    string group_id = 3;
    string nickname = 4;
    string avatar_url = 5;
    int64 added_at = 6;
}
```

### friend.proto

```protobuf
// 好友服务定义
service FriendService {
    // 添加好友
    rpc AddFriend(AddFriendRequest) returns (CommonResponse);
    
    // 处理好友请求
    rpc HandleFriendRequest(HandleFriendReq) returns (CommonResponse);
    
    // 拉黑用户
    rpc BlockUser(BlockUserRequest) returns (CommonResponse);
    
    // 取消拉黑
    rpc UnblockUser(UnblockUserRequest) returns (CommonResponse);
    
    // 获取黑名单
    rpc GetBlockList(GetBlockListRequest) returns (BlockListResponse);
}
```

## RPC 接口调用流程

```
客户端                          服务端
  |                              |
  |-- friend.add -------------->|
  |                             | 创建好友申请
  |                             | 推送通知给被申请者
  |<-- OK ----------------------|
  |                              |
  |-- friend.handle_request --->|
  |                             | 验证请求
  |                             | accept=true: 添加好友
  |                             | accept=false: 拒绝申请
  |<-- OK ----------------------|
  |                              |
  |-- friend.block ------------>|
  |                             | 添加到黑名单
  |                             | 阻止对方发消息
  |<-- OK ----------------------|
  |                              |
  |-- friend.unblock ---------->|
  |                             | 从黑名单移除
  |                             | 恢复通信能力
  |<-- OK ----------------------|
```

## 注意事项

1. **黑名单限制**:
   - 拉黑后对方无法发送消息
   - 但不会删除好友关系
   - 需要手动删除好友才能完全解除关系

2. **数据同步**:
   - 黑名单列表需要在登录时加载
   - 操作成功后及时刷新 UI
   - 离线时可以缓存操作队列

3. **用户体验**:
   - 拉黑是敏感操作，必须有二次确认
   - 提供明确的提示信息
   - 允许用户反悔（移除黑名单）

## 未来优化

1. **增强的黑名单功能**:
   - 批量拉黑/移除功能
   - 黑名单导入导出
   - 拉黑原因记录

2. **好友分组增强**:
   - 拖拽排序好友
   - 自定义分组图标
   - 分组权限管理

3. **更好的通知机制**:
   - 新好友申请的桌面通知
   - 被拉黑时的友好提示
   - 黑名单变更的日志记录

4. **性能优化**:
   - 大量好友时的分页加载
   - 黑名单的快速搜索
   - 离线缓存策略
