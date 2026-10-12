// Roll No: MT26159
// Name: Divyansh Patel
#pragma once

#include <map>
#include <string>
#include "bookmgmt/Budget.h"
#include <stdexcept>

namespace bookmgmt {

class DepartmentBudgets {
public:
    void addDepartment(const std::string& department, Money total) {
        if (department.empty())
            throw std::invalid_argument("department name cannot be empty");

        if (budgets_.count(department))
            throw std::invalid_argument("department already exists");

        budgets_.emplace(department, Budget(total));
    }

    Budget& get(const std::string& department) {
        auto it = budgets_.find(department);
        if (it == budgets_.end())
            throw std::invalid_argument("unknown department: " + department);
        return it->second;
    }

    const Budget& get(const std::string& department) const {
        auto it = budgets_.find(department);
        if (it == budgets_.end())
            throw std::invalid_argument("unknown department: " + department);
        return it->second;
    }

    bool contains(const std::string& department) const {
        return budgets_.count(department) > 0;
    }

    std::size_t size() const {
        return budgets_.size();
    }

private:
    std::map<std::string, Budget> budgets_;
};

} // namespace bookmgmt