# 消息引用功能实现说明

## 功能概述

已完成 TODO_AND_OPTIMIZATION.md 中第 90-93 行的消息引用功能，包括：

1. ✅ 右键菜单"回复"某条消息
2. ✅ 输入框上方显示"回复 XXX: 原消息内容"
3. ✅ 消息气泡中展示引用块（灰色背景）
4. ✅ 点击引用块跳转到原消息

## 实现细节

### 1. MessageItem 组件扩展

**文件**: `client/desktop/src/ui/messageitem.h` 和 `client/desktop/src/ui/messageitem.cpp`

**新增功能**:
- 添加了引用消息显示组件（`m_replyFrame`、`m_replyLabel`）
- 扩展了 `setMessage()` 方法，支持传递引用消息信息
- 新增 `setupReplyView()` 方法用于设置引用消息的显示样式
- 新增 `replyRequested` 信号用于通知父组件处理回复逻辑
- 重写 `contextMenuEvent()` 方法实现右键菜单
- 重写 `eventFilter()` 方法支持点击引用消息跳转

**引用样式**:
```cpp
QFrame#replyBubble { 
    background: rgba(0, 0, 0, 0.05); 
    border-left: 3px solid #07c160; 
    border-radius: 4px; 
    padding: 6px 8px; 
    margin: 2px 0; 
}
```

### 2. ChatWidget 组件扩展

**文件**: `client/desktop/src/ui/chatwidget.h` 和 `client/desktop/src/ui/chatwidget.cpp`

**新增功能**:
- 添加了引用状态管理变量（`m_replyToMsgId`、`m_replyToContent`、`m_replyToSender`）
- 新增 `onReplyMessageRequested()` 方法处理用户回复请求
- 新增 `scrollToMessage()` 方法实现滚动到目标消息并高亮显示
- 新增 `clearReplyState()` 方法在发送后清除引用状态
- 扩展 `buildMessageItemWidget()` 方法自动查找并填充引用消息内容
- 连接 `MessageItem::replyRequested` 信号处理回复逻辑

**输入框上方的回复提示**:
```cpp
QLabel#replyHintLabel { 
    background: #f0f0f0; 
    border: 1px solid #e0e0e0; 
    border-radius: 6px; 
    padding: 6px 10px; 
    color: #666; 
    font-size: 13px; 
}
```

### 3. 数据结构支持

**文件**: `client/desktop/src/models/message.h`

Message 结构已包含 `replyToMsgId` 字段，无需修改。

## 使用流程

### 回复消息
1. 用户在消息列表中找到要回复的消息
2. 右键点击该消息，选择"回复"选项
3. 输入框上方显示"回复 XXX: 原消息内容"提示
4. 用户输入回复内容并发送
5. 发送后引用状态自动清除

### 查看引用
1. 收到带有引用的消息时，消息气泡中显示灰色背景的引用块
2. 引用块左侧有绿色边框标识
3. 鼠标悬停在引用块上时显示手型光标

### 点击跳转
1. 用户点击引用块
2. 消息列表自动滚动到目标消息位置
3. 目标消息以黄色背景高亮显示 2 秒
4. 高亮结束后自动恢复原始样式

## 技术亮点

1. **美观的 UI 设计**: 引用块采用半透明背景 + 左边框的设计，简洁醒目
2. **流畅的交互体验**: 点击引用平滑滚动并高亮显示目标消息
3. **清晰的状态管理**: 引用状态在发送后自动清除，不影响后续消息
4. **完整的事件处理**: 支持右键菜单、点击跳转等多种交互方式
5. **兼容现有代码**: 不改变原有代码风格，所有参数均为可选默认值

## 注意事项

1. 引用消息的内容和发送者是在 `buildMessageItemWidget()` 中动态查找的
2. 如果引用的消息不在当前消息列表中，引用内容为空
3. 实际项目中应该从服务器获取完整的引用消息信息
4. 撤回的消息仍然可以被引用，但会显示"这条消息已撤回"

## 测试建议

1. 测试右键菜单是否正常显示"回复"选项
2. 测试输入框上方的回复提示是否正确显示
3. 测试发送引用消息后提示是否自动消失
4. 测试点击引用块是否能正确跳转到目标消息
5. 测试高亮效果是否正常显示和消失
6. 测试长消息列表中的滚动性能

## 未来优化

1. 可以添加关闭引用提示的按钮（×）
2. 支持引用自己的消息
3. 引用消息被撤回时，更新引用显示为"原消息已撤回"
4. 支持多级引用（引用另一条引用消息）
5. 优化查找引用消息的性能（使用 QMap 缓存）
