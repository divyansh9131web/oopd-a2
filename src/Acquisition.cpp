
#include "bookmgmt/Acquisition.h"

#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <map>

#include "bookmgmt/Exceptions.h"

namespace bookmgmt {

AcquisitionManager::AcquisitionManager(
    Catalog& catalog, Budget& budget, TaxConfig tax)
    : catalog_(catalog), budget_(budget), tax_(tax) {}

AcquisitionManager::AcquisitionManager(
    Catalog& catalog, Budget& budget,
    DepartmentBudgets& departments, TaxConfig tax)
    : catalog_(catalog),
      budget_(budget),
      departments_(&departments),
      tax_(tax) {}

void AcquisitionManager::setTaxConfig(TaxConfig tax) {
    tax_ = tax;
}

const TaxConfig& AcquisitionManager::taxConfig() const {
    return tax_;
}

void AcquisitionManager::addVendor(
    const std::string& resourceId,
    const std::string& vendorName,
    Money price) {

    catalog_.get(resourceId);

    if (vendorName.empty()) {
        throw std::invalid_argument("vendor name cannot be empty");
    }
    if (price.isNegative()) {
        throw std::invalid_argument("vendor price cannot be negative");
    }

    auto& offers = vendors_[resourceId];

    for (auto& offer : offers) {
        if (offer.name == vendorName) {
            offer.price = price;
            return;
        }
    }

    offers.push_back({vendorName, price});
}

std::string AcquisitionManager::cheapestVendor(
    const std::string& resourceId) const {

    const auto it = vendors_.find(resourceId);
    if (it == vendors_.end() || it->second.empty()) {
        return "";
    }

    const auto cheapest = std::min_element(
        it->second.begin(), it->second.end(),
        [](const VendorOffer& a, const VendorOffer& b) {
            return a.price < b.price;
        });

    return cheapest->name;
}

Money AcquisitionManager::priceFor(
    const Resource& resource, int quantity) const {

    const auto it = vendors_.find(resource.id());

    if (it == vendors_.end() || it->second.empty()) {
        return resource.costFor(quantity);
    }

    const auto cheapest = std::min_element(
        it->second.begin(), it->second.end(),
        [](const VendorOffer& a, const VendorOffer& b) {
            return a.price < b.price;
        });

    if (quantity <= 0) {
        throw std::invalid_argument("quantity must be positive");
    }
    return cheapest->price * quantity;
}

Money AcquisitionManager::quote(
    const std::string& id, int quantity) const {
    return priceFor(catalog_.get(id), quantity);
}

Money AcquisitionManager::taxFor(
    const Resource& resource, Money preTaxCost) const {
    const double rate = resource.isDigital()
        ? tax_.electronicTaxPercent()
        : tax_.printTaxPercent();

    const auto taxMinor = static_cast<long long>(std::llround(
        static_cast<double>(preTaxCost.minorUnits()) * rate / 100.0));

    return Money::fromMinor(taxMinor);
}


bool AcquisitionManager::canPurchase(
    const std::string& id, int quantity, std::string* reason) const {
    std::string why;

    if (const Resource* r = catalog_.find(id)) {
        if (quantity <= 0) {
            why = "quantity must be positive";
        } else {
            const Money preTax = priceFor(*r, quantity);
            const Money tax = taxFor(*r, preTax);
            const Money total = preTax + tax;

            const Budget* selectedBudget = &budget_;

            if (departments_ != nullptr) {
                if (!departments_->contains(currentDepartment_)) {
                    why = "unknown department: " + currentDepartment_;
                } else {
                    selectedBudget = &departments_->get(currentDepartment_);
                }
            }

            if (why.empty()) {
                why = selectedBudget->check(
                    r->category(), quantity, total, id);
            }
        }
    } else {
        why = "resource not found: " + id;
    }

    if (reason) {
        *reason = why;
    }

    return why.empty();
}


PurchaseRecord& AcquisitionManager::record(
    const Resource* r, const std::string& id, int qty,
    Money preTaxCost, Money tax, Money cost,
    bool approved, std::string reason) {

    history_.push_back(PurchaseRecord{
        nextOrderNo_++,
        id,
        r ? r->title() : std::string("(unknown)"),
        r ? r->category() : ResourceCategory::Book,
        qty,
        preTaxCost,
        tax,
        cost,
        approved,
        std::move(reason),
        false,
        0,
        currentDepartment_
    });

    return history_.back();
}


const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id, int quantity) {
    return purchase(id, quantity, "General");
}

const PurchaseRecord& AcquisitionManager::purchase(
    const std::string& id, int quantity,
    const std::string& department) {

    const std::string previousDepartment = currentDepartment_;
    currentDepartment_ = department;

    try {
        const Resource& r = catalog_.get(id);
        const Money preTax = priceFor(r, quantity);
        currentVendor_ = cheapestVendor(id);
        const Money tax = taxFor(r, preTax);
        const Money total = preTax + tax;

        Budget* selectedBudget = &budget_;

        if (departments_ != nullptr) {
            if (!departments_->contains(department)) {
                throw std::invalid_argument(
                    "unknown department: " + department);
            }

            selectedBudget = &departments_->get(department);
        }

        selectedBudget->commit(r.category(), quantity, total, id);
        catalog_.addHoldings(id, quantity);

        const PurchaseRecord& result =
            record(&r, id, quantity, preTax, tax, total, true, {});
        
        history_.back().vendor = currentVendor_;
        currentVendor_.clear();
        
        currentDepartment_ = previousDepartment;
        return result;
    } catch (...) {
        currentDepartment_ = previousDepartment;
        throw;
    }
}



std::vector<PurchaseRecord> AcquisitionManager::processBatch(
    const std::vector<PurchaseRequest>& reqs, bool allOrNothing) {

    if (!allOrNothing) {
        std::vector<PurchaseRecord> results;
        results.reserve(reqs.size());

        for (const auto& req : reqs) {
            const std::string previousDepartment = currentDepartment_;
            currentDepartment_ = req.department;

            const Resource* r = catalog_.find(req.resourceId);
            Money preTax;
            Money tax;
            Money total;
            std::string why;

            if (departments_ != nullptr &&
                !departments_->contains(req.department)) {
                why = "unknown department: " + req.department;
            } else if (!r) {
                why = "resource not found: " + req.resourceId;
            } else if (req.quantity <= 0) {
                why = "quantity must be positive";
            } else {
                preTax = priceFor(*r, req.quantity);
                tax = taxFor(*r, preTax);
                total = preTax + tax;

                const Budget* selectedBudget = &budget_;
                if (departments_ != nullptr) {
                    selectedBudget = &departments_->get(req.department);
                }

                why = selectedBudget->check(
                    r->category(), req.quantity, total, req.resourceId);
            }

            if (why.empty()) {
                results.push_back(
                    purchase(req.resourceId, req.quantity, req.department));
            } else {
                results.push_back(record(
                    r, req.resourceId, req.quantity,
                    preTax, tax, total, false, why));
            }

            currentDepartment_ = previousDepartment;
        }
        return results; 
    }

    // Validate against temporary budget copies before committing purchases.
    Budget trialBudget = budget_;
    std::map<std::string, Budget> trialDepartments;

    if (departments_ != nullptr) {
        // Copy every known department budget.
        // DepartmentBudgets itself remains unchanged.
        // Department names are supplied by each request.
    }

    struct Pending {
        const Resource* resource;
        const PurchaseRequest* request;
        Money preTax;
        Money tax;
        Money total;
    };

    std::vector<Pending> pending;
    pending.reserve(reqs.size());
    std::string failure;

    for (const auto& req : reqs) {
        const Resource* r = catalog_.find(req.resourceId);
        if (!r) {
            failure = "resource not found: " + req.resourceId;
            break;
        }
        if (req.quantity <= 0) {
            failure = "quantity must be positive";
            break;
        }
        if (departments_ != nullptr &&
            !departments_->contains(req.department)) {
            failure = "unknown department: " + req.department;
            break;
        }

        Money preTax = r->costFor(req.quantity);
        Money tax = taxFor(*r, preTax);
        Money total = preTax + tax;

        Budget* trial = &trialBudget;
        if (departments_ != nullptr) {
            auto it = trialDepartments.find(req.department);
            if (it == trialDepartments.end()) {
                // Copy the real budget the first time this department appears.
                trialDepartments.emplace(
                    req.department, departments_->get(req.department));
                it = trialDepartments.find(req.department);
            }
            trial = &it->second;
        }

        const std::string why = trial->check(
            r->category(), req.quantity, total, req.resourceId);
        if (!why.empty()) {
            failure = why;
            break;
        }

        trial->commit(r->category(), req.quantity, total, req.resourceId);
        pending.push_back({r, &req, preTax, tax, total});
    }

    if (!failure.empty()) {
        std::vector<PurchaseRecord> rejected;
        rejected.reserve(reqs.size());

        for (const auto& req : reqs) {
            const Resource* r = catalog_.find(req.resourceId);
            PurchaseRecord result{};
            result.orderNo = 0;
            result.resourceId = req.resourceId;
            result.title = r ? r->title() : "";
            result.category = r ? r->category() : ResourceCategory::Book;
            result.quantity = req.quantity;
            result.approved = false;
            result.reason = "batch aborted: " + failure;
            result.department = req.department;
            rejected.push_back(result);
        }
        return rejected;
    }

    std::vector<PurchaseRecord> results;
    results.reserve(reqs.size());
    for (const auto& item : pending) {
        results.push_back(purchase(
            item.request->resourceId,
            item.request->quantity,
            item.request->department));
    }
    return results;
}


const PurchaseRecord& AcquisitionManager::cancelOrder(int orderNo) {
    auto it = std::find_if(
        history_.begin(), history_.end(),
        [orderNo](const PurchaseRecord& r) {
            return r.orderNo == orderNo;
        });

    if (it == history_.end()) {
        throw std::invalid_argument("order not found");
    }

    if (!it->approved || it->cancelled || it->cancelsOrderNo != 0) {
        throw std::invalid_argument("order cannot be cancelled");
    }

    const std::size_t index =
        static_cast<std::size_t>(it - history_.begin());

    const std::string id = it->resourceId;
    const ResourceCategory category = it->category;
    const int quantity = it->quantity;
    const Money cost = it->cost;
    const Money preTax = it->preTaxCost;
    const Money tax = it->tax;
    const std::string title = it->title;

    // Keep the title counted if another active purchase uses the same ID.
    bool anotherActivePurchase = false;
    for (const auto& r : history_) {
        if (r.orderNo != orderNo && r.approved && !r.cancelled &&
            r.cancelsOrderNo == 0 && r.resourceId == id) {
            anotherActivePurchase = true;
            break;
        }
    }

    // Reverse holdings and budget usage.
   // Reverse holdings and refund the department that paid.
    catalog_.addHoldings(id, -quantity);

    if (departments_ != nullptr) {
        departments_->get(history_[index].department).refund(
            category, quantity, cost, id, !anotherActivePurchase);
    } else {
        budget_.refund(
            category, quantity, cost, id, !anotherActivePurchase);
    }
    // Preserve the original order but mark it as cancelled.
    history_[index].cancelled = true;

    // Append a separate cancellation record.
    PurchaseRecord& cancellation =
        record(catalog_.find(id), id, quantity,
               Money{} - preTax, Money{} - tax, Money{} - cost,
               false, "Cancelled order #" + std::to_string(orderNo));

    cancellation.cancelsOrderNo = orderNo;
    return cancellation;
}

Money AcquisitionManager::totalSpent() const {
    Money sum;
    for (const auto& rec : history_) {
    if (rec.approved && !rec.cancelled) sum += rec.cost;
    }
    return sum;
}

void AcquisitionManager::printReport(std::ostream& os) const {
    os << "Order history (" << history_.size() << " orders)\n";

    for (const auto& rec : history_) {
        os << "  #" << std::setw(3) << std::left << rec.orderNo
           << " " << (rec.cancelled ? "CANCELLED" :
    (rec.approved ? "APPROVED" : "REJECTED"))
           << "  " << std::setw(6) << rec.resourceId
           << " x" << std::setw(3) << rec.quantity
           << "  pre-tax: " << rec.preTaxCost
           << "  tax: " << rec.tax
           << "  total: " << rec.cost
           << "  " << rec.title
           << (rec.vendor.empty() ? "" : "  Vendor: " + rec.vendor);

        if (!rec.approved) {
            os << "\n        reason: " << rec.reason;
        }
        os << "\n";
    }

    os << "Total spent (including tax): " << totalSpent() << "\n";
}

}  // namespace bookmgmt
