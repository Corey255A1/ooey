#include "metrics.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#endif

// ---------------------------------------------------------
// Windows Implementation
// ---------------------------------------------------------
#ifdef _WIN32
CPUUsage read_cpu_usage() {
    CPUUsage u{};
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        ULARGE_INTEGER i, k, us;
        i.LowPart = idle.dwLowDateTime; i.HighPart = idle.dwHighDateTime;
        k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
        us.LowPart = user.dwLowDateTime; us.HighPart = user.dwHighDateTime;
        u.idle = i.QuadPart;
        u.system = k.QuadPart - i.QuadPart;
        u.user = us.QuadPart;
    }
    return u;
}

float calculate_cpu_percent(const CPUUsage& prev, const CPUUsage& curr) {
    unsigned long long prev_idle = prev.idle;
    unsigned long long curr_idle = curr.idle;
    unsigned long long prev_total = prev.idle + prev.system + prev.user;
    unsigned long long curr_total = curr.idle + curr.system + curr.user;
    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;
    if (total_diff == 0) return 0.0f;
    return 100.0f * (total_diff - idle_diff) / total_diff;
}

void read_ram_usage(size_t& total, size_t& free) {
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem_info)) {
        total = mem_info.ullTotalPhys;
        free = mem_info.ullAvailPhys;
    } else {
        total = 8ULL * 1024 * 1024 * 1024;
        free = 4ULL * 1024 * 1024 * 1024;
    }
}

std::vector<ProcessInfo> read_process_list(double dt, std::unordered_map<int, unsigned long long>& ticks_cache) {
    std::vector<ProcessInfo> list;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(snapshot, &pe)) {
            do {
                size_t mem_bytes = 0;
                float cpu_percent = 0.0f;
                HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                if (proc) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(proc, &pmc, sizeof(pmc))) {
                        mem_bytes = pmc.WorkingSetSize;
                    }
                    
                    FILETIME creation_time, exit_time, kernel_time, user_time;
                    if (GetProcessTimes(proc, &creation_time, &exit_time, &kernel_time, &user_time)) {
                        ULARGE_INTEGER k, u;
                        k.LowPart = kernel_time.dwLowDateTime; k.HighPart = kernel_time.dwHighDateTime;
                        u.LowPart = user_time.dwLowDateTime; u.HighPart = user_time.dwHighDateTime;
                        unsigned long long total_ticks = k.QuadPart + u.QuadPart;
                        if (ticks_cache.count(pe.th32ProcessID) > 0) {
                            unsigned long long prev_ticks = ticks_cache[pe.th32ProcessID];
                            if (total_ticks >= prev_ticks && dt > 0.0) {
                                cpu_percent = static_cast<float>(((total_ticks - prev_ticks) / 10000000.0) / dt * 100.0);
                                unsigned int num_cores = std::thread::hardware_concurrency();
                                if (num_cores > 0) {
                                    cpu_percent /= num_cores;
                                }
                                if (cpu_percent > 100.0f) {
                                    cpu_percent = 100.0f;
                                }
                            }
                        }
                        ticks_cache[pe.th32ProcessID] = total_ticks;
                    }
                    CloseHandle(proc);
                }
                list.push_back({(int)pe.th32ProcessID, pe.szExeFile, cpu_percent, mem_bytes, "R"});
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    
    std::unordered_set<int> active_pids;
    for (const auto& p : list) {
        active_pids.insert(p.pid);
    }
    for (auto it = ticks_cache.begin(); it != ticks_cache.end(); ) {
        if (active_pids.count(it->first) == 0) {
            it = ticks_cache.erase(it);
        } else {
            ++it;
        }
    }

    std::sort(list.begin(), list.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memory_bytes > b.memory_bytes;
    });
    return list;
}

// ---------------------------------------------------------
// Linux & Android Implementations
// ---------------------------------------------------------
#else
CPUUsage read_cpu_usage() {
    CPUUsage u{};
    std::ifstream file("/proc/stat");
    std::string line;
    if (file && std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cpu;
        ss >> cpu >> u.user >> u.nice >> u.system >> u.idle >> u.iowait >> u.irq >> u.softirq >> u.steal;
    }
    return u;
}

float calculate_cpu_percent(const CPUUsage& prev, const CPUUsage& curr) {
    unsigned long long prev_idle = prev.idle + prev.iowait;
    unsigned long long curr_idle = curr.idle + curr.iowait;
    unsigned long long prev_non_idle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    unsigned long long curr_non_idle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;
    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;
    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;
    if (total_diff == 0) return 0.0f;
    return 100.0f * (total_diff - idle_diff) / total_diff;
}

void read_ram_usage(size_t& total, size_t& free) {
    total = 0;
    free = 0;
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (file && std::getline(file, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::stringstream ss(line.substr(9));
            ss >> total;
            total *= 1024;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            std::stringstream ss(line.substr(13));
            ss >> free;
            free *= 1024;
        } else if (free == 0 && line.rfind("MemFree:", 0) == 0) {
            std::stringstream ss(line.substr(8));
            ss >> free;
            free *= 1024;
        }
    }
}

static bool parse_proc_stat(const std::string& line, int& pid, std::string& comm, char& state, unsigned long long& utime, unsigned long long& stime, long long& rss) {
    size_t start_paren = line.find('(');
    size_t end_paren = line.rfind(')');
    if (start_paren == std::string::npos || end_paren == std::string::npos || end_paren <= start_paren) {
        return false;
    }
    
    pid = std::stoi(line.substr(0, start_paren));
    comm = line.substr(start_paren + 1, end_paren - start_paren - 1);
    
    std::string rest = line.substr(end_paren + 2);
    std::stringstream ss(rest);
    
    ss >> state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int flags;
    unsigned long minflt, cminflt, majflt, cmajflt;
    ss >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt;
    ss >> utime >> stime;
    
    long long cutime, cstime, priority, nice, num_threads, itrealvalue;
    unsigned long long starttime;
    unsigned long vsize;
    ss >> cutime >> cstime >> priority >> nice >> num_threads >> itrealvalue >> starttime >> vsize >> rss;
    
    return true;
}

std::vector<ProcessInfo> read_process_list(double dt, std::unordered_map<int, unsigned long long>& ticks_cache) {
    std::vector<ProcessInfo> list;
    try {
        long ticks_per_sec = sysconf(_SC_CLK_TCK);
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            std::string filename = entry.path().filename().string();
            if (!std::all_of(filename.begin(), filename.end(), ::isdigit)) continue;
            int pid = std::stoi(filename);
            
            std::ifstream f(entry.path() / "stat");
            std::string line;
            if (f && std::getline(f, line)) {
                int parsed_pid;
                std::string comm;
                char state;
                unsigned long long utime = 0, stime = 0;
                long long rss_pages = 0;
                if (parse_proc_stat(line, parsed_pid, comm, state, utime, stime, rss_pages)) {
                    unsigned long long total_ticks = utime + stime;
                    float cpu_percent = 0.0f;
                    if (ticks_cache.count(pid) > 0) {
                        unsigned long long prev_ticks = ticks_cache[pid];
                        if (total_ticks >= prev_ticks && dt > 0.0) {
                            cpu_percent = static_cast<float>(((total_ticks - prev_ticks) / static_cast<double>(ticks_per_sec)) / dt * 100.0);
                            unsigned int num_cores = std::thread::hardware_concurrency();
                            if (num_cores > 0) {
                                cpu_percent /= num_cores;
                            }
                            if (cpu_percent > 100.0f) {
                                cpu_percent = 100.0f;
                            }
                        }
                    }
                    ticks_cache[pid] = total_ticks;
                    size_t mem_bytes = rss_pages * 4096;
                    list.push_back({pid, comm, cpu_percent, mem_bytes, std::string(1, state)});
                }
            }
        }
    } catch (...) {}
    
    std::unordered_set<int> active_pids;
    for (const auto& p : list) {
        active_pids.insert(p.pid);
    }
    for (auto it = ticks_cache.begin(); it != ticks_cache.end(); ) {
        if (active_pids.count(it->first) == 0) {
            it = ticks_cache.erase(it);
        } else {
            ++it;
        }
    }

    std::sort(list.begin(), list.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memory_bytes > b.memory_bytes;
    });
    return list;
}
#endif

void read_disk_usage(size_t& total, size_t& free) {
    try {
        auto space = std::filesystem::space(".");
        total = space.capacity;
        free = space.free;
    } catch (...) {
        total = 256ULL * 1024 * 1024 * 1024;
        free = 128ULL * 1024 * 1024 * 1024;
    }
}
