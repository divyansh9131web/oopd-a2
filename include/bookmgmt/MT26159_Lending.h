// Roll No: MT26159
// Name: Divyansh Patel
#pragma once

#include <map>
#include <set>
#include <string>

#include "bookmgmt/Catalog.h"

namespace bookmgmt {

// Q14: manages print borrowing and electronic-resource sessions.
class LendingManager {
public:
    explicit LendingManager(Catalog& catalog) : catalog_(catalog) {}

    void borrow(const std::string& patronId, const std::string& resourceId);
    void returnResource(const std::string& patronId,
                        const std::string& resourceId);

    void openSession(const std::string& patronId,
                     const std::string& resourceId);
    void closeSession(const std::string& patronId,
                      const std::string& resourceId);

    int borrowedCount(const std::string& patronId,
                      const std::string& resourceId) const;

    int activeSessions(const std::string& resourceId) const;

private:
    using PatronResource = std::pair<std::string, std::string>;

    Catalog& catalog_;
    std::map<PatronResource, int> borrowed_;
    std::map<std::string, std::set<std::string>> sessions_;
};

}  // namespace bookmgmt
