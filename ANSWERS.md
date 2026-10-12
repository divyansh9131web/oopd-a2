# OOPD Assignment 2 — Answers

## Q1 — Journal

`Journal` inherits from `Resource` and stores its ISSN, issues per year, and subscription duration. The subscription duration defaults to one year, and invalid values below one are rejected.

The total acquisition cost is calculated as:

`annual price × number of copies × subscription years`

The Journal category is included in the resource category list and budget system. Tests and the demo verify its pricing, validation, acquisition, and holdings.

## Q2 — EBook

`EBook` inherits from `ElectronicResource`, reusing its access URL, licensing model, platform fee, and seat-based pricing. It adds authors, ISBN, file format (PDF, EPUB, or HTML), and DRM status.

Bibliographic information such as authors and ISBN may overlap between `Book` and `EBook`, potentially duplicating fields and validation logic. A possible improvement is a shared `BibliographicMetadata` component containing common fields such as authors and ISBN. Both classes could use this component while retaining their type-specific features.


## Q3 — AudioBook and Thesis

`AudioBook` inherits from `ElectronicResource` because it is a digital resource that uses seat-based pricing, an access URL, a licensing model, and an optional platform fee. It adds the narrator and duration in minutes. The constructor rejects an empty narrator and a non-positive duration.

`Thesis` inherits directly from `Resource` because it represents an academic work with general resource information and academic metadata. It adds the university, degree, and supervisor. Its unit price can be zero because theses are often available without charge.

Tests verify the categories, metadata, pricing, printed details, and invalid inputs for both classes. The demo also shows an AudioBook purchase and a free Thesis acquisition.

## Q4 — Hardcover Pricing

The `Book::costFor()` method calculates the total cost based on the number of copies. Paperback books use the normal unit price, while Hardcover books receive a 20% surcharge.

The calculation uses integer minor currency units through the `Money` class, avoiding floating-point arithmetic.

Tests verify that two Paperback copies priced at 100.00 each cost 200.00, while two Hardcover copies cost 240.00. All 119 checks pass.

## Q5 — Bulk Discounts and Electronic Seat Pricing

Print books and journals receive a 10% discount when purchasing 10 or more copies. The existing Hardcover surcharge is applied before the bulk discount.

For electronic resources, the first 50 seats are charged at the regular price, while each additional seat costs half the regular price. The platform fee is charged once and is not discounted.

Tests verify pricing below and at the bulk-discount threshold, Hardcover pricing with the discount, and electronic pricing at 50, 51, and 60 seats. All 146 checks pass.

## Q6 — Configurable Tax

TaxConfig allows separate tax percentages for print and electronic resources. AcquisitionManager calculates tax on the pre-tax acquisition cost and records the pre-tax amount, tax, and final amount.

Budget checks and spending totals use the post-tax amount. The purchase report displays the pre-tax cost, tax, and final cost. Tests verify tax calculations and confirm that a purchase is rejected when the final cost exceeds the available budget, even if its pre-tax cost fits.

## Q7. Maximum Distinct Titles per Category

**Implementation:**

The `Quota` structure was extended with a `maxDistinctTitles` field to limit the number of distinct resources that can be purchased in each category. A value of `0` means that no distinct-title limit is configured.

The `Budget` class maintains a set of purchased resource IDs for each category. The `AcquisitionManager` passes the resource ID to the budget checks and commits.

**Working:**

- Purchasing a resource for the first time counts as one distinct title.
- Purchasing additional copies of the same resource ID does not increase the distinct-title count.
- Purchasing a new resource ID is rejected when the category's distinct-title limit has been reached.
- Rejected purchases do not consume a title slot.
- Existing unit and spending quotas continue to apply alongside the distinct-title limit.
- The limit can be disabled by setting `maxDistinctTitles` to `0`.

**Testing:**

The `testDistinctTitleQuota()` test verifies repeated purchases of the same resource, acceptance of a second distinct title, rejection of a third distinct title at the limit, acceptance of additional copies of an existing title, and disabling the title limit.

**Result:** All 146 checks passed after implementing Q7, with compilation using `-std=c++17 -Wall -Wextra`.

## Q8. Order Cancellation and Refunds

**Implementation:** Added an order cancellation mechanism to `AcquisitionManager` using `cancelOrder(int orderNo)`.

When an approved order is cancelled:
- The full purchase cost, including tax, is refunded to the overall budget and the corresponding category budget.
- The purchased quantity is removed from the catalog holdings.
- The original order is retained in the purchase history and marked as cancelled.
- A separate cancellation record is created, referencing the original order number.
- The distinct-title quota is updated when no other active purchase exists for that resource.
- Rejected orders, nonexistent orders, and orders that have already been cancelled cannot be cancelled again.

**Testing:** Added `testOrderCancellation()` to verify refunds, holdings, budget usage, purchase history, distinct-title quota availability, and prevention of duplicate cancellation.

**Result:** All 159 checks pass.

**Q9. Department-Specific Budgets and Quotas**

**Implementation:** Added department-specific budget management using `DepartmentBudgets`. Each department maintains its own independent budget, and purchase requests include the department name. The system checks and updates the selected department's budget during purchases. When an order is cancelled, the refund is credited to the correct department. Empty or duplicate department names and requests for unknown departments are rejected. Existing single-budget functionality is preserved using `General` as the default department.

**Testing:** Added `testDepartmentBudgets()` to verify independent departmental spending, correct budget updates, cancellation refunds, unchanged general budget, and catalog holdings after cancellation.

**Result:** All 169 checks pass.

**Q10. Year-End Budget Rollover**

**Implementation:** Added `Budget::rollover(double carryForwardPercentage)` to create a new budget using a configurable percentage of the unspent amount from the current budget. The carry-forward percentage must be between 0% and 100%; invalid values are rejected. The original budget remains unchanged.

**Testing:** Added `testBudgetRollover()` to verify 0%, 50%, and 100% carry-forward percentages, rejection of invalid percentages, and preservation of the original budget.

**Result:** All 177 checks pass.

**Q11. All-or-Nothing Batch Processing**

**Implementation:** Extended `AcquisitionManager::processBatch()` with an optional `allOrNothing` parameter. The default value (`false`) preserves the existing behavior, where each request is processed independently. When enabled, the system validates the entire batch using temporary budget copies before committing purchases. If any request fails validation, the entire batch is rejected without changing the budget, catalog holdings, or order history.

**Testing:** Added `testAllOrNothingBatch()` to verify successful batch processing, rejection of the entire batch when a request fails, unchanged budget and holdings after rejection, and unchanged order history.

**Result:** All 189 checks pass.

## Q12. Vendor Management

**Implementation:**

Added vendor management to the acquisition system. Multiple vendors can offer the same resource at different prices. The system selects the vendor offering the lowest price and uses that price when calculating quotes and processing purchases. The selected vendor's name is stored in the purchase record and displayed in the acquisition report. If no vendor is registered for a resource, the system uses the resource's original pricing.

**Testing:**

Added a test named `testCheapestVendor()` to verify that:
- Multiple vendors can be registered for the same resource.
- The vendor with the lowest price is selected.
- Quotes use the cheapest vendor's price.
- The purchase record stores the selected vendor's name.
- The purchase is approved and the catalog holdings are updated correctly.

**Result:**

All 205 checks pass. The project compiles using C++17 with `-Wall -Wextra` without compiler warnings.
## Q13. Catalogue Search and Persistence

**Implementation:**

Extended `Catalog` with search functions for author, ISBN/ISSN, and publication year range. Author searches are case-insensitive. ISBN/ISSN searches support books, e-books, and journals, while year-range searches include both boundary years.

Added `saveToFile()` and `loadFromFile()` to persist the catalogue. The saved data includes resource type information and the fields needed to reconstruct the appropriate derived class. Loading validates the saved data before replacing the existing catalogue.

**Testing:**

The tests verify catalogue searches and saving and loading resources. The demo demonstrates author search, ISBN/ISSN search, year-range search, and a catalogue save/load round trip.

## Q14. Borrowing and Electronic-Resource Sessions

**Implementation:**

Added `LendingManager` to manage physical-resource borrowing and electronic-resource sessions.

- Patrons can borrow print resources only when copies are available.
- Returning a borrowed resource restores its available-copy count.
- Electronic sessions are limited by the licensed seat count.
- Patrons must close active sessions, and duplicate or invalid operations are rejected.

**Testing:**

The demo demonstrates borrowing and returning a print resource, followed by opening and closing an electronic-resource session. Tests should also cover unavailable copies, borrowing the same resource repeatedly, licensed-seat limits, and invalid returns or session closures.

## Q15. Currency-Aware Money

**Implementation:**

Extended `Money` to store a currency code alongside the amount. Arithmetic and comparison operations check that both operands use the same currency. Operations involving different currencies throw `std::invalid_argument` instead of combining or comparing incompatible amounts.

Multiplication preserves the original currency. Amounts continue to use integer minor units to avoid floating-point arithmetic errors.

**Testing:**

Tests verify normal money operations and confirm that adding, subtracting, or comparing amounts in different currencies throws the expected exception.

## Q16. Replacing Test Macros with assert

**Implementation:**

Replaced the custom `CHECK` and `CHECK_THROWS` test macros in `tests/MT26159_test_main.cpp` with standard C++ `assert` statements. Added a `throwsAs` helper to check whether an expression throws the expected exception type.

**Testing:**

Compiled the test suite with C++17 and the `-Wall -Wextra` warning flags. The test executable reports `All assertions passed.`

Note: Standard `assert` checks are disabled when `NDEBUG` is defined, so the test suite should be compiled without that definition.
