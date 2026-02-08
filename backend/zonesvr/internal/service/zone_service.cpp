#include "zone_service.h"
#include "zone.pb.h"
#include "swift/chat_type.h"
#include "system/system_manager.h"
#include "system/auth_system.h"
#include "system/chat_system.h"
#include "system/friend_system.h"
#include "system/group_system.h"
#include "system/file_system.h"
#include "rpc/gate_rpc_client.h"
#include <swift/error_code.h>
#include <chrono>
#include <unordered_map>

namespace swift::zone {

namespace {
void SetResultError(ZoneServiceImpl::HandleClientRequestResult& r, swift::ErrorCode ec,
                    const std::string& request_id) {
    r.code = swift::ErrorCodeToInt(ec);
    r.message = swift::ErrorCodeToString(ec);
    r.request_id = request_id;
}
}  // namespace

ZoneServiceImpl::ZoneServiceImpl(std::shared_ptr<SessionStore> store, SystemManager* manager)
    : store_(std::move(store)), manager_(manager) {}

ZoneServiceImpl::~ZoneServiceImpl() = default;

bool ZoneServiceImpl::UserOnline(const std::string& user_id, const std::string& gate_id,
                                 const std::string& device_type, const std::string& device_id) {
    auto gate = store_->GetGate(gate_id);
    if (!gate) return false;
    UserSession session;
    session.user_id = user_id;
    session.gate_id = gate_id;
    session.gate_addr = gate->address;
    session.device_type = device_type;
    session.device_id = device_id;
    session.online_at = session.last_active_at =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    return store_->SetOnline(session);
}

bool ZoneServiceImpl::UserOffline(const std::string& user_id, const std::string& gate_id) {
    (void)gate_id;
    return store_->SetOffline(user_id);
}

std::optional<UserSession> ZoneServiceImpl::GetUserSession(const std::string& user_id) {
    return store_->GetSession(user_id);
}

std::vector<UserSession> ZoneServiceImpl::GetUserStatuses(const std::vector<std::string>& user_ids) {
    return store_->GetSessions(user_ids);
}

ZoneServiceImpl::RouteResult ZoneServiceImpl::RouteToUser(const std::string& user_id,
                                                          const std::string& cmd,
                                                          const std::string& payload) {
    RouteResult result{};
    auto opt = store_->GetSession(user_id);
    if (!opt) {
        result.user_online = false;
        return result;
    }
    result.user_online = true;
    result.gate_id = opt->gate_id;
    result.delivered = PushToGate(opt->gate_addr, user_id, cmd, payload);
    return result;
}

ZoneServiceImpl::BroadcastResult ZoneServiceImpl::Broadcast(const std::vector<std::string>& user_ids,
                                                           const std::string& cmd,
                                                           const std::string& payload) {
    BroadcastResult result{};
    auto sessions = store_->GetSessions(user_ids);
    result.online_count = static_cast<int>(sessions.size());
    result.delivered_count = 0;
    for (const auto& s : sessions) {
        if (PushToGate(s.gate_addr, s.user_id, cmd, payload))
            result.delivered_count++;
    }
    return result;
}

bool ZoneServiceImpl::RegisterGate(const std::string& gate_id, const std::string& address) {
    GateNode node;
    node.gate_id = gate_id;
    node.address = address;
    node.current_connections = 0;
    node.registered_at = node.last_heartbeat =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    return store_->RegisterGate(node);
}

bool ZoneServiceImpl::GateHeartbeat(const std::string& gate_id, int connections) {
    return store_->UpdateGateHeartbeat(gate_id, connections);
}

bool ZoneServiceImpl::KickUser(const std::string& user_id, const std::string& reason) {
    (void)reason;
    return store_->SetOffline(user_id);
}

std::shared_ptr<GateRpcClient> ZoneServiceImpl::GetOrCreateGateClient(const std::string& gate_addr) {
    if (gate_addr.empty()) return nullptr;
    std::lock_guard<std::mutex> lock(gate_clients_mutex_);
    auto it = gate_clients_.find(gate_addr);
    if (it != gate_clients_.end()) {
        auto& client = it->second;
        if (client->IsConnected()) return client;
        client->Disconnect();
    }
    auto client = std::make_shared<GateRpcClient>();
    if (!client->Connect(gate_addr)) return nullptr;
    client->InitStub();
    gate_clients_[gate_addr] = client;
    return client;
}

bool ZoneServiceImpl::PushToGate(const std::string& gate_addr, const std::string& user_id,
                                 const std::string& cmd, const std::string& payload) {
    auto client = GetOrCreateGateClient(gate_addr);
    if (!client) return false;
    return client->PushMessage(user_id, cmd, payload, nullptr);
}

// -----------------------------------------------------------------------------
// HandleClientRequest：按前缀表驱动分发，避免长 if-else
// -----------------------------------------------------------------------------

const std::unordered_map<std::string, ZoneServiceImpl::CmdHandlerFn>&
ZoneServiceImpl::GetPrefixHandlers() {
    static const std::unordered_map<std::string, CmdHandlerFn> kHandlers = {
        {"auth",   &ZoneServiceImpl::HandleAuth},
        {"chat",   &ZoneServiceImpl::HandleChat},
        {"friend", &ZoneServiceImpl::HandleFriend},
        {"group",  &ZoneServiceImpl::HandleGroup},
        {"file",   &ZoneServiceImpl::HandleFile},
    };
    return kHandlers;
}

ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::NotImplemented(
    const std::string& cmd, const std::string& request_id) {
    (void)cmd;
    HandleClientRequestResult result;
    SetResultError(result, swift::ErrorCode::UNSUPPORTED, request_id);
    return result;
}

ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleClientRequest(
    const std::string& conn_id,
    const std::string& user_id,
    const std::string& cmd,
    const std::string& payload,
    const std::string& request_id) {
    (void)conn_id;
    HandleClientRequestResult result;
    result.request_id = request_id;
    if (!manager_) {
        SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
        return result;
    }
    std::string prefix;
    {
        size_t dot = cmd.find('.');
        prefix = (dot != std::string::npos) ? cmd.substr(0, dot) : cmd;
    }
    const auto& handlers = GetPrefixHandlers();
    auto it = handlers.find(prefix);
    if (it != handlers.end())
        return (this->*it->second)(user_id, cmd, payload, request_id);
    return NotImplemented(cmd, request_id);
}

// ---------- auth.* ----------
ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleAuth(
    const std::string& user_id, const std::string& cmd,
    const std::string& payload, const std::string& request_id) {
    (void)user_id;
    HandleClientRequestResult result;
    result.request_id = request_id;
    auto* auth = manager_->GetAuthSystem();
    if (!auth) {
        SetResultError(result, swift::ErrorCode::SERVICE_UNAVAILABLE, request_id);
        return result;
    }
    if (cmd == "auth.login") {
        AuthLoginPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        AuthLoginResult login = auth->Login(req.username(), req.password(),
                                           req.device_id(), req.device_type());
        AuthLoginResponsePayload resp_pb;
        resp_pb.set_success(login.success);
        resp_pb.set_user_id(login.user_id);
        resp_pb.set_token(login.token);
        resp_pb.set_expire_at(login.expire_at);
        resp_pb.set_error(login.error);
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = login.success ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                                   : swift::ErrorCodeToInt(swift::ErrorCode::AUTH_FAILED);
        if (!login.error.empty()) result.message = login.error;
        else if (!login.success) result.message = swift::ErrorCodeToString(swift::ErrorCode::AUTH_FAILED);
        return result;
    }
    if (cmd == "auth.logout") {
        AuthLogoutPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        AuthLogoutResult logout = auth->Logout(req.user_id(), req.token());
        result.code = logout.success ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                                    : swift::ErrorCodeToInt(swift::ErrorCode::AUTH_FAILED);
        if (!logout.error.empty()) result.message = logout.error;
        else if (!logout.success) result.message = swift::ErrorCodeToString(swift::ErrorCode::AUTH_FAILED);
        return result;
    }
    if (cmd == "auth.validate_token") {
        AuthValidateTokenPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        std::string uid = auth->ValidateToken(req.token());
        AuthValidateTokenResponsePayload resp_pb;
        resp_pb.set_user_id(uid);
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = swift::ErrorCodeToInt(swift::ErrorCode::OK);
        return result;
    }
    return NotImplemented(cmd, request_id);
}

// ---------- chat.* ----------
ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleChat(
    const std::string& user_id, const std::string& cmd,
    const std::string& payload, const std::string& request_id) {
    HandleClientRequestResult result;
    result.request_id = request_id;
    auto* chat = manager_->GetChatSystem();
    if (!chat) {
        SetResultError(result, swift::ErrorCode::SERVICE_UNAVAILABLE, request_id);
        return result;
    }
    if (cmd == "chat.send_message") {
        ChatSendMessagePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        auto r = chat->SendMessage(req.from_user_id(), req.to_id(), req.chat_type(),
                                  req.content(), req.media_url(), req.media_type(),
                                  req.client_msg_id(), req.file_size());
        ChatSendMessageResponsePayload resp_pb;
        resp_pb.set_success(r.success);
        resp_pb.set_msg_id(r.msg_id);
        resp_pb.set_timestamp(r.timestamp);
        resp_pb.set_error(r.error);
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = r.success ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                               : swift::ErrorCodeToInt(swift::ErrorCode::MSG_SEND_FAILED);
        if (!r.error.empty()) result.message = r.error;
        else if (!r.success) result.message = swift::ErrorCodeToString(swift::ErrorCode::MSG_SEND_FAILED);

        // SendMessage 成功后，推送给在线接收者
        if (r.success && r.msg_id.size() > 0) {
            ChatMessagePushPayload push_pb;
            push_pb.set_msg_id(r.msg_id);
            push_pb.set_from_user_id(req.from_user_id());
            push_pb.set_to_id(req.to_id());
            push_pb.set_chat_type(req.chat_type());
            push_pb.set_content(req.content());
            push_pb.set_media_url(req.media_url());
            push_pb.set_media_type(req.media_type());
            push_pb.set_timestamp(r.timestamp);
            std::string push_payload;
            if (push_pb.SerializeToString(&push_payload)) {
                if (req.chat_type() == static_cast<int32_t>(swift::ChatType::PRIVATE)) {
                    // 私聊：推送给 to_id（接收方）
                    RouteToUser(req.to_id(), "chat.message", push_payload);
                } else if (req.chat_type() == static_cast<int32_t>(swift::ChatType::GROUP)) {
                    // 群聊：获取群成员，推送给除发送者外的所有在线成员
                    auto* grp = manager_->GetGroupSystem();
                    if (grp) {
                        std::vector<GroupMemberResult> members;
                        int total = 0;
                        std::string err;
                        if (grp->GetGroupMembers(req.to_id(), 0, 10000, &members, &total, &err)) {
                            for (const auto& m : members) {
                                if (m.user_id != req.from_user_id())
                                    RouteToUser(m.user_id, "chat.message", push_payload);
                            }
                        }
                    }
                }
            }
        }

        return result;
    }
    if (cmd == "chat.recall_message") {
        ChatRecallMessagePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        std::string err;
        bool ok = chat->RecallMessage(req.msg_id(), req.user_id(), &err);
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::RECALL_NOT_ALLOWED);
        if (!err.empty()) result.message = err;
        else if (!ok) result.message = swift::ErrorCodeToString(swift::ErrorCode::RECALL_NOT_ALLOWED);
        return result;
    }
    return NotImplemented(cmd, request_id);
}

// ---------- friend.* ----------
ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleFriend(
    const std::string& user_id, const std::string& cmd,
    const std::string& payload, const std::string& request_id) {
    HandleClientRequestResult result;
    result.request_id = request_id;
    auto* fr = manager_->GetFriendSystem();
    if (!fr) {
        SetResultError(result, swift::ErrorCode::SERVICE_UNAVAILABLE, request_id);
        return result;
    }
    if (cmd == "friend.add") {
        FriendAddPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = fr->AddFriend(req.user_id(), req.friend_id(), req.remark());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "friend.handle_request") {
        FriendHandleRequestPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = fr->HandleFriendRequest(req.user_id(), req.request_id(), req.accept());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "friend.remove") {
        FriendRemovePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = fr->RemoveFriend(req.user_id(), req.friend_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "friend.block") {
        FriendBlockPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = fr->BlockUser(req.user_id(), req.target_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "friend.unblock") {
        FriendBlockPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = fr->UnblockUser(req.user_id(), req.target_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    return NotImplemented(cmd, request_id);
}

// ---------- group.* ----------
ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleGroup(
    const std::string& user_id, const std::string& cmd,
    const std::string& payload, const std::string& request_id) {
    (void)user_id;
    HandleClientRequestResult result;
    result.request_id = request_id;
    auto* grp = manager_->GetGroupSystem();
    if (!grp) {
        SetResultError(result, swift::ErrorCode::SERVICE_UNAVAILABLE, request_id);
        return result;
    }
    if (cmd == "group.create") {
        GroupCreatePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        std::vector<std::string> ids(req.member_ids().begin(), req.member_ids().end());
        std::string group_id = grp->CreateGroup(req.creator_id(), req.group_name(), ids);
        GroupCreateResponsePayload resp_pb;
        resp_pb.set_success(!group_id.empty());
        resp_pb.set_group_id(group_id);
        if (group_id.empty()) resp_pb.set_error(swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR));
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = group_id.empty() ? swift::ErrorCodeToInt(swift::ErrorCode::INTERNAL_ERROR)
                                       : swift::ErrorCodeToInt(swift::ErrorCode::OK);
        if (group_id.empty()) result.message = swift::ErrorCodeToString(swift::ErrorCode::INTERNAL_ERROR);
        return result;
    }
    if (cmd == "group.dismiss") {
        GroupDismissPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = grp->DismissGroup(req.group_id(), req.operator_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::NOT_GROUP_OWNER);
        if (!ok) result.message = swift::ErrorCodeToString(swift::ErrorCode::NOT_GROUP_OWNER);
        return result;
    }
    if (cmd == "group.invite_members") {
        GroupInviteMembersPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        std::vector<std::string> ids(req.member_ids().begin(), req.member_ids().end());
        bool ok = grp->InviteMembers(req.group_id(), req.inviter_id(), ids);
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "group.remove_member") {
        GroupRemoveMemberPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = grp->RemoveMember(req.group_id(), req.operator_id(), req.member_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    if (cmd == "group.leave") {
        GroupLeavePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = grp->LeaveGroup(req.group_id(), req.user_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::UNKNOWN);
        return result;
    }
    return NotImplemented(cmd, request_id);
}

// ---------- file.* ----------
ZoneServiceImpl::HandleClientRequestResult ZoneServiceImpl::HandleFile(
    const std::string& user_id, const std::string& cmd,
    const std::string& payload, const std::string& request_id) {
    HandleClientRequestResult result;
    result.request_id = request_id;
    auto* file = manager_->GetFileSystem();
    if (!file) {
        SetResultError(result, swift::ErrorCode::SERVICE_UNAVAILABLE, request_id);
        return result;
    }
    if (cmd == "file.get_upload_token") {
        FileGetUploadTokenPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        auto tok = file->GetUploadToken(req.user_id(), req.file_name(), req.file_size());
        FileGetUploadTokenResponsePayload resp_pb;
        resp_pb.set_success(!tok.token.empty());
        resp_pb.set_upload_token(tok.token);
        resp_pb.set_upload_url(tok.upload_url);
        resp_pb.set_expire_at(tok.expire_at);
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = tok.token.empty() ? swift::ErrorCodeToInt(swift::ErrorCode::UPLOAD_FAILED)
                                        : swift::ErrorCodeToInt(swift::ErrorCode::OK);
        if (tok.token.empty()) result.message = swift::ErrorCodeToString(swift::ErrorCode::UPLOAD_FAILED);
        return result;
    }
    if (cmd == "file.get_file_url") {
        FileGetFileUrlPayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        auto url = file->GetFileUrl(req.file_id(), req.user_id());
        FileGetFileUrlResponsePayload resp_pb;
        resp_pb.set_success(!url.url.empty());
        resp_pb.set_file_url(url.url);
        resp_pb.set_file_name(url.file_name);
        resp_pb.set_file_size(url.file_size);
        resp_pb.set_content_type(url.content_type);
        resp_pb.set_expire_at(url.expire_at);
        if (!resp_pb.SerializeToString(&result.payload)) {
            SetResultError(result, swift::ErrorCode::INTERNAL_ERROR, request_id);
            return result;
        }
        result.code = url.url.empty() ? swift::ErrorCodeToInt(swift::ErrorCode::FILE_NOT_FOUND)
                                     : swift::ErrorCodeToInt(swift::ErrorCode::OK);
        if (url.url.empty()) result.message = swift::ErrorCodeToString(swift::ErrorCode::FILE_NOT_FOUND);
        return result;
    }
    if (cmd == "file.delete") {
        FileDeletePayload req;
        if (!req.ParseFromString(payload)) {
            SetResultError(result, swift::ErrorCode::INVALID_PARAM, request_id);
            return result;
        }
        bool ok = file->DeleteFile(req.file_id(), req.user_id());
        result.code = ok ? swift::ErrorCodeToInt(swift::ErrorCode::OK)
                        : swift::ErrorCodeToInt(swift::ErrorCode::FILE_NOT_FOUND);
        if (!ok) result.message = swift::ErrorCodeToString(swift::ErrorCode::FILE_NOT_FOUND);
        return result;
    }
    return NotImplemented(cmd, request_id);
}

}  // namespace swift::zone
