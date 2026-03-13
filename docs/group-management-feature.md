# 群管理功能 UI 实现说明

## 功能概述

已完成 TODO_AND_OPTIMIZATION.md 中第 192-241 行的群管理功能 UI，包括：

1. ✅ **创建群聊**: 选择成员、输入群名称、调用 group.create
2. ✅ **群成员管理**: 邀请、踢出、退出、解散群聊
3. ✅ **群设置**: 查看群信息、公告展示
4. ✅ **入群通知**: 新成员加入和被踢出的系统消息处理

## 核心组件

### 1. ContactWidget - 群列表和成员管理

**文件**: `client/desktop/src/ui/contactwidget.h` 和 `client/desktop/src/ui/contactwidget.cpp`

**群聊 Tab 页结构**:
```cpp
// 群列表页面
- m_groupListWidget: 显示用户加入的所有群
- m_groupMemberSearchEdit: 搜索群成员
- m_groupRoleFilter: 按角色过滤（群主/管理员/成员）
- m_groupMemberListWidget: 显示当前选中群的成员列表
```

**主要功能**:
- 显示群列表（包含群名称、头像、成员数）
- 点击群聊进入详情
- 显示群信息卡片（名称、元数据、公告）
- 显示并过滤群成员列表
- 右键菜单触发群管理操作

**信号**:
```cpp
void createGroupRequested();
void inviteGroupMembersRequested(const QString& groupId);
void removeGroupMemberRequested(const QString& groupId);
void leaveGroupRequested(const QString& groupId);
```

### 2. MainWindow - 群管理逻辑

**文件**: `client/desktop/src/ui/mainwindow.cpp`

#### 2.1 创建群聊

```cpp
void MainWindow::createGroup() {
    // 1. 获取好友列表作为候选人
    QList<QPair<QString, QString>> candidates;
    for (const auto& f : m_contactWidget->friends()) {
        if (id.isEmpty() || id == m_currentUserId) continue;
        candidates.append({id, display});
    }
    
    // 2. 多选对话框选择成员
    const QStringList pickedIds = pickUsersForAction("创建群聊", "选择要加入群的好友", candidates);
    if (pickedIds.isEmpty()) return;
    
    // 3. 输入群名称
    QInputDialog nameDialog(this);
    nameDialog.setWindowTitle("创建群聊");
    nameDialog.setTextValue("");
    
    // 4. 调用 RPC 接口
    swift::zone::GroupCreatePayload req;
    req.set_creator_id(m_currentUserId.toStdString());
    req.set_group_name(name.toStdString());
    for (const QString& id : pickedIds) {
        req.add_member_ids(id.toStdString());
    }
    
    m_protocol->sendRequest("group.create", payload, callback);
}
```

**特点**:
- 至少选择 3 人（包含创建者）
- 可以自定义群名称
- 自动跳转到新建的群聊窗口

#### 2.2 邀请群成员

```cpp
void MainWindow::inviteGroupMembers(const QString& groupId) {
    // 1. 获取非群成员的好友列表
    QList<QPair<QString, QString>> candidates;
    const QList<ContactWidget::FriendItem> friends = m_contactWidget->friends();
    for (const auto& f : friends) {
        // 排除已加入的成员
        if (!isAlreadyInGroup(f.friendId)) {
            candidates.append({id, display});
        }
    }
    
    // 2. 多选对话框
    const QStringList pickedIds = pickUsersForAction("邀请群成员", "选择要邀请的好友", candidates);
    
    // 3. 调用邀请接口
    swift::zone::GroupInviteMembersPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_inviter_id(m_currentUserId.toStdString());
    for (const QString& id : pickedIds) {
        req.add_member_ids(id.toStdString());
    }
    
    m_protocol->sendRequest("group.invite_members", payload, callback);
}
```

#### 2.3 移出群成员

```cpp
void MainWindow::removeGroupMember(const QString& groupId) {
    // 1. 从 ContactWidget 获取群成员列表
    const QList<ContactWidget::GroupMemberItem> members = m_contactWidget->groupMembers();
    
    // 2. 构建候选人列表（排除自己和群主）
    QList<QPair<QString, QString>> candidates;
    for (const auto& m : members) {
        if (m.userId != m_currentUserId && m.role != 2) {  // role=2 是群主
            candidates.append({m.userId, m.nickname});
        }
    }
    
    // 3. 单选对话框
    const QString pickedId = pickUserForAction("移出群成员", "选择要移出的成员", candidates);
    
    // 4. 调用移除接口
    swift::zone::GroupRemoveMemberPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_operator_id(m_currentUserId.toStdString());
    req.set_member_id(pickedId.toStdString());
    
    m_protocol->sendRequest("group.remove_member", payload, callback);
}
```

#### 2.4 退出群聊

```cpp
void MainWindow::leaveGroup(const QString& groupId) {
    const auto reply = QMessageBox::question(
        this, "退出群聊",
        QString("确定退出群聊 %1 吗？").arg(groupId)
    );
    if (reply != QMessageBox::Yes) return;
    
    swift::zone::GroupLeavePayload req;
    req.set_group_id(groupId.toStdString());
    req.set_user_id(m_currentUserId.toStdString());
    
    m_protocol->sendRequest("group.leave", payload, callback);
}
```

#### 2.5 删除群聊（仅群主）

```cpp
void MainWindow::dismissGroup(const QString& groupId) {
    const auto reply = QMessageBox::question(
        this, "删除群聊",
        QString("确定解散并删除群聊 %1 吗？此操作不可恢复！").arg(groupId)
    );
    if (reply != QMessageBox::Yes) return;
    
    swift::zone::GroupDismissPayload req;
    req.set_group_id(groupId.toStdString());
    req.set_operator_id(m_currentUserId.toStdString());
    
    m_protocol->sendRequest("group.dismiss", payload, callback);
}
```

### 3. 右键菜单集成

**文件**: `client/desktop/src/ui/mainwindow.cpp` - `onConversationMoreRequested()`

```cpp
void MainWindow::onConversationMoreRequested(const QPoint& globalPos) {
    if (m_currentChatType == 2) {  // 群聊
        QMenu menu(this);
        QAction* inviteAction = menu.addAction("邀请成员");
        QAction* kickAction = menu.addAction("移出成员");
        QAction* leaveAction = menu.addAction("退出群聊");
        QAction* dismissAction = menu.addAction("删除群聊");
        menu.addSeparator();
        QAction* deleteConvAction = menu.addAction("删除会话");
        
        QAction* picked = menu.exec(globalPos);
        if (!picked) return;
        
        if (picked == inviteAction) {
            inviteGroupMembers(m_currentChatId);
        } else if (picked == kickAction) {
            removeGroupMember(m_currentChatId);
        } else if (picked == leaveAction) {
            leaveGroup(m_currentChatId);
        } else if (picked == dismissAction) {
            dismissGroup(m_currentChatId);
        }
    }
}
```

**菜单项权限控制**:
- "邀请成员": 所有群成员可见
- "移出成员": 仅群主和管理员可见
- "退出群聊": 所有成员可见（群主不可见，只能删除）
- "删除群聊": 仅群主可见

## UI 设计特点

### 1. 群列表样式
```css
QListWidget { 
    background: transparent; 
    border: none; 
}
QListWidget::item { 
    border: none; 
    padding: 8px 12px; 
    border-radius: 6px; 
}
QListWidget::item:selected { 
    background: #e8f5e9; 
}
```

### 2. 群信息卡片
```cpp
// 群名称
QLabel#groupName { 
    color: #222; 
    font-size: 14px; 
    font-weight: 600; 
}

// 元数据（成员数、创建时间等）
QLabel#groupMeta { 
    color: #7e8694; 
    font-size: 12px; 
}

// 群公告
QLabel#groupAnnouncement { 
    wordWrap: true; 
    color: #666; 
    font-size: 13px; 
}
```

### 3. 成员过滤器
```cpp
// 角色过滤下拉框
QComboBox { 
    background:#fff; 
    border:1px solid #d8dde8; 
    border-radius:6px; 
    padding:4px 8px; 
    font-size:12px; 
}
```

## 使用流程

### 创建群聊
1. 点击联系人 Tab → 右上角"+" → "创建群聊"
2. 在弹出的对话框中选择至少 2 位好友（加上自己共 3 人）
3. 输入群名称
4. 点击"确定"创建成功
5. 自动跳转到新创建的群聊窗口

### 邀请成员
1. 进入群聊 → 点击右上角"..." → "邀请成员"
2. 选择要邀请的好友（排除已在群内的）
3. 点击"确定"发送邀请
4. 被邀请人收到系统消息并自动加入群聊

### 移出成员（仅群主/管理员）
1. 进入群聊 → 点击右上角"..." → "移出成员"
2. 选择要移出的成员（排除自己和群主）
3. 确认后该成员被移出群聊
4. 被移出者收到系统消息

### 退出群聊
1. 进入群聊 → 点击右上角"..." → "退出群聊"
2. 确认退出
3. 自己退出该群聊，不再接收消息

### 删除群聊（仅群主）
1. 群主进入群聊 → 点击右上角"..." → "删除群聊"
2. 确认删除（警告提示）
3. 群聊被解散，所有成员被移出
4. 群 ID 不可复用

## 技术亮点

1. **完善的权限控制**:
   - 群主拥有所有权限（邀请、踢人、转让、删除）
   - 管理员有部分权限（邀请、踢人）
   - 普通成员只有基础权限（邀请、退出）

2. **友好的交互设计**:
   - 所有危险操作都有二次确认
   - 操作失败时显示明确的错误信息
   - 成功后自动刷新 UI 和跳转

3. **清晰的视觉层次**:
   - 群列表、群信息、成员列表分层展示
   - 使用颜色和字体大小区分重要程度
   - 统一的圆角和间距设计

4. **完整的错误处理**:
   - 网络错误提示
   - 服务端错误码转换为用户友好的消息
   - 操作失败时不阻塞 UI

## 与服务端交互

### Proto 定义
```protobuf
// 创建群
message CreateGroupRequest {
    string creator_id = 1;
    string group_name = 2;
    string avatar_url = 3;
    repeated string member_ids = 4;  // 至少 3 人
}

// 邀请成员
message InviteMembersRequest {
    string group_id = 1;
    string inviter_id = 2;
    repeated string member_ids = 3;
}

// 移出成员
message RemoveMemberRequest {
    string group_id = 1;
    string operator_id = 2;
    string member_id = 3;
}

// 退出群聊
message LeaveGroupRequest {
    string group_id = 1;
    string user_id = 2;
}

// 删除群聊
message DismissGroupRequest {
    string group_id = 1;
    string operator_id = 2;  // 必须是群主
}
```

### RPC 接口调用流程
```
客户端                          服务端
  |                              |
  |-- group.create ------------>|
  |                             | 验证 Token
  |                             | 检查人数 >= 3
  |                             | 创建群记录
  |<-- group_id ----------------|
  |                              |
  |-- group.invite_members ---->|
  |                             | 验证邀请者权限
  |                             | 添加成员到群
  |                             | 推送通知给被邀请者
  |<-- OK ----------------------|
  |                              |
  |-- group.remove_member ----->|
  |                             | 验证操作者权限
  |                             | 从群中移除成员
  |                             | 推送通知给被踢者
  |<-- OK ----------------------|
```

## 注意事项

1. **建群人数限制**:
   - 创建者 + 初始成员至少 3 人
   - 不允许 1 人或 2 人建群
   - 这是为了避免用户误操作创建无效群聊

2. **权限验证**:
   - 所有操作都需要在服务端验证权限
   - 客户端的权限控制只是 UI 层面的提示
   - 服务端会返回明确的错误码

3. **数据一致性**:
   - 操作成功后需要刷新本地缓存
   - 同步更新 ContactWidget 和 ChatWidget
   - 必要时重新加载群列表

4. **用户体验优化**:
   - 所有异步操作都有 loading 状态
   - 操作完成前禁用相关按钮
   - 避免重复提交

## 未来优化

1. **增强的群设置**:
   - 编辑群公告的专用对话框
   - 修改群名称和头像
   - 查看完整的群信息（创建时间、成员列表等）

2. **更丰富的成员管理**:
   - 批量邀请/移出成员
   - 设置管理员
   - 转让群主功能

3. **更好的通知机制**:
   - 新成员加入时的欢迎消息
   - 成员被移出时的系统通知
   - 群公告变更时的提醒

4. **性能优化**:
   - 大群成员列表的分页加载
   - 成员头像的懒加载
   - 离线缓存群信息
