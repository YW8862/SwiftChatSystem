#include "file_sink.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

namespace asynclog {

FileSink::FileSink(const std::string& log_dir,
                   const std::string& file_prefix,
                   size_t max_file_size,
                   int max_file_count)
    : log_dir_(log_dir)
    , file_prefix_(file_prefix)
    , max_file_size_(max_file_size)
    , max_file_count_(max_file_count)
    , current_file_size_(0) {
    
    EnsureDirectory();
    OpenNewFile();
}

FileSink::~FileSink() {
    Close();
}

void FileSink::Write(const std::string& formatted_log) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!file_.is_open()) {
        if (!OpenNewFile()) {
            return;
        }
    }
    
    file_ << formatted_log;
    current_file_size_ += formatted_log.size();
    
    CheckRotate();
}

void FileSink::WriteBatch(const std::vector<std::string>& logs) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!file_.is_open()) {
        if (!OpenNewFile()) {
            return;
        }
    }
    
    for (const auto& log : logs) {
        file_ << log;
        current_file_size_ += log.size();
    }
    
    CheckRotate();
}

void FileSink::Flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

void FileSink::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void FileSink::CheckRotate() {
    if (current_file_size_ >= max_file_size_) {
        RotateFile();
    }
}

void FileSink::RotateFile() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    
    CleanupOldFiles();
    OpenNewFile();
}

bool FileSink::OpenNewFile() {
    std::string file_path = GenerateFileName();
    file_.open(file_path, std::ios::app);
    
    if (file_.is_open()) {
        current_file_path_ = file_path;
        current_file_size_ = 0;
        
        // 获取已有文件大小
        file_.seekp(0, std::ios::end);
        current_file_size_ = static_cast<size_t>(file_.tellp());
        return true;
    }
    
    return false;
}

std::string FileSink::GenerateFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    
    std::ostringstream oss;
    oss << log_dir_ << "/"
        << file_prefix_ << "_"
        << std::put_time(&tm_buf, "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << ms.count()
        << ".log";
    
    return oss.str();
}

void FileSink::CleanupOldFiles() {
    std::vector<std::string> log_files;
    
#ifdef _WIN32
    WIN32_FIND_DATAA find_data;
    std::string pattern = log_dir_ + "/" + file_prefix_ + "_*.log";
    HANDLE handle = FindFirstFileA(pattern.c_str(), &find_data);
    
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            log_files.push_back(log_dir_ + "/" + find_data.cFileName);
        } while (FindNextFileA(handle, &find_data));
        FindClose(handle);
    }
#else
    DIR* dir = opendir(log_dir_.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.find(file_prefix_) == 0 && 
                filename.rfind(".log") == filename.size() - 4) {
                log_files.push_back(log_dir_ + "/" + filename);
            }
        }
        closedir(dir);
    }
#endif
    
    // 按文件名排序（包含时间戳，所以字典序就是时间序）
    std::sort(log_files.begin(), log_files.end());
    
    // 删除多余的旧文件
    while (static_cast<int>(log_files.size()) > max_file_count_ - 1) {
        std::remove(log_files.front().c_str());
        log_files.erase(log_files.begin());
    }
}

bool FileSink::EnsureDirectory() {
    // 简单的目录创建（不处理多级目录）
    MKDIR(log_dir_.c_str());
    return true;
}

}  // namespace asynclog
