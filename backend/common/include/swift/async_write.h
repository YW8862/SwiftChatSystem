/**
 * @file async_write.h
 * @brief 异步写中间层接口定义（预留，当前无业务调用）。
 *
 * 设计目标：
 * - 当引入真正的异步写（队列 + 写线程，如 AsyncDbWriteExecutor）时，各 Svr 的写操作可改为
 *   通过 IWriteExecutor 提交任务，从而统一接口、控制并发与背压。
 *
 * 当前状态：
 * - FileSvr、ChatSvr 等仍直接同步写（不经过本接口），避免多余封装与拷贝。
 * - Submit()：fire-and-forget，供未来非关键写（统计、埋点等）使用，目前无调用方。
 * - SubmitAndWait()：入队并阻塞等结果，供未来强一致写使用，目前无调用方。
 * - SyncWriteExecutor 仅作为接口的默认同步实现，供测试或未切异步前的占位。
 */

#pragma once

#include <functional>
#include <string>
#include <future>
#include <stdexcept>

namespace swift::async {

/**
 * @brief 写任务类型（无返回值）。
 *
 * key 用于标识「顺序域」，例如：
 * - FileSvr: upload_id（同一上传会话内需要严格顺序追加 chunk）
 * - ChatSvr: conversation_id（同一会话内消息需要按顺序写入）
 *
 * 实际写入逻辑由 execute 回调封装，方便后续放入队列后在写线程中调用。
 */
struct WriteTask {
    /// 用于顺序与分片的 key（如 upload_id / conversation_id）
    std::string key;

    /// 业务自定义标识（可选），便于 future/回调映射，当前阶段可不使用
    std::string id;

    /// 真正执行写盘/写库的回调。当前阶段为同步调用，未来可在后台线程中执行。
    std::function<void()> execute;
};

/**
 * @brief 带结果的写任务（用于「入队 + 等结果」的同步语义）。
 *
 * Result 一般为轻量结构体或错误码，表示写入是否成功及必要信息。
 */
template <typename Result>
struct WriteTaskWithResult {
    std::string key;      ///< 顺序域 key
    std::string id;       ///< 业务自定义标识（可选）
    std::function<Result()> execute;  ///< 真正执行写入并返回结果的回调
};

/**
 * @brief 写任务执行器抽象。
 *
 * 业务侧只依赖该接口：Submit(task) 即可，不关心任务是同步执行还是进入异步队列。
 */
class IWriteExecutor {
public:
    virtual ~IWriteExecutor() = default;

    /**
     * @brief fire-and-forget：提交无返回值写任务，不等待完成。
     * 用于非关键写（如统计、埋点）；当前无业务调用，预留予 AsyncDbWriteExecutor。
     * @return true=已接受，false=被拒绝（如队列满，背压）
     */
    virtual bool Submit(const WriteTask& task) = 0;

    /**
     * @brief 入队 + 等结果：提交一个带 Result 的写任务，并返回 future。
     *
     * 典型用法：
     *   auto fut = executor.SubmitAndWait<TaskResult>({key, id, []{ ... return TaskResult{...}; }});
     *   auto res = fut.get();  // 在需要时阻塞等待，保持「返回前已落库/落盘」的强语义。
     *
     * 默认实现可以直接在当前线程执行（同步语义），
     * 异步实现则可将任务入队，由写线程执行并在完成时 set_value。
     */
    template <typename Result>
    std::future<Result> SubmitAndWait(const WriteTaskWithResult<Result>& task);
};

/**
 * @brief 同步实现：Submit 时立即执行 task.execute()，无队列、无额外线程。
 * 供测试或尚未接入 AsyncDbWriteExecutor 时占位使用。
 */
class SyncWriteExecutor : public IWriteExecutor {
public:
    bool Submit(const WriteTask& task) override;
};

// ========== IWriteExecutor 模板默认实现 ==========

template <typename Result>
std::future<Result> IWriteExecutor::SubmitAndWait(const WriteTaskWithResult<Result>& task) {
    // 默认同步实现：在当前线程直接执行 execute() 并填充 promise。
    std::promise<Result> p;
    auto fut = p.get_future();
    if (nullptr != task.execute) {
        try {
            p.set_value(task.execute());
        } catch (...) {
            try {
                p.set_exception(std::current_exception());
            } catch (...) {
                // ignore
            }
        }
    } else {
        // 若未提供 execute，则抛异常
        p.set_exception(std::make_exception_ptr(std::runtime_error("empty async write task")));
    }
    return fut;
}

}  // namespace swift::async

