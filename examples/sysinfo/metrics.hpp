#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstddef>

struct CPUUsage {
    unsigned long long user{0};
    unsigned long long nice{0};
    unsigned long long system{0};
    unsigned long long idle{0};
    unsigned long long iowait{0};
    unsigned long long irq{0};
    unsigned long long softirq{0};
    unsigned long long steal{0};
};

struct ProcessInfo {
    int pid;
    std::string name;
    float cpu_usage;
    size_t memory_bytes;
    std::string status;
};

// Gathers active CPU tick counts
CPUUsage read_cpu_usage();

// Computes current CPU utilization percentage based on delta ticks
float calculate_cpu_percent(const CPUUsage& prev, const CPUUsage& curr);

// Gathers system memory (RAM) capacity and availability
void read_ram_usage(size_t& total, size_t& free);

// Gathers disk layout space properties
void read_disk_usage(size_t& total, size_t& free);

// Enumerates all active running system processes and calculates resource usage
std::vector<ProcessInfo> read_process_list(double dt, std::unordered_map<int, unsigned long long>& ticks_cache);
