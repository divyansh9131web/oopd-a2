#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bookmgmt {

class Money {
public:
    Money() = default;

    static Money fromMinor(std::int64_t minor,
                           const std::string& currency = "USD");

    static Money of(std::int64_t major, int minor = 0,
                    const std::string& currency = "USD");

    std::int64_t minorUnits() const { return minor_; }
    const std::string& currencyCode() const { return currency_; }

    double toDouble() const {
        return static_cast<double>(minor_) / 100.0;
    }

    std::string toString() const;

    bool isZero() const { return minor_ == 0; }
    bool isNegative() const { return minor_ < 0; }

    Money& operator+=(const Money& other);
    Money& operator-=(const Money& other);
    Money& operator*=(std::int64_t multiplier);

    friend Money operator+(Money a, const Money& b) {
        return a += b;
    }

    friend Money operator-(Money a, const Money& b) {
        return a -= b;
    }

    friend Money operator*(Money a, std::int64_t k) {
        return a *= k;
    }

    friend Money operator*(std::int64_t k, Money a) {
        return a *= k;
    }

    friend bool operator==(const Money& a, const Money& b);
    friend bool operator!=(const Money& a, const Money& b);
    friend bool operator<(const Money& a, const Money& b);
    friend bool operator<=(const Money& a, const Money& b);
    friend bool operator>(const Money& a, const Money& b);
    friend bool operator>=(const Money& a, const Money& b);

private:
    Money(std::int64_t minor, std::string currency)
        : minor_(minor), currency_(std::move(currency)) {}

    void ensureSameCurrency(const Money& other) const;

    std::int64_t minor_ = 0;
    std::string currency_ = "USD";
};

std::ostream& operator<<(std::ostream& os, const Money& m);

}  // namespace bookmgmt
