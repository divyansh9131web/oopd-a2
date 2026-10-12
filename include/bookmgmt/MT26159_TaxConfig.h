// Roll No: MT26159
// Name: Divyansh Patel
#include <stdexcept>
#pragma once

namespace bookmgmt {

class TaxConfig {
public:
    // Rates are percentages: 10.0 means 10% tax.
    TaxConfig(double printTaxPercent = 0.0,
              double electronicTaxPercent = 0.0)
        : printTaxPercent_(printTaxPercent),
          electronicTaxPercent_(electronicTaxPercent) {
        validate(printTaxPercent_);
        validate(electronicTaxPercent_);
    }

    double printTaxPercent() const { return printTaxPercent_; }
    double electronicTaxPercent() const { return electronicTaxPercent_; }

    void setPrintTaxPercent(double rate) {
        validate(rate);
        printTaxPercent_ = rate;
    }

    void setElectronicTaxPercent(double rate) {
        validate(rate);
        electronicTaxPercent_ = rate;
    }

private:
    static void validate(double rate) {
        if (rate < 0.0 || rate > 100.0) {
            throw std::invalid_argument(
                "tax rate must be between 0 and 100 percent"
            );
        }
    }

    double printTaxPercent_;
    double electronicTaxPercent_;
};

}  // namespace bookmgmt
