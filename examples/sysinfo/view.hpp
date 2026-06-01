#pragma once

#include "gooey/controls/column.hpp"
#include "view_model.hpp"
#include <memory>

class SystemMonitorView : public gooey::controls::Column {
public:
    explicit SystemMonitorView(std::shared_ptr<SystemMonitorViewModel> view_model);

private:
    std::shared_ptr<SystemMonitorViewModel> view_model_;
};
