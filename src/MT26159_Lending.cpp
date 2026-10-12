// Roll No: MT26159
// Name: Divyansh Patel
#include "bookmgmt/MT26159_Lending.h"
#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

void LendingManager::borrow(const std::string& patronId,
                            const std::string& resourceId) {
    if (patronId.empty())
        throw LibraryError("patron ID cannot be empty");

    const Resource& resource = catalog_.get(resourceId);

    if (resource.isDigital())
        throw LibraryError("electronic resources require a session, not borrowing");

    if (catalog_.holdings(resourceId) <= 0)
        throw LibraryError("no available copies: " + resourceId);

    ++borrowed_[{patronId, resourceId}];
    catalog_.addHoldings(resourceId, -1);
}

void LendingManager::returnResource(const std::string& patronId,
                                    const std::string& resourceId) {
    auto key = PatronResource{patronId, resourceId};
    auto it = borrowed_.find(key);

    if (it == borrowed_.end() || it->second <= 0)
        throw LibraryError("patron has no borrowed copy of: " + resourceId);

    catalog_.addHoldings(resourceId, 1);
    --it->second;

    if (it->second == 0)
        borrowed_.erase(it);
}

void LendingManager::openSession(const std::string& patronId,
                                 const std::string& resourceId) {
    if (patronId.empty())
        throw LibraryError("patron ID cannot be empty");

    const Resource& resource = catalog_.get(resourceId);

    if (!resource.isDigital())
        throw LibraryError("print resources cannot open electronic sessions");

    auto& active = sessions_[resourceId];

    if (active.count(patronId))
        throw LibraryError("patron already has an active session");

    if (static_cast<int>(active.size()) >= catalog_.holdings(resourceId))
        throw LibraryError("no licensed seats available: " + resourceId);

    active.insert(patronId);
}

void LendingManager::closeSession(const std::string& patronId,
                                  const std::string& resourceId) {
    auto it = sessions_.find(resourceId);

    if (it == sessions_.end() || it->second.erase(patronId) == 0)
        throw LibraryError("patron has no active session for: " + resourceId);

    if (it->second.empty())
        sessions_.erase(it);
}

int LendingManager::borrowedCount(const std::string& patronId,
                                  const std::string& resourceId) const {
    auto it = borrowed_.find({patronId, resourceId});
    return it == borrowed_.end() ? 0 : it->second;
}

int LendingManager::activeSessions(const std::string& resourceId) const {
    auto it = sessions_.find(resourceId);
    return it == sessions_.end() ? 0 : static_cast<int>(it->second.size());
}

}  // namespace bookmgmt
