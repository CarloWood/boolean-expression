// boolean-expression -- Indeterminate booleans as sum of product expressions.
//
//! @file
//! @brief Definitions of TruthProduct in namespace boolean.
//
// Copyright (C) 2017 - 2018  Carlo Wood.
//
// RSA-1024 0x624ACAD5 1997-01-26                    Sign & Encrypt
// Fingerprint16 = 32 EC A7 B6 AC DB 65 A6  F6 F6 55 DD 1C DC FF 61
//
// This file is part of boolean-expression.
//
// boolean-expression is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// boolean-expression is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with boolean-expression.  If not, see <http://www.gnu.org/licenses/>.

// Usage:
//
// A number of variables can be given a value by creating a TruthProduct.
// The TruthProduct is assumed to be True.
//
// For example, if you want to investigate what a certain Expression would
// turn into when A = true, B = false and D = false then you'd create
// a TruthProduct from the Product A * !B * !D and pass that to an Expression:
//
// Expression expr = !A * B + A * !C;
// TruthProduct tp = B * !C;
// assert(expr(tp).is_one());   // Because when B is true and C is false then
//                              // !A * B + A * !C = !A + A = 1.

#pragma once

#include "BooleanExpression.h"

namespace boolean {

class TruthProduct : public Product
{
 public:
  using Product::Product;
  TruthProduct() : Product(true) { }
  TruthProduct(int number_of_booleans) { m_variables = m_negation = full_mask << number_of_booleans; }
  explicit TruthProduct(Product const& product) : Product(product) { }

  TruthProduct& operator++();
};

} // namespace boolean
