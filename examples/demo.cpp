// Demo: builds a small catalog, sets a budget with per-category quotas,
// and runs a batch of purchase requests through the acquisition manager.

#include <cstdio>
#include <iostream>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;

int main() {
    Catalog catalog;

    catalog.emplace<Book>("B001", "Clean Code", std::vector<std::string>{"Robert C. Martin"},
                          "978-0132350884", "Prentice Hall", 2008, Money::of(450));
    catalog.emplace<Book>("B002", "The C++ Programming Language",
                          std::vector<std::string>{"Bjarne Stroustrup"}, "978-0321563842",
                          "Addison-Wesley", 2013, Money::of(1200), 4, Binding::Hardcover);
    catalog.emplace<ElectronicResource>("R001", "IEEE Xplore Digital Library", "IEEE", 2026,
                                        Money::of(150), "https://ieeexplore.example",
                                        LicenseModel::AnnualSubscription, Money::of(2000));
    catalog.emplace<ElectronicResource>("R002", "MATLAB Campus Licence", "MathWorks", 2026,
                                        Money::of(400), "https://licensing.example/matlab",
                                        LicenseModel::Perpetual);
    
    catalog.emplace<EBook>(
    "E001", "Digital Electronics",
    std::vector<std::string>{"A. Author", "B. Writer"},
    "978-1234567890", "Tech Press", 2026,
    Money::of(100), "https://ebooks.example",
    EBookFormat::EPUB, true,
    LicenseModel::AnnualSubscription, Money::of(50));

    catalog.emplace<Journal>("J001", "Nature Research Journal", "Springer", 2026,
                         Money::of(500), "2041-1723", 12, 2);

    catalog.emplace<AudioBook>(
    "A001", "Computer Science Audio Course", "Audio Press", 2026,
    Money::of(100), "https://audio.example",
    "Jane Narrator", 360,
    LicenseModel::AnnualSubscription, Money::of(50));

    catalog.emplace<Thesis>(
    "T001", "Chip Design Research", "IIIT Delhi", 2026,
    Money::of(0), "IIIT Delhi", "M.Tech", "Dr. Sharma");

    std::cout << "=== Catalog ===\n";
    for (const Resource* r : catalog.all()) std::cout << r->summary() << "\n";

    std::cout << "\n=== Details of R001 ===\n" << catalog.get("R001");

    Budget budget(Money::of(40000));
    budget.setQuota(ResourceCategory::Book, {10, Money::of(8000)});
    budget.setQuota(ResourceCategory::ElectronicResource, {40, Money::of(12000)});
    budget.setQuota(ResourceCategory::Journal, {10, Money::of(6000)});
    budget.setQuota(ResourceCategory::EBook, {20, Money::of(4000)});
    budget.setQuota(ResourceCategory::AudioBook, {20, Money::of(3000)});
    budget.setQuota(ResourceCategory::Thesis, {10, Money::of(1000)});



    AcquisitionManager acq(catalog, budget, TaxConfig{5.0, 10.0});
    
    
    // Q12: Register vendors for B001
    acq.addVendor("B001", "Vendor A", Money::of(500));
    acq.addVendor("B001", "Vendor B", Money::of(400));

    std::cout << "\n=== Vendor Pricing (Q12) ===\n";
    std::cout << "Cheapest vendor for B001: "
              << acq.cheapestVendor("B001") << "\n";
    std::cout << "Quote for 2 copies of B001: "
              << acq.quote("B001", 2) << "\n";


    std::cout << "\n=== Quotes ===\n";
    std::cout << "5 copies of B002  = " << acq.quote("B002", 5) << "\n";
    std::cout << "20 seats of R001  = " << acq.quote("R001", 20) << "  (incl. platform fee)\n";

    acq.processBatch({
        {"B001", 4},   // 1800  ok
        {"B002", 5},   // 6000  ok  -> book spend 7800
        {"J001", 3},  // Journal subscription purchase
        {"B001", 1},   // 450   rejected: book spend quota (200 left)
        {"R001", 20},  // 5000  ok
        {"R002", 25},  // 10000 rejected: e-resource unit quota (20 seats left)
        {"R002", 15},  // 6000  ok  -> e-resource spend 11000
        {"R002", 5},   // 2000  rejected: e-resource spend quota (1000 left)
        {"X999", 1},   // rejected: unknown id
        {"E001", 3},  // EBook seats
        {"A001", 3},  // AudioBook seats: 350 total, including platform fee
        {"T001", 2},  // Thesis copies: free
    });

    std::cout << "\n=== Acquisition report ===\n";
    acq.printReport(std::cout);

    std::cout << "\n=== Budget ===\n";
    budget.print(std::cout);

    std::cout << "\n=== Holdings ===\n";
    for (const Resource* r : catalog.all())
        std::cout << "  " << r->id() << ": " << catalog.holdings(r->id())
                  << (r->isDigital() ? " seats" : " copies") << "\n";

    // Direct purchase: errors are reported with exceptions
    std::cout << "\n=== Direct purchase that breaks a quota ===\n";
    try {
        acq.purchase("B002", 1);
    } catch (const QuotaExceededError& e) {
        std::cout << "QuotaExceededError: " << e.what() << "\n";
    }

    // Q13: Catalogue search and persistence
    std::cout << "\n=== Q13: Catalogue search and persistence ===\n";

    std::cout << "Author search (Robert C. Martin): "
              << catalog.searchAuthor("Robert C. Martin").size()
              << " result(s)\n";
    std::cout << "ISBN/ISSN search (978-0132350884): "
              << catalog.searchISBN("978-0132350884").size()
              << " result(s)\n";
    std::cout << "Resources from 2025 to 2026: "
              << catalog.searchYearRange(2025, 2026).size()
              << " result(s)\n";

    const std::string demoFile = "catalog_demo.txt";
    catalog.saveToFile(demoFile);

    Catalog restoredCatalog;
    restoredCatalog.loadFromFile(demoFile);
    std::cout << "Saved and reloaded catalogue: "
              << restoredCatalog.size() << " resource(s)\n";
    std::remove(demoFile.c_str());

    // Q14: Print borrowing and electronic-resource sessions
    std::cout << "\n=== Q14: Borrowing and digital sessions ===\n";
    LendingManager lending(catalog);

    lending.borrow("P001", "B001");
    std::cout << "P001 borrowed B001; borrowed count: "
              << lending.borrowedCount("P001", "B001") << "\n";
    lending.returnResource("P001", "B001");
    std::cout << "P001 returned B001; borrowed count: "
              << lending.borrowedCount("P001", "B001") << "\n";

    lending.openSession("P001", "R001");
    std::cout << "P001 opened R001; active sessions: "
              << lending.activeSessions("R001") << "\n";
    lending.closeSession("P001", "R001");
    std::cout << "P001 closed R001; active sessions: "
              << lending.activeSessions("R001") << "\n";

    return 0;
}
