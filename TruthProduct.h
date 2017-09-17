#pragma once

#include "BooleanExpression.h"

namespace boolean {

class TruthProduct : public Product
{
 public:
  TruthProduct() : Product(true) { }
  using Product::Product;

  TruthProduct& operator++();
};

} // namespace boolean
