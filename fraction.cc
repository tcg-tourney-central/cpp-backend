#include "fraction.h"

#include <cassert>

namespace tcgtc {
namespace {

// Euclidean Algorithm (Greatest Common Divisor) implementation.
uint64_t gcd(uint64_t a, uint64_t b) {
  while (b != 0) {
    uint64_t tmp = b;
    b = a % b;
    a = tmp;
  }
  return a;
}

}  // namespace 

Fraction::Fraction(uint64_t numer, uint64_t denom) {
  assert(denom != 0);
  uint64_t div = gcd(numer, denom);
  numer_ = numer / div;
  denom_ = denom / div;
}

// Used internally if we have already done the GCD computation.
Fraction::Fraction(uint64_t numer, uint64_t denom, void*) 
  : numer_(numer), denom_(denom) {
  assert(denom != 0);
}

// Assuming a,c >= 0, b,d > 0, then:
// a/b == c/d <=> (b*d)*(a/b) == (b*d)*(c/d) <=> a*d == b*c
bool Fraction::operator==(const Fraction& other) const {
  return this->numer_ * other.denom_ == this->denom_ * other.numer_;
}

// Assuming a,c >= 0, b,d > 0, then:
// a/b < c/d <=> (b*d)*(a/b) < (b*d)*(c/d) < a*d < b*c
bool Fraction::operator<(const Fraction& other) const {
  return this->numer_ * other.denom_ < this->denom_ * other.numer_;
}

// Assuming a,c >= 0, b,d > 0, then:
// a/b + c/d == (a*d)/(b*d) + (c*b)/(d*b) = (a*d + c*b)/(b*d)
Fraction Fraction::operator+(const Fraction& other) const {
  uint64_t tmpnum = this->numer_ * other.denom_ + other.numer_ * this->denom_;
  return Fraction(tmpnum, this->denom_ * other.denom_);
}

Fraction& Fraction::operator+=(const Fraction& other) {
  *this = *this + other;
  return *this;
}

// Assuming a,c >= 0, b,d > 0, then:
// a/b * c/d == a*c/b*d
Fraction Fraction::operator*(const Fraction& other) const {
  return Fraction(this->numer_ * other.numer_, 
                  this->denom_ * other.denom_);
}

// Represent this in terms of multiplication.
Fraction Fraction::operator/(const Fraction& other) const {
  Fraction inv(other.denom_, other.numer_, nullptr);
  return (*this) * inv;
}

Fraction Fraction::ApplyMtrBound() const {
  static const Fraction kLowerBound = Fraction(1, 3);
  return *this < kLowerBound ? kLowerBound : *this;
}


}  // namespace tcgtc
