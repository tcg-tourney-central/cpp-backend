// This file defines fractions as in the canonical definition (via equivalence
// classes of the rational numbers). We do this so that we can avoid floating
// point comparisons for sensitive matters such as OMW, GWP, OGWP.

#ifndef _TCGTC_FRACTION_H_
#define _TCGTC_FRACTION_H_

#include <cstdint>

namespace tcgtc {

// In theory this class and its comparison functions are prone to overflow
// operations, but we have a few things mitigating:
// (1) We store the fraction in reduced form.
// (2) We deal with small numbers as inputs, generally.
class Fraction {
 public:
  Fraction(uint64_t numer, uint64_t denom);

  // For creating Fractions representing a positive integer.
  explicit Fraction(uint64_t intval) : Fraction(intval, 1, nullptr) {}
  Fraction() : Fraction(0) {}

  // Order operations.
  bool operator==(const Fraction& other) const;
  bool operator!=(const Fraction& other) const { return !(*this == other); }
  bool operator<(const Fraction& other) const;

  // Arithmetic operations.
  Fraction operator+(const Fraction& other) const;
  Fraction operator*(const Fraction& other) const;
  Fraction operator/(const Fraction& other) const;
  Fraction& operator+=(const Fraction& other);

  // Should only be used for printing and visualization.
  //
  // TODO: Consider only exposing a function that returns the string value, so
  // no one tries to use this for numeric comparison. Unfortunately std::string
  // has an operator<...
  double dbl() const { return static_cast<double>(numer_) /
                              static_cast<double>(denom_); }

  // MTR Dictates that for the purposes of OMW%, and OGW%, they are never lower
  // than 1/3.
  Fraction ApplyMtrBound() const;

 private:
  // Private constructor for not computing the GCD.
  Fraction(uint64_t numer, uint64_t denom, void* /*unused*/);

  uint64_t numer_;
  uint64_t denom_;
};

}  // namespace tcgtc

#endif // _TCGTC_FRACTION_H_
