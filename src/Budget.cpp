#include "bookmgmt/Budget.h"

#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <cmath>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

namespace {
// Every category, in the order Budget::print() lists them.
const ResourceCategory kAllCategories[] = {
    ResourceCategory::Book,
    ResourceCategory::ElectronicResource,
    ResourceCategory::Journal,
    ResourceCategory::EBook,
    ResourceCategory::AudioBook,
    ResourceCategory::Thesis
};
}

Budget::Budget(Money total) : total_(total) {
    if (total_.isNegative()) throw std::invalid_argument("budget must not be negative");
}

void Budget::setQuota(ResourceCategory c, Quota q) {
    if (q.maxUnits < 0 || q.maxSpend.isNegative() ||
        q.maxDistinctTitles < 0) {
        throw std::invalid_argument("quota limits must not be negative");
    }

    quotas_[c] = q;
}

void Budget::removeQuota(ResourceCategory c) { quotas_.erase(c); }

std::optional<Quota> Budget::quotaFor(ResourceCategory c) const {
    auto it = quotas_.find(c);
    if (it == quotas_.end()) return std::nullopt;
    return it->second;
}

Usage Budget::usageFor(ResourceCategory c) const {
    auto it = usage_.find(c);
    return it == usage_.end() ? Usage{} : it->second;
}

std::optional<int> Budget::unitsRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxUnits - usageFor(c).units;
}

std::optional<Money> Budget::spendRemaining(ResourceCategory c) const {
    auto q = quotaFor(c);
    if (!q) return std::nullopt;
    return q->maxSpend - usageFor(c).spent;
}

Budget::Failure Budget::evaluate(ResourceCategory c, int units, Money cost,
                                 std::string& why) const {
    if (units <= 0) {
        why = "quantity must be positive";
        return Failure::BadInput;
    }
    if (cost.isNegative()) {
        why = "cost must not be negative";
        return Failure::BadInput;
    }
    if (auto left = unitsRemaining(c); left && units > *left) {
        why = std::string(categoryName(c)) + " unit quota exceeded: requested " +
              std::to_string(units) + ", " + std::to_string(*left) + " remaining";
        return Failure::Quota;
    }
    if (auto left = spendRemaining(c); left && cost > *left) {
        why = std::string(categoryName(c)) + " spend quota exceeded: cost " +
              cost.toString() + ", " + left->toString() + " remaining";
        return Failure::Quota;
    }
    if (cost > remaining()) {
        why = "overall budget exceeded: cost " + cost.toString() + ", " +
              remaining().toString() + " remaining";
        return Failure::Overall;
    }
    why.clear();
    return Failure::None;
}

std::string Budget::check(ResourceCategory c, int units, Money cost) const {
    std::string why;
    evaluate(c, units, cost, why);
    return why;
}

std::string Budget::check(
    ResourceCategory c, int units, Money cost,
    const std::string& resourceId) const {

    std::string why;
    const Failure failure = evaluate(c, units, cost, why);

    if (failure != Failure::None) {
        return why;
    }

    const auto q = quotaFor(c);
    if (!q || q->maxDistinctTitles == 0) {
        return {};
    }

    const auto it = purchasedTitles_.find(c);
    const bool alreadyPurchased =
        it != purchasedTitles_.end() &&
        it->second.count(resourceId) > 0;

    if (!alreadyPurchased) {
        const std::size_t currentCount =
            it == purchasedTitles_.end() ? 0 : it->second.size();

        if (currentCount >=
            static_cast<std::size_t>(q->maxDistinctTitles)) {
            return std::string(categoryName(c)) +
                   " distinct-title quota exceeded";
        }
    }

    return {};
}

void Budget::commit(ResourceCategory c, int units, Money cost) {
    std::string why;
    switch (evaluate(c, units, cost, why)) {
        case Failure::None: break;
        case Failure::BadInput: throw std::invalid_argument(why);
        case Failure::Quota: throw QuotaExceededError(why);
        case Failure::Overall: throw BudgetExceededError(why);
    }
    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    spent_ += cost;
}

void Budget::commit(
    ResourceCategory c, int units, Money cost,
    const std::string& resourceId) {

    const std::string why = check(c, units, cost, resourceId);

    if (!why.empty()) {
        if (why.find("budget exceeded") != std::string::npos) {
            throw BudgetExceededError(why);
        }
        throw QuotaExceededError(why);
    }

    Usage& u = usage_[c];
    u.units += units;
    u.spent += cost;
    spent_ += cost;

    purchasedTitles_[c].insert(resourceId);
}



Budget Budget::rollover(double carryForwardPercentage) const {
    if (!std::isfinite(carryForwardPercentage) ||
        carryForwardPercentage < 0.0 ||
        carryForwardPercentage > 100.0) {
        throw std::invalid_argument(
            "carry-forward percentage must be between 0 and 100");
    }

    const std::int64_t remainingMinor = remaining().minorUnits();

    const auto carryMinor = static_cast<std::int64_t>(
        std::llround(
            static_cast<double>(remainingMinor) *
            carryForwardPercentage / 100.0));

    return Budget(Money::fromMinor(carryMinor));
}

void Budget::print(std::ostream& os) const {
    os << "Budget: total " << total_ << ", spent " << spent_ << ", remaining "
       << remaining() << "\n";
    os << std::left << std::setw(22) << "  Category" << std::setw(18) << "Units used/max"
       << "Spend used/max\n";
    for (ResourceCategory c : kAllCategories) {
        const Usage u = usageFor(c);
        const auto q = quotaFor(c);
        const std::string units =
            std::to_string(u.units) + "/" + (q ? std::to_string(q->maxUnits) : "-");
        const std::string spend =
            u.spent.toString() + "/" + (q ? q->maxSpend.toString() : "-");
        os << "  " << std::setw(20) << categoryName(c) << std::setw(18) << units << spend
           << "\n";
    }
}

void Budget::refund(ResourceCategory c, int units, Money cost,
                    const std::string& resourceId, bool removeTitle) {
    if (units <= 0 || cost.isNegative()) {
        throw std::invalid_argument("invalid refund");
    }

    Usage& u = usage_[c];

    if (u.units < units || u.spent < cost || spent_ < cost) {
        throw std::invalid_argument("refund exceeds recorded purchases");
    }

    u.units -= units;
    u.spent -= cost;
    spent_ -= cost;

    if (removeTitle) {
        auto it = purchasedTitles_.find(c);
        if (it != purchasedTitles_.end()) {
            it->second.erase(resourceId);
        }
    }
}

}  // namespace bookmgmt
