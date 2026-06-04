#include "TodoViewModel.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>

TodoViewModel::TodoViewModel() : newTaskText(""), taskList(std::vector<Task>{}) {
    loadTasks();
    list_sub_ = taskList.subscribe([this](const std::vector<Task>& list) {
        bool changed = false;
        if (tasks_.size() != list.size()) {
            tasks_ = list;
            changed = true;
        } else {
            for (size_t i = 0; i < list.size(); ++i) {
                if (tasks_[i].text != list[i].text || tasks_[i].completed != list[i].completed) {
                    tasks_[i] = list[i];
                    changed = true;
                }
            }
        }
        if (changed) {
            saveTasks();
        }
    });
}

void TodoViewModel::addTask() {
    std::string text = newTaskText.get();
    // Trim whitespace
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    text.erase(std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), text.end());

    if (text.empty()) {
        return;
    }

    tasks_.push_back({text, false});
    newTaskText.set("");
    saveTasks();
    syncTasksToProperty();
}

void TodoViewModel::toggleTask(int index) {
    int target = (index >= 0) ? index : selectedIndex;
    if (target < 0 || target >= static_cast<int>(tasks_.size())) {
        std::cout << "TodoViewModel: Selection out of bounds for toggleTask: " << target << "\n";
        return;
    }
    tasks_[target].completed = !tasks_[target].completed;
    saveTasks();
    syncTasksToProperty();
}

void TodoViewModel::deleteTask(int index) {
    int target = (index >= 0) ? index : selectedIndex;
    if (target < 0 || target >= static_cast<int>(tasks_.size())) {
        std::cout << "TodoViewModel: Selection out of bounds for deleteTask: " << target << "\n";
        return;
    }
    tasks_.erase(tasks_.begin() + target);
    if (selectedIndex == target) {
        selectedIndex = -1;
    } else if (selectedIndex > target) {
        selectedIndex--;
    }
    saveTasks();
    syncTasksToProperty();
}

void TodoViewModel::selectTask(int index) {
    selectedIndex = index;
    std::cout << "TodoViewModel: Selected task index: " << selectedIndex << "\n";
}

void TodoViewModel::loadTasks() {
    tasks_.clear();
    std::ifstream ifs("tasks.txt");
    if (!ifs.is_open()) {
        // Create initial tasks if file doesn't exist to give a welcoming feel!
        tasks_.push_back({"Try the ooey file parser layout engine", true});
        tasks_.push_back({"Add more tasks to this dynamic list", false});
        tasks_.push_back({"Toggle task states and check tasks.txt file", false});
        saveTasks();
        syncTasksToProperty();
        return;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        if (line.rfind("[X] ", 0) == 0) {
            tasks_.push_back({line.substr(4), true});
        } else if (line.rfind("[ ] ", 0) == 0) {
            tasks_.push_back({line.substr(4), false});
        } else {
            tasks_.push_back({line, false});
        }
    }
    ifs.close();
    syncTasksToProperty();
}

void TodoViewModel::saveTasks() {
    std::ofstream ofs("tasks.txt");
    if (!ofs.is_open()) {
        std::cerr << "TodoViewModel: Error saving tasks to tasks.txt\n";
        return;
    }
    for (const auto& task : tasks_) {
        ofs << (task.completed ? "[X] " : "[ ] ") << task.text << "\n";
    }
    ofs.close();
}

void TodoViewModel::syncTasksToProperty() {
    taskList.set(tasks_);
}
