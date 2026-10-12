
#pragma once

#include <string>
#include "bookmgmt/Resource.h"

namespace bookmgmt {

// A thesis is an academic work, so it inherits general resource metadata.
// Its price can be zero because theses are often supplied without charge.
class Thesis : public Resource {
public:
    Thesis(std::string id, std::string title, std::string publisher,
           int year, Money unitPrice, std::string university,
           std::string degree, std::string supervisor);

    const std::string& university() const { return university_; }
    const std::string& degree() const { return degree_; }
    const std::string& supervisor() const { return supervisor_; }

    ResourceCategory category() const override {
        return ResourceCategory::Thesis;
    }

protected:
    void printDetails(std::ostream& os) const override;

private:
    std::string university_;
    std::string degree_;
    std::string supervisor_;
};

}  // namespace bookmgmt
