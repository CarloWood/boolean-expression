#pragma once

#include "BooleanExpression.h"

namespace boolean {

class TruthProduct : public Product
{
 public:
  using Product::Product;
  TruthProduct() : Product(true) { }
  TruthProduct(int number_of_booleans) { m_variables = m_negation = full_mask << number_of_booleans; }

  TruthProduct& operator++();
};

} // namespace boolean
