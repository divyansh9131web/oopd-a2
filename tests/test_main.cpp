// Minimal self-contained test runner (no external framework needed).

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <memory>
#include <vector>

#include "bookmgmt/bookmgmt.h"

using namespace bookmgmt;


template <typename Exception, typename Callable>
bool throwsAs(Callable&& callable) {
    try {
        callable();
    } catch (const Exception&) {
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

static void testMoney() {
    // Existing money behavior
    assert(Money::of(12, 5).toString() == "12.05");
    assert(Money::of(-3, 50).toString() == "-3.50");
    assert(Money::fromMinor(7).toString() == "0.07");
    assert(Money::of(10) + Money::of(0, 50) == Money::fromMinor(1050));
    assert(Money::of(3) * 4 == Money::of(12));
    assert(Money::of(1) < Money::of(2));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Money::of(1, 100)); }));

    // Q15: currency codes
    const Money usd100 = Money::of(100, 0, "USD");
    const Money usd50 = Money::of(50, 0, "USD");
    const Money inr100 = Money::of(100, 0, "INR");

    assert(usd100.currencyCode() == "USD");
    assert(inr100.currencyCode() == "INR");
    assert(Money::fromMinor(250, "INR").currencyCode() == "INR");

    // Operations using the same currency
    assert(usd100 + usd50 == Money::of(150, 0, "USD"));
    assert(usd100 - usd50 == Money::of(50, 0, "USD"));
    assert(usd50 * 2 == Money::of(100, 0, "USD"));
    assert(usd50 < usd100);
    assert(usd50 <= usd100);
    assert(usd100 > usd50);
    assert(usd100 >= usd50);
    assert(usd100 == Money::of(100, 0, "USD"));
    assert(usd100 != usd50);

    // Operations between different currencies must throw
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 + inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 - inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 == inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 != inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 < inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 <= inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 > inr100); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(usd100 >= inr100); }));

    // Empty currency codes are invalid
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Money::of(10, 0, "")); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Money::fromMinor(100, "")); }));
}

static void testResourcesAndCost() {
    Book b("B1", "T", {"A", "B", "C"}, "isbn", "P", 2020, Money::of(100));
    assert(b.category() == ResourceCategory::Book);
    assert(!b.isDigital());
    assert(b.costFor(3) == Money::of(300));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(b.costFor(0)); }));
    assert(joinAuthors(b.authors()) == "A, B and C");

    ElectronicResource e("R1", "DB", "P", 2026, Money::of(10), "url",
                         LicenseModel::AnnualSubscription, Money::of(100));
    assert(e.isDigital());
    assert(e.costFor(5) == Money::of(150));

    assert(e.category() == ResourceCategory::ElectronicResource);

    // Polymorphism through a base-class reference
    const Resource& r = e;
    assert(r.costFor(1) == Money::of(110));
    std::ostringstream os;
    os << r;
    assert(os.str().find("platform fee: 100.00") != std::string::npos);

    assert(throwsAs<std::invalid_argument>([&]() { (void)(Book("", "T", {}, "", "", 2000, Money::of(1))); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Book("B", "T", {}, "", "", 2000, Money::fromMinor(-1))); }));
}

static void testJournal() {
    Journal j("J1", "Science Journal", "Science Press", 2026,
              Money::of(100), "1234-5678", 12);

    assert(j.category() == ResourceCategory::Journal);
    assert(!j.isDigital());
    assert(j.issn() == "1234-5678");
    assert(j.issuesPerYear() == 12);
    assert(j.subscriptionYears() == 1);
    assert(j.costFor(3) == Money::of(300));

    Journal twoYears("J2", "Research Journal", "Research Press", 2026,
                     Money::of(100), "8765-4321", 6, 2);
    assert(twoYears.costFor(3) == Money::of(600));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(j.costFor(0)); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Journal("J3", "Bad Journal", "Press", 2026, Money::of(10),
                "1111-2222", 0)); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(Journal("J4", "Bad Journal", "Press", 2026, Money::of(10),
                "1111-2222", 12, 0)); }));

    std::ostringstream os;
    os << j;
    assert(os.str().find("1234-5678") != std::string::npos);
    assert(os.str().find("issues per year: 12") != std::string::npos);
}


static void testEBook() {
    EBook ebook(
        "E001", "Digital Electronics",
        std::vector<std::string>{"A. Author", "B. Writer"},
        "978-1234567890", "Tech Press", 2026,
        Money::of(100), "https://ebooks.example",
        EBookFormat::EPUB, true,
        LicenseModel::AnnualSubscription, Money::of(50));

    assert(ebook.category() == ResourceCategory::EBook);
    assert(ebook.isDigital());
    assert(ebook.isbn() == "978-1234567890");
    assert(ebook.authors().size() == 2);
    assert(ebook.format() == EBookFormat::EPUB);
    assert(ebook.drmProtected());
    assert(ebook.costFor(3) == Money::of(350));

    assert(std::string(eBookFormatName(EBookFormat::PDF)) == "PDF");
    assert(std::string(eBookFormatName(EBookFormat::EPUB)) == "EPUB");
    assert(std::string(eBookFormatName(EBookFormat::HTML)) == "HTML");

    assert(throwsAs<std::invalid_argument>([&]() { (void)(ebook.costFor(0)); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(EBook("E002", "No Author", {}, "978-000", "Press", 2026,
              Money::of(100), "https://ebooks.example")); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(EBook("E003", "No ISBN", std::vector<std::string>{"Author"},
              "", "Press", 2026, Money::of(100),
              "https://ebooks.example")); }));

    std::ostringstream os;
    os << ebook;
    assert(os.str().find("Digital Electronics") != std::string::npos);
    assert(os.str().find("978-1234567890") != std::string::npos);
    assert(os.str().find("EPUB") != std::string::npos);
    assert(os.str().find("DRM protected: yes") != std::string::npos);
    assert(os.str().find("platform fee: 50.00") != std::string::npos);
}


static void testAudioBook() {
    AudioBook a("A1", "Learning C++ Audio", "Tech Press", 2026,
                Money::of(100), "https://audio.example",
                "Jane Narrator", 360,
                LicenseModel::AnnualSubscription, Money::of(50));

    assert(a.category() == ResourceCategory::AudioBook);
    assert(a.isDigital());
    assert(a.narrator() == "Jane Narrator");
    assert(a.durationMinutes() == 360);
    assert(a.costFor(3) == Money::of(350));

    std::ostringstream os;
    a.print(os);
    assert(os.str().find("narrator: Jane Narrator") != std::string::npos);
    assert(os.str().find("duration minutes: 360") != std::string::npos);
    assert(os.str().find("platform fee: 50.00") != std::string::npos);

    assert(throwsAs<std::invalid_argument>([&]() { (void)(AudioBook("A2", "Audio", "P", 2026, Money::of(10), "url",
                  "", 60)); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(AudioBook("A3", "Audio", "P", 2026, Money::of(10), "url",
                  "Narrator", 0)); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(a.costFor(0)); }));
}

static void testThesis() {
    Thesis t("T1", "Computer Architecture Research", "IIIT Delhi", 2026,
             Money::of(0), "IIIT Delhi", "M.Tech", "Dr. Sharma");

    assert(t.category() == ResourceCategory::Thesis);
    assert(!t.isDigital());
    assert(t.university() == "IIIT Delhi");
    assert(t.degree() == "M.Tech");
    assert(t.supervisor() == "Dr. Sharma");
    assert(t.costFor(3) == Money::of(0));

    std::ostringstream os;
    t.print(os);
    assert(os.str().find("university: IIIT Delhi") != std::string::npos);
    assert(os.str().find("degree: M.Tech") != std::string::npos);
    assert(os.str().find("supervisor: Dr. Sharma") != std::string::npos);

    assert(throwsAs<std::invalid_argument>([&]() { (void)(Thesis("T2", "Thesis", "P", 2026, Money::of(0),
               "", "M.Tech", "Dr. Sharma")); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(Thesis("T3", "Thesis", "P", 2026, Money::of(0),
               "IIIT Delhi", "", "Dr. Sharma")); }));

    assert(throwsAs<std::invalid_argument>([&]() { (void)(Thesis("T4", "Thesis", "P", 2026, Money::of(0),
               "IIIT Delhi", "M.Tech", "")); }));
}


static void testCatalog() {
    Catalog c;
    c.emplace<Book>("B1", "Clean Code", std::vector<std::string>{"M"}, "i", "P", 2008,
                    Money::of(1));
    c.emplace<Book>("B2", "Clean Architecture", std::vector<std::string>{"M"}, "i", "P",
                    2017, Money::of(1));
    c.emplace<ElectronicResource>("R1", "ACM Digital Library", "ACM", 2026, Money::of(1),
                                  "url");

    assert(c.size() == 3);
    assert(c.contains("B1"));
    assert(c.find("nope") == nullptr);
    assert(throwsAs<NotFoundError>([&]() { (void)(c.get("nope")); }));
    assert(throwsAs<DuplicateIdError>([&]() { (void)(c.emplace<Book>("B1", "dup", std::vector<std::string>{}, "", "", 1,
                                 Money::of(1))); }));

    assert(c.searchTitle("clean").size() == 2);
    assert(c.byCategory(ResourceCategory::ElectronicResource).size() == 1);
    assert(c.where([](const Resource& r) { return r.isDigital(); }).size() == 1);

    assert(c.holdings("B1") == 0);
    c.addHoldings("B1", 3);
    assert(c.holdings("B1") == 3);
    assert(throwsAs<std::invalid_argument>([&]() { (void)(c.addHoldings("B1", -5)); }));

    c.remove("R1");
    assert(c.size() == 2);
    assert(throwsAs<NotFoundError>([&]() { (void)(c.remove("R1")); }));
}

static void testBudget() {
    Budget b(Money::of(1000));
    b.setQuota(ResourceCategory::Book, {5, Money::of(400)});

    assert(b.check(ResourceCategory::Book, 2, Money::of(200)).empty());
    assert(!b.check(ResourceCategory::Book, 6, Money::of(10)).empty());   // units
    assert(!b.check(ResourceCategory::Book, 1, Money::of(401)).empty());  // spend
    assert(!b.check(ResourceCategory::ElectronicResource, 1, Money::of(1001)).empty());  // overall
    assert(b.check(ResourceCategory::ElectronicResource, 1, Money::of(900)).empty());    // no quota

    b.commit(ResourceCategory::Book, 4, Money::of(300));
    assert(b.spent() == Money::of(300));
    assert(*b.unitsRemaining(ResourceCategory::Book) == 1);
    assert(*b.spendRemaining(ResourceCategory::Book) == Money::of(100));
    assert(!b.unitsRemaining(ResourceCategory::ElectronicResource).has_value());

    assert(throwsAs<QuotaExceededError>([&]() { (void)(b.commit(ResourceCategory::Book, 2, Money::of(10))); }));
    assert(throwsAs<BudgetExceededError>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 1, Money::of(800))); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(b.commit(ResourceCategory::ElectronicResource, 0, Money::of(1))); }));
    assert(b.spent() == Money::of(300));  // failed commits changed nothing
}


void testHardcoverPricing() {
    Book paperback(
        "PB01", "Paperback Test", {"Author"},
        "ISBN-PB01", "Publisher", 2026, Money::of(100)
    );

    Book hardcover(
        "HC01", "Hardcover Test", {"Author"},
        "ISBN-HC01", "Publisher", 2026, Money::of(100),
        1, Binding::Hardcover
    );

    assert(paperback.costFor(2) == Money::of(200));
    assert(hardcover.costFor(2) == Money::of(240));
}


static void testBulkDiscounts() {
    Book paperback(
        "PBQ5", "Bulk Paperback", {"Author"},
        "ISBN-PBQ5", "Publisher", 2026, Money::of(100)
    );

    Book hardcover(
        "HCQ5", "Bulk Hardcover", {"Author"},
        "ISBN-HCQ5", "Publisher", 2026, Money::of(100),
        1, Binding::Hardcover
    );

    // Fewer than 10 copies: no bulk discount.
    assert(paperback.costFor(9) == Money::of(900));

    // 10 copies: 10% discount.
    assert(paperback.costFor(10) == Money::of(900));

    // Hardcover surcharge first, then bulk discount.
    assert(hardcover.costFor(10) == Money::of(1080));

    ElectronicResource electronic(
        "EQ5", "Electronic Test", "Publisher", 2026,
        Money::of(100), "https://example.com",
        LicenseModel::Perpetual, Money::of(50)
    );

    // 50 seats at full price, plus platform fee.
    assert(electronic.costFor(50) == Money::of(5050));

    // 51st seat costs half price.
    assert(electronic.costFor(51) == Money::of(5100));

    // 60 seats: 50 full-price seats and 10 half-price seats.
    assert(electronic.costFor(60) == Money::of(5550));
}



static void testTaxConfiguration() {
    Catalog catalog;
    Budget budget(Money::of(10000));

    catalog.add(std::make_unique<Book>(
        "TAX01", "Tax Test Book", std::vector<std::string>{"Author"},
        "ISBN-TAX01", "Publisher", 2026, Money::of(100)));

    catalog.add(std::make_unique<ElectronicResource>(
        "TAX02", "Tax Test E-Resource", "Publisher", 2026,
        Money::of(100), "https://example.com",
        LicenseModel::Perpetual, Money::of(0)));

    budget.setQuota(ResourceCategory::Book, {20, Money::of(5000)});
    budget.setQuota(ResourceCategory::ElectronicResource,
                    {100, Money::of(10000)});

    AcquisitionManager acq(catalog, budget, TaxConfig{10.0, 5.0});

    // Book: 2 copies at 100 each, plus 10% tax.
    const auto& bookOrder = acq.purchase("TAX01", 2);
    assert(bookOrder.preTaxCost == Money::of(200));
    assert(bookOrder.tax == Money::of(20));
    assert(bookOrder.cost == Money::of(220));

    // Electronic resource: 10 seats at 100 each, plus 5% tax.
    const auto& ebookOrder = acq.purchase("TAX02", 10);
    assert(ebookOrder.preTaxCost == Money::of(1000));
    assert(ebookOrder.tax == Money::of(50));
    assert(ebookOrder.cost == Money::of(1050));

    assert(acq.totalSpent() == Money::of(1270));
}


static void testPostTaxBudgetRejection() {
    Catalog catalog;
    Budget budget(Money::of(105));

    catalog.add(std::make_unique<Book>(
        "TAX03", "Budget Tax Test", std::vector<std::string>{"Author"},
        "ISBN-TAX03", "Publisher", 2026, Money::of(100)));

    budget.setQuota(ResourceCategory::Book, {10, Money::of(1000)});

    // A 10% tax makes the final cost 110, exceeding the 105 budget.
    AcquisitionManager acq(catalog, budget, TaxConfig{10.0, 0.0});

    std::string reason;
    assert(!acq.canPurchase("TAX03", 1, &reason));
    assert(!reason.empty());

    const auto results = acq.processBatch({{"TAX03", 1}});
    assert(results.size() == 1);
    assert(!results[0].approved);
    assert(results[0].preTaxCost == Money::of(100));
    assert(results[0].tax == Money::of(10));
    assert(results[0].cost == Money::of(110));
    
}


static void testAcquisition() {
    Catalog c;
    c.emplace<Book>("B1", "Book", std::vector<std::string>{"A"}, "i", "P", 2020,
                    Money::of(100));
    c.emplace<ElectronicResource>("R1", "DB", "P", 2026, Money::of(10), "url",
                                  LicenseModel::AnnualSubscription, Money::of(50));
    Budget b(Money::of(500));
    b.setQuota(ResourceCategory::Book, {3, Money::of(1000)});
    AcquisitionManager acq(c, b);

    assert(acq.quote("R1", 5) == Money::of(100));
    std::string why;
    assert(acq.canPurchase("B1", 3, &why) && why.empty());
    assert(!acq.canPurchase("B1", 4, &why) && !why.empty());
    assert(!acq.canPurchase("nope", 1, &why));

    const auto& rec = acq.purchase("B1", 2);
    assert(rec.approved && rec.cost == Money::of(200) && rec.orderNo == 1);
    assert(c.holdings("B1") == 2);

    assert(throwsAs<QuotaExceededError>([&]() { (void)(acq.purchase("B1", 2)); }));
    assert(throwsAs<NotFoundError>([&]() { (void)(acq.purchase("nope", 1)); }));
    assert(acq.history().size() == 1);  // exceptions don't record

    auto res = acq.processBatch({{"R1", 10}, {"R1", 100}, {"B1", 1}, {"zzz", 1}, {"B1", 0}});
    assert(res.size() == 5);
    assert(res[0].approved && res[0].cost == Money::of(150));
    assert(!res[1].approved);  // 1050 > remaining 150
    assert(res[2].approved);
    assert(!res[3].approved && res[3].reason.find("not found") != std::string::npos);
    assert(!res[4].approved);
    assert(acq.totalSpent() == Money::of(450));
    assert(b.spent() == acq.totalSpent());
    assert(c.holdings("R1") == 10 && c.holdings("B1") == 3);
    assert(acq.history().size() == 6);
}


static void testAllOrNothingBatch() {
    Catalog catalog;

    catalog.emplace<Book>(
        "A11", "Book One", std::vector<std::string>{"Author"},
        "ISBN-A11", "Publisher", 2026, Money::of(100));

    catalog.emplace<Book>(
        "A12", "Book Two", std::vector<std::string>{"Author"},
        "ISBN-A12", "Publisher", 2026, Money::of(100));

    Budget budget(Money::of(250));
    AcquisitionManager acq(catalog, budget);

    // A valid batch should approve both requests.
    auto success = acq.processBatch(
        {{"A11", 1}, {"A12", 1}}, true);

    assert(success.size() == 2);
    assert(success[0].approved);
    assert(success[1].approved);
    assert(catalog.holdings("A11") == 1);
    assert(catalog.holdings("A12") == 1);
    assert(budget.spent() == Money::of(200));

    // The next batch contains one valid request and one missing resource.
    // Nothing from this batch should be purchased.
    const auto historyBefore = acq.history().size();
    const auto spentBefore = budget.spent();

    auto failure = acq.processBatch(
        {{"A11", 1}, {"MISSING", 1}}, true);

    assert(failure.size() == 2);
    assert(!failure[0].approved);
    assert(!failure[1].approved);
    assert(catalog.holdings("A11") == 1);
    assert(budget.spent() == spentBefore);
    assert(acq.history().size() == historyBefore);
}


static void testDistinctTitleQuota() {
    Catalog catalog;
    Budget budget(Money::of(10000));

    catalog.add(std::make_unique<Book>(
        "Q701", "Book One", std::vector<std::string>{"Author"},
        "ISBN-Q701", "Publisher", 2026, Money::of(100)));

    catalog.add(std::make_unique<Book>(
        "Q702", "Book Two", std::vector<std::string>{"Author"},
        "ISBN-Q702", "Publisher", 2026, Money::of(100)));

    catalog.add(std::make_unique<Book>(
        "Q703", "Book Three", std::vector<std::string>{"Author"},
        "ISBN-Q703", "Publisher", 2026, Money::of(100)));

    // Allow 10 total copies, but only 2 distinct resource IDs.
    budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 2});

    AcquisitionManager acq(catalog, budget);

    // First title counts as one distinct title.
    assert(acq.purchase("Q701", 1).approved);

    // Rebuying the same resource does not use another title slot.
    assert(acq.purchase("Q701", 2).approved);

    // Second distinct title fills the title quota.
    assert(acq.purchase("Q702", 1).approved);

    // A third distinct title must be rejected.
    std::string reason;
    assert(!acq.canPurchase("Q703", 1, &reason));
    assert(reason.find("distinct-title quota exceeded") !=
          std::string::npos);

    // Rejection must not prevent additional copies of an existing title.
    assert(acq.purchase("Q701", 1).approved);

    // The rejected title did not consume a slot; after removing the limit,
    // it can be purchased.
    budget.setQuota(
        ResourceCategory::Book,
        {10, Money::of(1000), 0});
    assert(acq.purchase("Q703", 1).approved);
}


static void testOrderCancellation() {
    Catalog catalog;
    Budget budget(Money::of(10000));

    catalog.add(std::make_unique<Book>(
        "C801", "Cancel Test Book",
        std::vector<std::string>{"Author"},
        "ISBN-C801", "Publisher", 2026, Money::of(100)));

    budget.setQuota(ResourceCategory::Book,
                   {10, Money::of(5000), 2});

    AcquisitionManager acq(catalog, budget);

    // Buy two copies.
    const int originalOrder = acq.purchase("C801", 2).orderNo;

    assert(catalog.holdings("C801") == 2);
    assert(budget.spent() == Money::of(200));
    assert(acq.totalSpent() == Money::of(200));

    // Cancel the order.
    const auto& cancellation = acq.cancelOrder(originalOrder);

    assert(cancellation.cancelsOrderNo == originalOrder);
    assert(cancellation.reason.find("Cancelled") != std::string::npos);
    assert(catalog.holdings("C801") == 0);
    assert(budget.spent() == Money{});
    assert(budget.usageFor(ResourceCategory::Book).units == 0);
    assert(acq.totalSpent() == Money{});

    // Original order remains in history and is marked cancelled.
    assert(acq.history().size() == 2);
    assert(acq.history()[0].cancelled);

    // A second cancellation must fail.
    assert(throwsAs<std::invalid_argument>([&]() { (void)(acq.cancelOrder(originalOrder)); }));

    // The title slot should be available again.
    assert(acq.canPurchase("C801", 1));
}


static void testBudgetRollover() {
    Budget budget(Money::of(1000));
    budget.commit(ResourceCategory::Book, 2, Money::of(400));

    // 600 remains; 0% carry-forward gives a zero budget.
    Budget zero = budget.rollover(0.0);
    assert(zero.total() == Money{});

    // 50% of 600 is 300.
    Budget half = budget.rollover(50.0);
    assert(half.total() == Money::of(300));

    // 100% of 600 is 600.
    Budget full = budget.rollover(100.0);
    assert(full.total() == Money::of(600));

    // The original budget must remain unchanged.
    assert(budget.total() == Money::of(1000));
    assert(budget.spent() == Money::of(400));
    assert(budget.remaining() == Money::of(600));

    // Invalid percentages must be rejected.
    assert(throwsAs<std::invalid_argument>([&]() { (void)(budget.rollover(-1.0)); }));
    assert(throwsAs<std::invalid_argument>([&]() { (void)(budget.rollover(101.0)); }));
} 


static void testDepartmentBudgets() {
    Catalog catalog;
    catalog.emplace<Book>(
        "D901", "Department Budget Test",
        std::vector<std::string>{"Author"},
        "Publisher", "ISBN-D901", 2026, Money::of(100));

    Budget generalBudget(Money::of(10000));
    DepartmentBudgets departments;
    departments.addDepartment("CSE", Money::of(300));
    departments.addDepartment("ECE", Money::of(500));

    AcquisitionManager acq(
        catalog, generalBudget, departments, TaxConfig{});

    auto cseOrder = acq.purchase("D901", 2, "CSE");
    assert(cseOrder.approved);
    assert(cseOrder.department == "CSE");
    assert(departments.get("CSE").spent() == Money::of(200));
    assert(departments.get("ECE").spent() == Money{});
    assert(generalBudget.spent() == Money{});

    auto eceOrder = acq.purchase("D901", 3, "ECE");
    assert(eceOrder.approved);
    assert(departments.get("ECE").spent() == Money::of(300));

   

    const int cseOrderNo = cseOrder.orderNo;
    acq.cancelOrder(cseOrderNo);

    assert(departments.get("CSE").spent() == Money{});
    assert(departments.get("ECE").spent() == Money::of(300));
    assert(catalog.holdings("D901") == 3);
}

static void testCheapestVendor() {
    Catalog catalog;
    catalog.emplace<Book>(
        "V001", "Vendor Test Book",
        std::vector<std::string>{"Author"},
        "Publisher", "ISBN-V001", 2026, Money::of(500));

    Budget budget(Money::of(10000));
    AcquisitionManager acq(catalog, budget);

    acq.addVendor("V001", "Expensive Books", Money::of(450));
    acq.addVendor("V001", "Cheap Books", Money::of(300));

    assert(acq.cheapestVendor("V001") == "Cheap Books");
    assert(acq.quote("V001", 2) == Money::of(600));

    const auto& order = acq.purchase("V001", 2);
    assert(order.approved);
    assert(order.preTaxCost == Money::of(600));
    assert(order.vendor == "Cheap Books");
    assert(catalog.holdings("V001") == 2);
}

static void testCatalogSaveLoad() {
    const std::string filename = "/tmp/bookmgmt_catalog_test.txt";

    Catalog original;

    original.emplace<Book>(
        "Q13-B1", "Currency Test Book",
        std::vector<std::string>{"Test Author"},
        "ISBN-Q13", "Test Publisher", 2025,
        Money::of(250, 50, "INR"));

    original.addHoldings("Q13-B1", 4);

    original.emplace<ElectronicResource>(
        "Q13-R1", "Currency Test Database", "Test Publisher",
        2026, Money::of(100, 0, "EUR"), "https://example.com",
        LicenseModel::AnnualSubscription, Money::of(5, 0, "EUR"));

    original.addHoldings("Q13-R1", 3);

    original.saveToFile(filename);

    Catalog loaded;
    loaded.loadFromFile(filename);

    assert(loaded.size() == 2);
    assert(loaded.holdings("Q13-B1") == 4);
    assert(loaded.holdings("Q13-R1") == 3);

    const Resource* book = loaded.find("Q13-B1");
    assert(book != nullptr);
    if (book != nullptr) {
        assert(book->category() == ResourceCategory::Book);
        assert(book->unitPrice() == Money::of(250, 50, "INR"));
        assert(book->unitPrice().currencyCode() == "INR");
    }

    const Resource* resource = loaded.find("Q13-R1");
    assert(resource != nullptr);
    if (resource != nullptr) {
        assert(resource->category() == ResourceCategory::ElectronicResource);
        assert(resource->unitPrice() == Money::of(100, 0, "EUR"));
        assert(resource->unitPrice().currencyCode() == "EUR");

        const auto* digital =
            dynamic_cast<const ElectronicResource*>(resource);
        assert(digital != nullptr);
        if (digital != nullptr) {
            assert(digital->platformFee() == Money::of(5, 0, "EUR"));
            assert(digital->platformFee().currencyCode() == "EUR");
        }
    }

    std::remove(filename.c_str());
}

int main() {
    testMoney();
    testResourcesAndCost();
    testCatalog();
    testBudget();
    testAcquisition();
    testJournal();
    testEBook();
    testAudioBook();
    testThesis();
    testHardcoverPricing();
    testBulkDiscounts();
    testTaxConfiguration();
    testPostTaxBudgetRejection();
    testDistinctTitleQuota();
    testOrderCancellation();
    testDepartmentBudgets();
    testBudgetRollover();
    testAllOrNothingBatch();
    testDepartmentBudgets();
    testCheapestVendor();
    testCatalogSaveLoad();
    std::cout << "All assertions passed.\n";
    return 0;
}
