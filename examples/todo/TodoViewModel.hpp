#pragma once

#include "gooey/mvvmc/property.hpp"
#include <string>
#include <vector>

class TodoViewModel : public gooey::mvvmc::ViewModel {
public:
    struct Task {
        std::string text;
        bool completed;
    };

    TodoViewModel();

    // Observable properties bound directly by the generated C++ view
    gooey::mvvmc::Property<std::string> newTaskText;
    gooey::mvvmc::Property<std::vector<Task>> taskList;

    // Track task selection state
    int selectedIndex = -1;

    // ViewModel action signals
    void addTask();
    void toggleTask(int index = -1);
    void deleteTask(int index = -1);
    void selectTask(int index);

private:
    std::vector<Task> tasks_;
    gooey::mvvmc::ScopedSubscription list_sub_;

    void loadTasks();
    void saveTasks();
    void syncTasksToProperty();
};
