#pragma once

#include <QString>

namespace client::logger {

bool Init();
void Shutdown();
QString LogDirectory();

}  // namespace client::logger
