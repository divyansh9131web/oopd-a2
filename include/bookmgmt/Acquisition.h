#pragma once
// AcquisitionManager: turns purchase requests into orders, enforcing the
// Budget's quotas, updating Catalog holdings and keeping an order history.

#include <iosfwd>
#include <string>
#include <vector>

#include "bookmgmt/Budget.h"
#include "bookmgmt/Catalog.h"
#include "bookmgmt/TaxConfig.h"
#include "bookmgmt/DepartmentBudgets.h"
#include <map>

namespace bookmgmt {

struct PurchaseRequest {
    std::string resourceId;
    int quantity;
    std::string department = "General";
};

struct PurchaseRecord {
    int orderNo;
    std::string resourceId;
    std::string title;
    ResourceCategory category;
    int quantity;
    Money preTaxCost;
    Money tax;
    Money cost;  // Final cost including tax
    bool approved;
    std::string reason;
    bool cancelled = false;
    int cancelsOrderNo = 0;
    std::string department = "General";
    std::string vendor = "";
};

class AcquisitionManager {
public:
    AcquisitionManager(Catalog& catalog, Budget& budget,
                   TaxConfig tax = TaxConfig{});
    AcquisitionManager(Catalog& catalog, Budget& budget,
                   DepartmentBudgets& departments,
                   TaxConfig tax = TaxConfig{});

    void setTaxConfig(TaxConfig tax);
    const TaxConfig& taxConfig() const;
    const PurchaseRecord& cancelOrder(int orderNo);

    // Register a vendor's price for a resource.
    void addVendor(const std::string& resourceId,
                const std::string& vendorName,
                Money price);

    // Return the cheapest registered vendor's name.
    std::string cheapestVendor(const std::string& resourceId) const;

    // Price of a request without buying anything. Throws NotFoundError.
    Money quote(const std::string& id, int quantity) const;

    // True if the purchase would be approved; if not, `reason` explains why.
    bool canPurchase(const std::string& id, int quantity,
                     std::string* reason = nullptr) const;

    // Buys immediately. Throws NotFoundError, QuotaExceededError,
    // BudgetExceededError or std::invalid_argument. On success the budget
    // and holdings are updated and the record is added to history.
    const PurchaseRecord& purchase(const std::string& id, int quantity);
    const PurchaseRecord& purchase(
        const std::string& id, int quantity,
        const std::string& department);

    // Processes requests in order; each is approved or rejected on its own
    // (never throws for a rejected request). Every outcome is recorded.
    // EXTENSION POINT: priority ordering, all-or-nothing batches, ...
    std::vector<PurchaseRecord> processBatch(
        const std::vector<PurchaseRequest>& reqs,
        bool allOrNothing = false);

    const std::vector<PurchaseRecord>& history() const { return history_; }
    Money totalSpent() const;

    void printReport(std::ostream& os) const;

private:
    PurchaseRecord& record(const Resource* r, const std::string& id, int qty,
                           Money preTaxCost, Money tax, Money cost,
                           bool approved, std::string reason);

    Money taxFor(const Resource& resource, Money preTaxCost) const;

    Catalog& catalog_;
    Budget& budget_;
    DepartmentBudgets* departments_ = nullptr;
    std::string currentDepartment_ = "General";

    struct VendorOffer {
        std::string name;
        Money price;
    };

    std::map<std::string, std::vector<VendorOffer>> vendors_;
    Money priceFor(const Resource& resource, int quantity) const;
    std::string currentVendor_;

    TaxConfig tax_;
    std::vector<PurchaseRecord> history_;
    int nextOrderNo_ = 1;
};
}  // namespace bookmgmt
