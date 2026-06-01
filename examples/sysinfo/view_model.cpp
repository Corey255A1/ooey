#include "view_model.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

SystemMonitorViewModel::SystemMonitorViewModel(std::shared_ptr<gooey::ThemeManager> theme_manager)
    : theme_manager_(std::move(theme_manager)) {
    prev_cpu_ = read_cpu_usage();
    update_metrics(1.0f);
}

void SystemMonitorViewModel::update(float dt) {
    elapsed_time_ += dt;
    if (elapsed_time_ >= 1.0f) {
        update_metrics(elapsed_time_);
        elapsed_time_ = 0.0f;
    }
}

void SystemMonitorViewModel::set_theme(const std::string& name) {
    if (theme_manager_) {
        theme_manager_->set_active_theme(name);
    }
}

void SystemMonitorViewModel::update_metrics(float dt) {
    // CPU
    CPUUsage curr_cpu = read_cpu_usage();
    float cpu_pct = calculate_cpu_percent(prev_cpu_, curr_cpu);
    prev_cpu_ = curr_cpu;

    std::stringstream ss_cpu;
    ss_cpu << std::fixed << std::setprecision(1) << cpu_pct << " %";
    cpu_text.set(ss_cpu.str());
    cpu_desc.set(cpu_pct > 80.0f ? "High System Load" : "CPU Activity Stable");

    // RAM
    size_t ram_total = 0, ram_free = 0;
    read_ram_usage(ram_total, ram_free);
    size_t ram_used = ram_total - ram_free;
    double ram_used_gb = ram_used / (1024.0 * 1024.0 * 1024.0);
    double ram_total_gb = ram_total / (1024.0 * 1024.0 * 1024.0);
    
    std::stringstream ss_ram;
    ss_ram << std::fixed << std::setprecision(2) << ram_used_gb << " GB / " << ram_total_gb << " GB";
    ram_text.set(ss_ram.str());
    
    float ram_pct = ram_total > 0 ? (100.0f * ram_used / ram_total) : 0.0f;
    std::stringstream ss_ram_desc;
    ss_ram_desc << std::fixed << std::setprecision(1) << ram_pct << "% Memory Active";
    ram_desc.set(ss_ram_desc.str());

    // Disk
    size_t disk_total = 0, disk_free = 0;
    read_disk_usage(disk_total, disk_free);
    size_t disk_used = disk_total - disk_free;
    double disk_used_gb = disk_used / (1024.0 * 1024.0 * 1024.0);
    double disk_total_gb = disk_total / (1024.0 * 1024.0 * 1024.0);
    
    std::stringstream ss_disk;
    ss_disk << std::fixed << std::setprecision(1) << disk_used_gb << " GB / " << disk_total_gb << " GB";
    disk_text.set(ss_disk.str());
    
    float disk_pct = disk_total > 0 ? (100.0f * disk_used / disk_total) : 0.0f;
    std::stringstream ss_disk_desc;
    ss_disk_desc << std::fixed << std::setprecision(1) << disk_pct << "% Disk Space Active";
    disk_desc.set(ss_disk_desc.str());

    // Processes
    auto procs = read_process_list(dt, ticks_cache_);
    std::vector<std::vector<std::string>> rows;
    size_t limit = std::min(procs.size(), size_t(100));
    for (size_t i = 0; i < limit; ++i) {
        const auto& p = procs[i];
        double mem_mb = p.memory_bytes / (1024.0 * 1024.0);
        
        std::stringstream ss_proc_cpu;
        ss_proc_cpu << std::fixed << std::setprecision(1) << p.cpu_usage << " %";

        std::stringstream ss_proc_mem;
        ss_proc_mem << std::fixed << std::setprecision(1) << mem_mb << " MB";

        rows.push_back({
            std::to_string(p.pid),
            p.name,
            ss_proc_cpu.str(),
            ss_proc_mem.str(),
            p.status
        });
    }
    process_rows.set(rows);
}
