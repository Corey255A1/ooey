#pragma once

#include "gooey/mvvmc/property.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "metrics.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class SystemMonitorViewModel {
private:
    std::shared_ptr<gooey::ThemeManager> theme_manager_;
    CPUUsage prev_cpu_{};
    float elapsed_time_{0.0f};
    std::unordered_map<int, unsigned long long> ticks_cache_;

public:
    explicit SystemMonitorViewModel(std::shared_ptr<gooey::ThemeManager> theme_manager);

    gooey::Property<std::string> cpu_text{"0.0 %"};
    gooey::Property<std::string> ram_text{"0.0 GB / 0.0 GB"};
    gooey::Property<std::string> disk_text{"0.0 GB / 0.0 GB"};
    
    gooey::Property<std::string> cpu_desc{"CPU Cores Active"};
    gooey::Property<std::string> ram_desc{"0% Memory Used"};
    gooey::Property<std::string> disk_desc{"0% Disk Space Used"};
    
    gooey::Property<std::vector<std::vector<std::string>>> process_rows;

    void update(float dt);
    void set_theme(const std::string& name);
    void update_metrics(float dt);
};
