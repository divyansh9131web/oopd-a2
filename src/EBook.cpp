
#include "bookmgmt/EBook.h"

#include <ostream>
#include <stdexcept>
#include <utility>

namespace bookmgmt {

const char* eBookFormatName(EBookFormat format) {
    switch (format) {
        case EBookFormat::PDF:  return "PDF";
        case EBookFormat::EPUB: return "EPUB";
        case EBookFormat::HTML: return "HTML";
    }
    return "Unknown";
}

EBook::EBook(std::string id, std::string title,
             std::vector<std::string> authors, std::string isbn,
             std::string publisher, int year, Money pricePerSeat,
             std::string accessUrl, EBookFormat format,
             bool drmProtected, LicenseModel license, Money platformFee)
    : ElectronicResource(std::move(id), std::move(title), std::move(publisher),
                         year, pricePerSeat, std::move(accessUrl),
                         license, platformFee),
      authors_(std::move(authors)),
      isbn_(std::move(isbn)),
      format_(format),
      drmProtected_(drmProtected) {
    if (authors_.empty()) {
        throw std::invalid_argument("EBook must have at least one author");
    }
    if (isbn_.empty()) {
        throw std::invalid_argument("EBook ISBN must not be empty");
    }
}

void EBook::printDetails(std::ostream& os) const {
    ElectronicResource::printDetails(os);

    os << "  authors: ";
    for (std::size_t i = 0; i < authors_.size(); ++i) {
        if (i > 0) {
            os << ", ";
        }
        os << authors_[i];
    }

    os << "\n"
       << "  ISBN: " << isbn_ << "\n"
       << "  file format: " << eBookFormatName(format_) << "\n"
       << "  DRM protected: " << (drmProtected_ ? "yes" : "no") << "\n";
}

}  // namespace bookmgmt
