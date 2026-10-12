// Roll No: MT26159
// Name: Divyansh Patel

#include "bookmgmt/MT26159_Journal.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Journal::Journal(std::string id, std::string title, std::string publisher,
                 int year, Money annualPrice, std::string issn,
                 int issuesPerYear, int subscriptionYears)
    : Resource(std::move(id), std::move(title), std::move(publisher),
               year, annualPrice),
      issn_(std::move(issn)),
      issuesPerYear_(issuesPerYear),
      subscriptionYears_(subscriptionYears) {
    if (issn_.empty()) {
        throw std::invalid_argument("Journal ISSN must not be empty");
    }

    if (issuesPerYear_ < 1) {
        throw std::invalid_argument("Issues per year must be at least 1");
    }

    if (subscriptionYears_ < 1) {
        throw std::invalid_argument("Subscription years must be at least 1");
    }
}


Money Journal::costFor(int copies) const {
    requirePositive(copies);

    Money total = unitPrice() * copies * subscriptionYears_;

    // Q5: 10% discount for 10 or more copies.
    if (copies >= 10) {
        total = Money::fromMinor(
            total.minorUnits() * 90 / 100
        );
    }

    return total;
}


void Journal::printDetails(std::ostream& os) const {
    os << "  ISSN: " << issn_ << "\n"
       << "  issues per year: " << issuesPerYear_ << "\n"
       << "  subscription years: " << subscriptionYears_ << "\n";
}

}  // namespace bookmgmt
