#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include "ooey/ooey.hpp"
#include "gooey/application.hpp"
#include "ooey/platform.hpp"
#include "gooey/mvvmc/view.hpp"
#include "gooey/mvvmc/theme.hpp"
#include "gooey/controls/button.hpp"
#include "gooey/controls/label.hpp"
#include "gooey/controls/list_control.hpp"
#include "gooey/controls/column.hpp"
#include "gooey/controls/row.hpp"
#include "ooey/renderer/primitives/rect_primitive.hpp"
#include "ooey/renderer/primitives/rounded_rect_primitive.hpp"

// Windows imports
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

using namespace ooey;
using namespace gooey;
using namespace gooey::controls;

// ---------------------------------------------------------
// 1. Cross-Platform Metrics Gathering Helper
// ---------------------------------------------------------
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

std::vector<ProcessInfo> read_process_list() {
    std::vector<ProcessInfo> list;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(snapshot, &pe)) {
            do {
                size_t mem_bytes = 0;
                HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                if (proc) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(proc, &pmc, sizeof(pmc))) {
                        mem_bytes = pmc.WorkingSetSize;
                    }
                    CloseHandle(proc);
                }
                list.push_back({(int)pe.th32ProcessID, pe.szExeFile, 0.0f, mem_bytes, "R"});
            } while (Process32Next(snapshot, &pe));
        }
        CloseHandle(snapshot);
    }
    std::sort(list.begin(), list.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.memory_bytes > b.memory_bytes;
    });
    return list;
}
#else
// Linux Implementations
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

std::vector<ProcessInfo> read_process_list() {
    std::vector<ProcessInfo> list;
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (!entry.is_directory()) continue;
            std::string filename = entry.path().filename().string();
            if (!std::all_of(filename.begin(), filename.end(), ::isdigit)) continue;
            int pid = std::stoi(filename);
            
            std::string comm = "unknown";
            {
                std::ifstream f(entry.path() / "comm");
                if (f) std::getline(f, comm);
            }

            size_t mem_bytes = 0;
            std::string status = "S";
            {
                std::ifstream f(entry.path() / "status");
                std::string line;
                while (f && std::getline(f, line)) {
                    if (line.rfind("State:", 0) == 0) {
                        std::stringstream ss(line.substr(6));
                        ss >> status;
                    } else if (line.rfind("VmRSS:", 0) == 0) {
                        std::stringstream ss(line.substr(6));
                        size_t val_kb = 0;
                        ss >> val_kb;
                        mem_bytes = val_kb * 1024;
                    }
                }
            }
            if (mem_bytes == 0) {
                std::ifstream f(entry.path() / "statm");
                if (f) {
                    size_t size, resident;
                    if (f >> size >> resident) {
                        mem_bytes = resident * 4096;
                    }
                }
            }
            list.push_back({pid, comm, 0.0f, mem_bytes, status});
        }
    } catch (...) {}
    
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

// ---------------------------------------------------------
// 2. Custom Styled Panel component with background drawing
// ---------------------------------------------------------
class StyledPanel : public Column {
public:
    explicit StyledPanel() {
        set_padding(15);
        set_margin(5);
        set_align_self(Align::Stretch);
    }

    void draw(ooey::IRenderTarget& target) const override {
        // Draw card background based on active styling parameters
        if (corner_radius_ > 0 || stroke_thickness_ > 0.0f) {
            RoundedRectPrimitive(layout_bounds, corner_radius_, fill_color_, stroke_color_, stroke_thickness_).draw(target);
        } else {
            RectPrimitive(layout_bounds, fill_color_).draw(target);
        }
        // Draw children on top of background
        Column::draw(target);
    }

    void apply_style(const Style& style) override {
        fill_color_ = style.fill_color;
        stroke_color_ = style.stroke_color;
        stroke_thickness_ = style.stroke_thickness;
        corner_radius_ = style.corner_radius;
        Column::apply_style(style);
    }

private:
    Color fill_color_{35, 35, 40};
    Color stroke_color_{0, 0, 0, 0};
    float stroke_thickness_{0.0f};
    int corner_radius_{8};
};

// ---------------------------------------------------------
// 3. The ViewModel (Logic & States)
// ---------------------------------------------------------
class SystemMonitorViewModel {
private:
    std::shared_ptr<ThemeManager> theme_manager_;
    CPUUsage prev_cpu_{};
    float elapsed_time_{0.0f};

public:
    explicit SystemMonitorViewModel(std::shared_ptr<ThemeManager> theme_manager)
        : theme_manager_(std::move(theme_manager)) {
        prev_cpu_ = read_cpu_usage();
        update_metrics();
    }

    Property<std::string> cpu_text{"0.0 %"};
    Property<std::string> ram_text{"0.0 GB / 0.0 GB"};
    Property<std::string> disk_text{"0.0 GB / 0.0 GB"};
    
    Property<std::string> cpu_desc{"CPU Cores Active"};
    Property<std::string> ram_desc{"0% Memory Used"};
    Property<std::string> disk_desc{"0% Disk Space Used"};
    
    Property<std::vector<std::string>> process_names;

    void update(float dt) {
        elapsed_time_ += dt;
        if (elapsed_time_ >= 1.0f) {
            elapsed_time_ = 0.0f;
            update_metrics();
        }
    }

    void set_theme(const std::string& name) {
        if (theme_manager_) {
            theme_manager_->set_active_theme(name);
        }
    }

    void update_metrics() {
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
        auto procs = read_process_list();
        std::vector<std::string> names;
        size_t limit = std::min(procs.size(), size_t(25));
        for (size_t i = 0; i < limit; ++i) {
            const auto& p = procs[i];
            double mem_mb = p.memory_bytes / (1024.0 * 1024.0);
            std::stringstream ss_proc;
            ss_proc << "PID " << std::left << std::setw(6) << p.pid 
                    << "  |  " << std::left << std::setw(15) << p.name.substr(0, 15)
                    << "  |  " << std::right << std::fixed << std::setprecision(1) << mem_mb << " MB"
                    << "  |  State: " << p.status;
            names.push_back(ss_proc.str());
        }
        process_names.set(names);
    }
};

// ---------------------------------------------------------
// 4. The View Component
// ---------------------------------------------------------
class SystemMonitorView : public Column {
public:
    explicit SystemMonitorView(std::shared_ptr<SystemMonitorViewModel> view_model)
        : view_model_(std::move(view_model)) {
        
        // Match parent size
        set_width(SizePolicy::MatchParent);
        set_height(SizePolicy::MatchParent);
        set_padding(25);
        set_style_name("window"); // Style for main window background

        // Main outer frame card
        auto main_card = std::make_shared<StyledPanel>();
        main_card->set_width(SizePolicy::MatchParent);
        main_card->set_height(SizePolicy::MatchParent);
        main_card->set_padding(20);
        main_card->set_style_name("window-card");
        add_child(main_card);

        // Dashboard Header Inside Card
        auto header = std::make_shared<Column>();
        header->set_width(SizePolicy::MatchParent);
        header->set_height(SizePolicy::WrapContent);
        header->set_margin(0, 0, 0, 15);

        auto title = std::make_shared<Label>(
            "Real-time System Monitor",
            Font{"sans-serif", 24, FontWeight::Bold},
            Point{0, 0},
            Color{255, 255, 255}
        );
        title->set_absolute(false);
        title->set_margin(0, 0, 0, 5);
        title->set_style_name("title-text");
        header->add_child(title);

        auto subtitle = std::make_shared<Label>(
            "Dynamic hardware performance monitoring dashboard & running processes list",
            Font{"sans-serif", 13},
            Point{0, 0},
            Color{140, 140, 150}
        );
        subtitle->set_absolute(false);
        subtitle->set_margin(0, 0, 0, 10);
        subtitle->set_style_name("subtitle-text");
        header->add_child(subtitle);

        main_card->add_child(header);

        // Top Row: 3 Metrics Cards
        auto metrics_row = std::make_shared<Row>();
        metrics_row->set_width(SizePolicy::MatchParent);
        metrics_row->set_height(SizePolicy::WrapContent);
        metrics_row->set_margin(0, 0, 0, 15);

        // Sub-Card 1: CPU Metrics
        auto cpu_card = std::make_shared<StyledPanel>();
        cpu_card->set_style_name("card-bg");
        cpu_card->set_width(SizePolicy::Fixed, 235.0f);
        cpu_card->set_height(SizePolicy::Fixed, 140.0f);
        cpu_card->set_margin(0, 5, 10, 5);

        auto cpu_lbl = std::make_shared<Label>("CPU UTILIZATION", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{0, 180, 240});
        cpu_lbl->set_absolute(false);
        cpu_lbl->set_margin(0, 0, 0, 5);
        cpu_lbl->set_style_name("card-header-cpu");
        cpu_card->add_child(cpu_lbl);

        auto cpu_val = std::make_shared<Label>("0.0 %", Font{"sans-serif", 22, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
        cpu_val->set_absolute(false);
        cpu_val->set_margin(0, 0, 0, 12);
        cpu_val->set_style_name("card-value-cpu");
        bind(view_model_->cpu_text, [cpu_val](const std::string& val) { cpu_val->set_text(val); });
        cpu_card->add_child(cpu_val);

        auto cpu_sec = std::make_shared<Label>("Cores Active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
        cpu_sec->set_absolute(false);
        cpu_sec->set_style_name("card-desc-text");
        bind(view_model_->cpu_desc, [cpu_sec](const std::string& val) { cpu_sec->set_text(val); });
        cpu_card->add_child(cpu_sec);

        metrics_row->add_child(cpu_card);

        // Sub-Card 2: RAM Metrics
        auto ram_card = std::make_shared<StyledPanel>();
        ram_card->set_style_name("card-bg");
        ram_card->set_width(SizePolicy::Fixed, 235.0f);
        ram_card->set_height(SizePolicy::Fixed, 140.0f);
        ram_card->set_margin(0, 5, 10, 5);

        auto ram_lbl = std::make_shared<Label>("RAM USAGE", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{235, 160, 0});
        ram_lbl->set_absolute(false);
        ram_lbl->set_margin(0, 0, 0, 5);
        ram_lbl->set_style_name("card-header-ram");
        ram_card->add_child(ram_lbl);

        auto ram_val = std::make_shared<Label>("0.0 GB / 0.0 GB", Font{"sans-serif", 15, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
        ram_val->set_absolute(false);
        ram_val->set_margin(0, 0, 0, 18);
        ram_val->set_style_name("card-value-ram");
        bind(view_model_->ram_text, [ram_val](const std::string& val) { ram_val->set_text(val); });
        ram_card->add_child(ram_val);

        auto ram_sec = std::make_shared<Label>("0% memory active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
        ram_sec->set_absolute(false);
        ram_sec->set_style_name("card-desc-text");
        bind(view_model_->ram_desc, [ram_sec](const std::string& val) { ram_sec->set_text(val); });
        ram_card->add_child(ram_sec);

        metrics_row->add_child(ram_card);

        // Sub-Card 3: Disk Metrics
        auto disk_card = std::make_shared<StyledPanel>();
        disk_card->set_style_name("card-bg");
        disk_card->set_width(SizePolicy::Fixed, 235.0f);
        disk_card->set_height(SizePolicy::Fixed, 140.0f);
        disk_card->set_margin(0, 5, 10, 5);

        auto disk_lbl = std::make_shared<Label>("DISK SPACE", Font{"sans-serif", 11, FontWeight::Bold}, Point{0, 0}, Color{180, 100, 240});
        disk_lbl->set_absolute(false);
        disk_lbl->set_margin(0, 0, 0, 5);
        disk_lbl->set_style_name("card-header-disk");
        disk_card->add_child(disk_lbl);

        auto disk_val = std::make_shared<Label>("0.0 GB / 0.0 GB", Font{"sans-serif", 15, FontWeight::Bold}, Point{0, 0}, Color{230, 230, 235});
        disk_val->set_absolute(false);
        disk_val->set_margin(0, 0, 0, 18);
        disk_val->set_style_name("card-value-disk");
        bind(view_model_->disk_text, [disk_val](const std::string& val) { disk_val->set_text(val); });
        disk_card->add_child(disk_val);

        auto disk_sec = std::make_shared<Label>("0% disk active", Font{"sans-serif", 12}, Point{0, 0}, Color{150, 150, 160});
        disk_sec->set_absolute(false);
        disk_sec->set_style_name("card-desc-text");
        bind(view_model_->disk_desc, [disk_sec](const std::string& val) { disk_sec->set_text(val); });
        disk_card->add_child(disk_sec);

        metrics_row->add_child(disk_card);

        main_card->add_child(metrics_row);

        // Bottom Row: Process List and Theme Selection
        auto bottom_row = std::make_shared<Row>();
        bottom_row->set_width(SizePolicy::MatchParent);
        bottom_row->set_height(SizePolicy::MatchParent); // Dynamically expands to fill remainder of screen
        bottom_row->set_margin(0, 0, 0, 10);

        // Left column in bottom: List Panel
        auto list_container = std::make_shared<Column>();
        list_container->set_width(SizePolicy::Fixed, 510.0f);
        list_container->set_height(SizePolicy::MatchParent);
        list_container->set_margin(0, 5, 10, 5);

        auto list_lbl = std::make_shared<Label>(
            "Top System Processes (Sorted by RSS Memory)",
            Font{"sans-serif", 13, FontWeight::Bold},
            Point{0, 0},
            Color{180, 180, 195}
        );
        list_lbl->set_absolute(false);
        list_lbl->set_margin(0, 0, 0, 5);
        list_lbl->set_style_name("section-header");
        list_container->add_child(list_lbl);

        auto proc_list = std::make_shared<ListControl>(
            Rect{0, 0, 510, 240},
            34,
            Font{"monospace", 13},
            Color{200, 200, 205},
            Color{36, 36, 42},
            Color{0, 120, 215},
            Color{255, 255, 255}
        );
        proc_list->set_absolute(false);
        proc_list->set_width(SizePolicy::MatchParent);
        proc_list->set_height(SizePolicy::MatchParent); // Fill container!
        proc_list->set_style_name("list-box");
        bind(view_model_->process_names, [proc_list](const std::vector<std::string>& list) {
            proc_list->set_items(list);
        });
        list_container->add_child(proc_list);

        bottom_row->add_child(list_container);

        // Right column in bottom: Theme Selection
        auto theme_card = std::make_shared<StyledPanel>();
        theme_card->set_style_name("card-bg");
        theme_card->set_width(SizePolicy::Fixed, 220.0f);
        theme_card->set_height(SizePolicy::MatchParent);
        theme_card->set_margin(0, 5, 10, 5);

        auto theme_lbl = std::make_shared<Label>(
            "DASHBOARD THEME",
            Font{"sans-serif", 11, FontWeight::Bold},
            Point{0, 0},
            Color{150, 150, 165}
        );
        theme_lbl->set_absolute(false);
        theme_lbl->set_margin(0, 0, 0, 12);
        theme_lbl->set_style_name("theme-header");
        theme_card->add_child(theme_lbl);

        // Layout Theme Buttons inside column
        auto btn_dark = std::make_shared<Button>(Rect{0, 0, 190, 34}, Color{45, 45, 52});
        btn_dark->set_absolute(false);
        btn_dark->set_margin(0, 0, 0, 8);
        btn_dark->set_label_text("Dark Mode");
        btn_dark->set_style_name("btn-dark");
        btn_dark->on_click = [this]() { view_model_->set_theme("dark"); };
        theme_card->add_child(btn_dark);

        auto btn_light = std::make_shared<Button>(Rect{0, 0, 190, 34}, Color{45, 45, 52});
        btn_light->set_absolute(false);
        btn_light->set_margin(0, 0, 0, 8);
        btn_light->set_label_text("Light Clean");
        btn_light->set_style_name("btn-light");
        btn_light->on_click = [this]() { view_model_->set_theme("light"); };
        theme_card->add_child(btn_light);

        auto btn_hacker = std::make_shared<Button>(Rect{0, 0, 190, 34}, Color{45, 45, 52});
        btn_hacker->set_absolute(false);
        btn_hacker->set_margin(0, 0, 0, 8);
        btn_hacker->set_label_text("Hacker Green");
        btn_hacker->set_style_name("btn-hacker");
        btn_hacker->on_click = [this]() { view_model_->set_theme("hacker"); };
        theme_card->add_child(btn_hacker);

        auto btn_lofi = std::make_shared<Button>(Rect{0, 0, 190, 34}, Color{45, 45, 52});
        btn_lofi->set_absolute(false);
        btn_lofi->set_margin(0, 0, 0, 8);
        btn_lofi->set_label_text("Soft Lofi");
        btn_lofi->set_style_name("btn-lofi");
        btn_lofi->on_click = [this]() { view_model_->set_theme("lofi"); };
        theme_card->add_child(btn_lofi);

        bottom_row->add_child(theme_card);

        main_card->add_child(bottom_row);

        // Footnote inside card
        auto footnote = std::make_shared<Label>(
            "Note: Processes list refreshed dynamically in real time (once per second). Responsive Layout Flow enabled.",
            Font{"sans-serif", 11},
            Point{0, 0},
            Color{110, 110, 120}
        );
        footnote->set_absolute(false);
        footnote->set_style_name("footnote-text");
        main_card->add_child(footnote);
    }

private:
    std::shared_ptr<SystemMonitorViewModel> view_model_;
};

// ---------------------------------------------------------
// 5. Main Entry (Theme Definition and Application Bootstrap)
// ---------------------------------------------------------
int main() {
    std::cout << "Starting OOEY Real-Time System Monitor Dashboard...\n";

    Application app;

    auto backend = create_default_window_backend();
    if (!backend || !backend->create({900, 700}, "OOEY Live System Monitor")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }
    app.set_window_backend(std::move(backend));

    auto theme_manager = std::make_shared<ThemeManager>();

    // --- 1. DARK THEME DEFINITION ---
    auto dark_theme = std::make_shared<Theme>();
    dark_theme->name = "dark";
    dark_theme->set_style("window", Style{Color{18, 18, 22}});
    dark_theme->set_style("window-card", Style{Color{28, 28, 33}, Color{55, 55, 65}, 1.5f, Color{0,0,0,0}, 16});
    dark_theme->set_style("card-bg", Style{Color{23, 23, 27}, Color{46, 46, 54}, 1.0f, Color{0,0,0,0}, 10});
    dark_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}});
    dark_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{150, 150, 160}});
    dark_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 180, 240}});
    dark_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{235, 160, 0}});
    dark_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{180, 100, 240}});
    dark_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 110}});
    dark_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{245, 175, 40}});
    dark_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 120, 255}});
    dark_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{145, 145, 155}});
    dark_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 200, 210}});
    dark_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{150, 150, 165}});
    dark_theme->set_style("list-box", Style{Color{20, 20, 24}, Color{50, 50, 60}, 1.5f, Color{210, 210, 215}, 8});
    dark_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 120, 130}});
    dark_theme->set_style("btn-dark", Style{Color{0, 120, 215}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 6});
    dark_theme->set_style("btn-light", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    dark_theme->set_style("btn-hacker", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    dark_theme->set_style("btn-lofi", Style{Color{45, 45, 52}, Color{75, 75, 85}, 1.5f, Color{200, 200, 205}, 6});
    theme_manager->add_theme("dark", dark_theme);

    // --- 2. LIGHT CLEAN THEME DEFINITION ---
    auto light_theme = std::make_shared<Theme>();
    light_theme->name = "light";
    light_theme->set_style("window", Style{Color{242, 242, 247}});
    light_theme->set_style("window-card", Style{Color{255, 255, 255}, Color{215, 215, 225}, 1.5f, Color{0,0,0,0}, 16});
    light_theme->set_style("card-bg", Style{Color{248, 248, 250}, Color{220, 220, 230}, 1.0f, Color{0,0,0,0}, 10});
    light_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{20, 20, 30}});
    light_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{100, 100, 115}});
    light_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 100, 200}});
    light_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{190, 110, 0}});
    light_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 40, 180}});
    light_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 140, 70}});
    light_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{200, 120, 0}});
    light_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 40, 190}});
    light_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 90, 105}});
    light_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{60, 60, 75}});
    light_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{110, 110, 125}});
    light_theme->set_style("list-box", Style{Color{255, 255, 255}, Color{210, 210, 220}, 1.5f, Color{50, 50, 60}, 8});
    light_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{120, 120, 135}});
    light_theme->set_style("btn-dark", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    light_theme->set_style("btn-light", Style{Color{0, 90, 180}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 6});
    light_theme->set_style("btn-hacker", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    light_theme->set_style("btn-lofi", Style{Color{230, 230, 235}, Color{195, 195, 205}, 1.0f, Color{50, 50, 60}, 6});
    theme_manager->add_theme("light", light_theme);

    // --- 3. HACKER MONOCHROME GREEN THEME DEFINITION ---
    auto hacker_theme = std::make_shared<Theme>();
    hacker_theme->name = "hacker";
    hacker_theme->set_style("window", Style{Color{0, 0, 0}});
    hacker_theme->set_style("window-card", Style{Color{0, 0, 0}, Color{0, 255, 0}, 2.0f, Color{0,0,0,0}, 0});
    hacker_theme->set_style("card-bg", Style{Color{0, 0, 0}, Color{0, 200, 0}, 1.5f, Color{0,0,0,0}, 0});
    hacker_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 0}});
    hacker_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 200, 0}});
    hacker_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 255, 0}});
    hacker_theme->set_style("list-box", Style{Color{0, 0, 0}, Color{0, 255, 0}, 2.0f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{0, 180, 0}});
    hacker_theme->set_style("btn-dark", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("btn-light", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    hacker_theme->set_style("btn-hacker", Style{Color{0, 255, 0}, Color{0,0,0,0}, 0.0f, Color{0, 0, 0}, 0});
    hacker_theme->set_style("btn-lofi", Style{Color{0, 0, 0}, Color{0, 255, 0}, 1.5f, Color{0, 255, 0}, 0});
    theme_manager->add_theme("hacker", hacker_theme);

    // --- 4. SOFT WARM LOFI THEME DEFINITION ---
    auto lofi_theme = std::make_shared<Theme>();
    lofi_theme->name = "lofi";
    lofi_theme->set_style("window", Style{Color{246, 238, 233}});
    lofi_theme->set_style("window-card", Style{Color{236, 224, 218}, Color{205, 190, 183}, 1.5f, Color{0,0,0,0}, 18});
    lofi_theme->set_style("card-bg", Style{Color{241, 231, 225}, Color{212, 198, 191}, 1.0f, Color{0,0,0,0}, 12});
    lofi_theme->set_style("title-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{105, 82, 76}});
    lofi_theme->set_style("subtitle-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{135, 115, 110}});
    lofi_theme->set_style("card-header-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 92, 82}});
    lofi_theme->set_style("card-header-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{145, 102, 75}});
    lofi_theme->set_style("card-header-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{115, 90, 110}});
    lofi_theme->set_style("card-value-cpu", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-value-ram", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-value-disk", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{90, 75, 70}});
    lofi_theme->set_style("card-desc-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{130, 115, 110}});
    lofi_theme->set_style("section-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{100, 80, 75}});
    lofi_theme->set_style("theme-header", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{125, 105, 100}});
    lofi_theme->set_style("list-box", Style{Color{242, 232, 226}, Color{200, 185, 178}, 1.5f, Color{100, 82, 76}, 12});
    lofi_theme->set_style("footnote-text", Style{Color{0,0,0,0}, Color{0,0,0,0}, 0.0f, Color{140, 125, 120}});
    lofi_theme->set_style("btn-dark", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-light", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-hacker", Style{Color{228, 214, 208}, Color{195, 180, 173}, 1.0f, Color{110, 90, 85}, 12});
    lofi_theme->set_style("btn-lofi", Style{Color{130, 92, 82}, Color{0,0,0,0}, 0.0f, Color{255, 255, 255}, 12});
    theme_manager->add_theme("lofi", lofi_theme);

    // Default to dark theme
    theme_manager->set_active_theme("dark");
    app.set_theme_manager(theme_manager);

    auto view_model = std::make_shared<SystemMonitorViewModel>(theme_manager);
    auto root_view = std::make_shared<SystemMonitorView>(view_model);
    app.set_root_view(root_view);

    // Metric update ticker callback on every frame iteration, throttle to 1s in VM
    auto last_time = std::chrono::high_resolution_clock::now();
    app.set_before_render_callback([view_model, &last_time](IRenderTarget*) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        view_model->update(dt);
    });

    app.run();
    return 0;
}
