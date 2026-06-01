#include "view_model.hpp"
#include "gooey/application.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

SystemMonitorViewModel::SystemMonitorViewModel(std::shared_ptr<gooey::ThemeManager> theme_manager)
    : theme_manager_(std::move(theme_manager)) {
    prev_cpu_ = read_cpu_usage();
    running_ = true;
    worker_thread_ = std::thread(&SystemMonitorViewModel::run_worker, this);
}

SystemMonitorViewModel::~SystemMonitorViewModel() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void SystemMonitorViewModel::set_theme(const std::string& name) {
    if (theme_manager_) {
        theme_manager_->set_active_theme(name);
    }
}

void SystemMonitorViewModel::run_worker() {
    auto last_time = std::chrono::steady_clock::now();
    while (running_) {
        // Sleep in small 100ms intervals to respond quickly to shutdown requests
        for (int i = 0; i < 10 && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (!running_) {
            break;
        }

        // If the user is actively interacting (e.g. resizing, dragging, scrolling),
        // we defer telemetry updates to ensure 100% smooth UI.
        if (gooey::Application::get_instance() && gooey::Application::get_instance()->is_user_interacting()) {
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;

        // Gather CPU Metrics
        CPUUsage curr_cpu = read_cpu_usage();
        float cpu_pct = calculate_cpu_percent(prev_cpu_, curr_cpu);
        prev_cpu_ = curr_cpu;

        std::stringstream ss_cpu;
        ss_cpu << std::fixed << std::setprecision(1) << cpu_pct << " %";
        std::string cpu_str = ss_cpu.str();
        std::string cpu_desc_str = cpu_pct > 80.0f ? "High System Load" : "CPU Activity Stable";

        // Gather RAM Metrics
        size_t ram_total = 0, ram_free = 0;
        read_ram_usage(ram_total, ram_free);
        size_t ram_used = ram_total - ram_free;
        double ram_used_gb = ram_used / (1024.0 * 1024.0 * 1024.0);
        double ram_total_gb = ram_total / (1024.0 * 1024.0 * 1024.0);
        
        std::stringstream ss_ram;
        ss_ram << std::fixed << std::setprecision(2) << ram_used_gb << " GB / " << ram_total_gb << " GB";
        std::string ram_str = ss_ram.str();
        
        float ram_pct = ram_total > 0 ? (100.0f * ram_used / ram_total) : 0.0f;
        std::stringstream ss_ram_desc;
        ss_ram_desc << std::fixed << std::setprecision(1) << ram_pct << "% Memory Active";
        std::string ram_desc_str = ss_ram_desc.str();

        // Gather Disk Metrics
        size_t disk_total = 0, disk_free = 0;
        read_disk_usage(disk_total, disk_free);
        size_t disk_used = disk_total - disk_free;
        double disk_used_gb = disk_used / (1024.0 * 1024.0 * 1024.0);
        double disk_total_gb = disk_total / (1024.0 * 1024.0 * 1024.0);
        
        std::stringstream ss_disk;
        ss_disk << std::fixed << std::setprecision(1) << disk_used_gb << " GB / " << disk_total_gb << " GB";
        std::string disk_str = ss_disk.str();
        
        float disk_pct = disk_total > 0 ? (100.0f * disk_used / disk_total) : 0.0f;
        std::stringstream ss_disk_desc;
        ss_disk_desc << std::fixed << std::setprecision(1) << disk_pct << "% Disk Space Active";
        std::string disk_desc_str = ss_disk_desc.str();

        // Gather Process list metrics
        auto procs = read_process_list(dt, ticks_cache_);
        std::vector<std::vector<std::string>> rows;
        size_t limit = std::min(procs.size(), size_t(100));
        rows.reserve(limit);
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

        // Safely marshal the property updates back to the main UI thread
        auto* app = gooey::Application::get_instance();
        if (app) {
            app->dispatch([this, cpu_str, cpu_desc_str, ram_str, ram_desc_str, disk_str, disk_desc_str, rows = std::move(rows)]() mutable {
                this->cpu_text.set(std::move(cpu_str));
                this->cpu_desc.set(std::move(cpu_desc_str));
                this->ram_text.set(std::move(ram_str));
                this->ram_desc.set(std::move(ram_desc_str));
                this->disk_text.set(std::move(disk_str));
                this->disk_desc.set(std::move(disk_desc_str));
                this->process_rows.set(std::move(rows));
            });
        }
    }
}
