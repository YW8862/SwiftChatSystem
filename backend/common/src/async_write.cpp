/**
 * @file async_write.cpp
 * @brief 「异步写中间层」当前的同步实现。
 */

#include "swift/async_write.h"

namespace swift::async {

bool SyncWriteExecutor::Submit(const WriteTask& task) {
    // 当前阶段：不做真正的异步，仅作为统一入口，直接在调用线程执行。
    if (task.execute) {
        task.execute();
    }
    return true;
}

}  // namespace swift::async

