// Roll No: MT26159
// Name: Divyansh Patel

#include "bookmgmt/MT26159_Thesis.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

Thesis::Thesis(std::string id, std::string title,
               std::string publisher, int year, Money unitPrice,
               std::string university, std::string degree,
               std::string supervisor)
    : Resource(std::move(id), std::move(title), std::move(publisher),
               year, unitPrice),
      university_(std::move(university)),
      degree_(std::move(degree)),
      supervisor_(std::move(supervisor)) {
    if (university_.empty()) {
        throw std::invalid_argument("thesis university must not be empty");
    }
    if (degree_.empty()) {
        throw std::invalid_argument("thesis degree must not be empty");
    }
    if (supervisor_.empty()) {
        throw std::invalid_argument("thesis supervisor must not be empty");
    }
}

void Thesis::printDetails(std::ostream& os) const {
    os << "  university: " << university_ << "\n"
       << "  degree: " << degree_ << "\n"
       << "  supervisor: " << supervisor_ << "\n";
}

}  // namespace bookmgmt
