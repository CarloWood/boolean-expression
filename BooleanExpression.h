// boolean-expression -- Indeterminate booleans as sum of product expressions.
//
//! @file
//! @brief Definitions of Context, Variable, Product and Expression in namespace boolean.
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
// using namespace boolean;
//
// // Create variables.
//
// Context& context{Context::instance()};
// Variable A{context.create_variable("A")};
// Variable B{context.create_variable("B")};
// Variable C{context.create_variable("B")};
//
// // Construct single variable "Products" from variables.
//
// Product a(A);                // A
// Product _a(A, true);         // !A
// Product b, _b;               // Uninitialized b and _b.
// b = B;                       // B
// _b = !B;                     // !B
//
// // Multiplying Products.
//
// Product _ab{_a};             // !A
// _ab *= b;                    // !A * B
// Product ab_c;                // Uninitialized product.
// ab_c = A * b * !C;
// assert(_ab * ab_c == 1);
//
//
// // Expressions.
//
// Expression e{true};          // True
// Expression f(_ab);           // !A * B.
// Expression g(f.copy());      // Must explicitely use copy().
//
// e  = vA * vD;                // A * D.
// e += g * !vD;                // A * D + !A * B * !D.
// e += !vB * vC;               // A * D + !A * B * !D + !B * C.
// f = e.times(f);              // !A * B * !D. Must explicitely use 'times()' to multiply two Expressions.
// e = !_ab;                    // A + !B
// f = f.inverse();             // A + !B + D. Must explicitely use 'inverse()' to calculate the inverse of an Expression.

#pragma once

#include "utils/Singleton.h"
#include <iosfwd>
#include <string>
#include <map>

namespace boolean {

class Context;
class Product;
class Expression;
class TruthProduct;

// Data associated with a boolean variable.
class VariableData
{
 private:
  std::string m_name;           // Human readable (user provided) name; the name does not have to be unique.
  int m_user_id;                // A user provided id to allow the program to recognize what this variable represents.

 public:
  VariableData(std::string const& name, int user_id = 0) : m_name(name), m_user_id(user_id) { }

  std::string const& name() const { return m_name; }
  int user_id() const { return m_user_id; }

  friend std::ostream& operator<<(std::ostream& os, VariableData const& variable_data);
};

// An indeterminate boolean variable.
class Variable
{
 public:
  using id_type = unsigned int;

 private:
  friend class Product;
  id_type m_id;                 // A unique identifier for this variable.
  static id_type s_next_id;     // The id to use for the next Variable that is created (this code is not thread-safe).

 private:
  friend class Context;         // Needs access to the two constructors below.
  // Create a NEW Variable.
  Variable() : m_id(s_next_id++) { }
  // Create a Variable from id for use as key when looking up a variable in Context::m_variables.
  Variable(id_type id) : m_id(id) { }

 public:
  inline Product operator!() const;

  friend bool operator<(Variable const& variable1, Variable const& variable2) { return variable1.m_id < variable2.m_id; }
  friend std::ostream& operator<<(std::ostream& os, Variable const& variable);
};

class Context : public Singleton<Context>
{
  friend_Instance;
 public:
  using VariableKey = Variable;
  using variables_type = std::map<VariableKey, VariableData>;

 private:
  variables_type m_variables;

 private:
  Context() { }
  ~Context() { }
  Context(Context const&) = delete;

 public:
  Variable create_variable(std::string const& name, int user_id = 0);
  VariableData const& operator()(Variable::id_type id) const;
};

// A product is a catenation of logical AND-ed boolean variables, ie
//
// D AND E AND F AND G AND Z ...
//
// where each uppercase letter stands for an indeterminate boolean variable (a Variable).
//
// The Product stores these "products" as a bit mask: one bit per variable,
// in a mask with 64-bit; hence we can deal with at most 64 variables.
//
// Consider the case where we have only four variables (four bits).
// Say, A, B, C and D at respectively the least significant to most
// significant bit (DCBA).
//
// A variable is present in the product when its corresponding bit is NOT set;
// so, A AND C = 1010, and so on.
// We can think of this as: T AND C AND T AND A == A AND C, where a 'T' stands
// for True (so in a way the bitset 1010 represents "TCTA").
//
// Let X be one of A, B, C or D.
// Note that X AND F = F (where 'F' stands for False) for any variable X.
// Also note that X AND X = X. So for a single bit (variable) we have the table:
//
//     X AND X = X    and thus the bit (for X):  0 AND 0 = 0
//     X AND T = X                               0 AND 1 = 0
//     T AND X = X                               1 AND 0 = 0
//     T AND T = T                               1 AND 1 = 1
//
// So that AND-ing two indeterminate variables is implemented with a bit-wise AND.
// From which we can conclude that the boolean literal True must be represented by 1111,
// so that AND-ing something with True has no effect.
// On the other hand we want False to be represented by 0000, so that AND-ing with
// False results in False.
//
// The above means that we may not use a product that includes all variables,
// because that is reserved for True.
//
// For our purpose we talk about multiplication when AND-ing and addition
// when OR-ing. Aka, X * X = X, X * T = T, T * X = T and T * T = T.
// Therefore we also speak of One instead of True and Zero instead of False.
//
// On top of in the product the negation of single variables may be used, for example
//
//     !B AND !C AND D
//
// which is represented in the same way as B AND C AND D but with a second
// bitmask to mark the negation of B and C (a set bit meaning its negation).
//
class Product
{
 public:
  using mask_type = uint64_t;

  static constexpr mask_type empty_mask = 0;                    // A mask with all bits unset.
  static constexpr mask_type full_mask = empty_mask - 1;        // A mask with all bits set.
  static constexpr size_t mask_size = sizeof(mask_type) * 8;    // Size of mask_type in bits.
  static constexpr Variable::id_type max_number_of_variables = mask_size - 1; // Disallow products of all mask_size variables whose value is reserved for One.
  static constexpr mask_type all_variables = full_mask >> (mask_size - max_number_of_variables);
 private:
  // Encode a Variable id to a mask representing its bit.
  static mask_type to_mask(Variable::id_type id) { ASSERT(id < max_number_of_variables); return mask_type{1} << id; }

 protected:
  friend class Expression;
  mask_type m_variables;        // Set for variables that are not in use. Variables in use have their bit unset.
  mask_type m_negation;         // Set for variables that are not in use and for variables that are in use and negated.

 public:
  // Construct an uninitialized Product.
  Product() { }

  // Construct a Product that represents a literal.
  // Zero = { empty_mask, full_mask }
  // One = { full_mask, empty_mask }
  Product(bool literal) : m_variables(literal ? full_mask : empty_mask), m_negation(~m_variables) { }
  // Assign a literal to the Product.
  Product& operator=(bool literal) { m_variables = literal ? full_mask : empty_mask; m_negation = ~m_variables; return *this; }

  // Construct a Product that represents just one variable.
  Product(Variable variable, bool negated = false) : m_variables(~to_mask(variable.m_id)), m_negation(negated ? full_mask : m_variables) { }

  // Construct a Product directly from two masks (for internal use only; but public because it is called from a header of the std library).
  explicit Product(mask_type variables, mask_type negation) : m_variables(variables), m_negation(negation) { ASSERT(is_sane()); }

#ifdef CWDEBUG
  bool is_sane() const;
#endif

  Product& operator*=(Product const& product)
  {
    // 00 = A
    // 01 = !A or part of F
    // 10 = part of T
    // 11 = - (no variable A, but also not a literal).
    //
    //       this object              product                                     desired result
    // -----------------------  -----------------------                       -----------------------
    // m_variables  m_negation  m_variables  m_negation                       m_variables  m_negation
    //      0            0           0            0           A  *  A =  A         0           0
    //      0            0           0            1           A  * !A =  F         0           1
    //                                                    or  A  *  F =  F         0           1
    //      0            0           1            0           A  *  T =  A         0           0
    //      0            0           1            1           A  *  - =  A         0           0
    //      0            1           0            0          !A  *  A =  F         0           1
    //                                                    or  F  *  A =  F         0           1
    //      0            1           0            1          !A  * !A = !A         0           1
    //                                                    or !A  *  F =  F         0           1
    //                                                    or  F  * !A =  F         0           1
    //                                                    or  F  *  F =  F         0           1
    //      0            1           1            0          !A  *  T = !A         0           1
    //                                                    or  F  *  T =  F         0           1
    //      0            1           1            1          !A  *  - = !A         0           1
    //                                                    or  F  *  - =  F         0           1
    //      1            0           0            0           T  *  A =  A         0           0
    //      1            0           0            1           T  * !A = !A         0           1
    //                                                    or  T  *  F =  F         0           1
    //      1            0           1            0           T  *  T =  T         1           0
    //      1            0           1            1           T  *  - =  -         1           1
    //      1            1           0            0           -  *  A =  A         0           0
    //      1            1           0            1           -  * !A = !A         0           1
    //                                                    or  -  *  F =  F         0           1
    //      1            1           1            0           -  *  T =  -         1           1
    //      1            1           1            1           -  *  - =  -         1           1
    //
    mask_type is_false = ~m_variables & ~product.m_variables & (m_negation ^ product.m_negation) ? full_mask : empty_mask;
    m_negation = (~m_variables & m_negation) | (~product.m_variables & product.m_negation) | (product.m_variables & m_negation) | (m_variables & product.m_negation);
    m_variables &= product.m_variables;
    m_negation |= is_false;
    m_variables &= ~is_false;
    return *this;
  }

  inline Expression operator!() const;

  void negate()
  {
    mask_type is_literal = (m_variables ^ m_negation) == full_mask ? full_mask : empty_mask;
    m_negation ^= (is_literal | ~m_variables);
    m_variables ^= is_literal;
  }

  // Zero = { empty_mask, full_mask }
  // One = { full_mask, empty_mask }
  bool is_literal() const { return (m_variables ^ m_negation) == full_mask; }
  bool is_zero() const { return m_variables == empty_mask; }
  bool is_one() const { return m_variables == full_mask; }

  int number_of_variables() const
  {
    int count = 0;
    mask_type value = ~m_variables;
    while (value != 0) { ++count; value &= value - 1; }
    return count;
  }

 private:
  bool is_single_negation_different_from(Product const& product);
  bool includes_all_of(Product const& product);
  bool has_different_negation_for_single_variable(Product const& product);
  static Product common_factor(Product const& product1, Product const& product2);
  static Product remove_variable(Product const& product, Product const& variable);

 public:
  std::string to_string(bool html = false) const;

  friend std::ostream& operator<<(std::ostream& os, Product const& product) { return os << product.to_string(); }
  friend bool operator==(Product const& product1, Product const& product2)
      { return product1.m_variables == product2.m_variables && product1.m_negation == product2.m_negation; }
  friend bool operator!=(Product const& product1, Product const& product2)
      { return product1.m_variables != product2.m_variables || product1.m_negation != product2.m_negation; }
  friend Product operator*(Product lhs, Product const& rhs) { lhs *= rhs; return lhs; }
};

//inline
Product Variable::operator!() const
{
  return Product(*this, true);
}

inline Product operator*(Variable lhs, Variable rhs) { Product result(lhs); result *= rhs; return result; }

class Expression
{
 public:
  using mask_type = Product::mask_type;

 protected:
  using sum_of_products_type = std::vector<Product>;
  sum_of_products_type m_sum_of_products;       // Elements must have a unique set of variables (Product::m_variables) and be ordered.
  static Expression s_zero;
  static Expression s_one;

  // Used for ordering m_sum_of_products.
  static bool less(Product const& product1, Product const& product2)
  {
    int number_of_variables1 = product1.number_of_variables();
    int number_of_variables2 = product2.number_of_variables();
    // Terms with more variables must be on the left in the sum for simplify() to work correctly.
    return number_of_variables1 < number_of_variables2 ||
           (number_of_variables1 == number_of_variables2 &&
            // The rest is just arbitrary ordering (doesn't matter for simplify to work).
            (product1.m_variables < product2.m_variables ||
             (product1.m_variables == product2.m_variables &&
              product1.m_negation < product2.m_negation)));
  }

  // Used by simplify.
  void insert_after(Product const& term, int after, int retest, int& size, int& first_removed);

 public:
  Expression() { }
  Expression(Expression&& expression) : m_sum_of_products(std::move(expression.m_sum_of_products)) { }
  Expression& operator=(Expression&& expression) { m_sum_of_products = std::move(expression.m_sum_of_products); return *this; }
  Expression& operator=(Product const& product) { m_sum_of_products.resize(1); m_sum_of_products[0] = product; return *this; }
  Expression& operator=(bool literal) { m_sum_of_products.resize(1); m_sum_of_products[0] = Product{literal}; return *this; }
  explicit Expression(Product const& product) : m_sum_of_products(1, product) { }
  Expression(bool literal) : m_sum_of_products(1, Product(literal)) { }
  Expression copy() const { Expression result; result.m_sum_of_products = m_sum_of_products; return result; }
  Expression times(Expression const& expression) const;
  Expression inverse() const;
  Expression operator()(TruthProduct const& truth_product) const;
  static Expression const& zero() { return s_zero; }
  static Expression const& one() { return s_one; }

  // Same as operator+(Expression const& expression0, Expression const& expression1) but without call to simplify.
  friend bool zip(Expression& output, Expression const& expression0, Expression const& expression1);

  friend Expression operator+(Expression const& expression0, Expression const& expression1);
  Expression& operator+=(Expression const& expression) { *this = *this + expression; return *this; }

  // Same as operator+=(Product const& product) but without call to simplify.
  bool add(Product const& product);

  Expression& operator+=(Product const& product);

  Expression operator*(Product const& product) const;
  void simplify();
#ifdef CWDEBUG
  void sanity_check() const;
#endif

  // A literal (zero or one) can only be in a sum when it is the only term.
  bool is_literal() const { return m_sum_of_products[0].is_literal(); }
  bool is_zero() const { return m_sum_of_products[0].is_zero(); }
  bool is_one() const { return m_sum_of_products[0].is_one(); }
  bool is_product() const { return m_sum_of_products.size() == 1; }
  bool is_initialized() const { return !m_sum_of_products.empty(); }
  bool equivalent(Expression const& expression) const;
  std::string as_html_string() const;
  Product const& as_product() const { ASSERT(is_product()); return m_sum_of_products[0]; }

 //private:
  static Expression inverse(Product const& product);

  friend std::ostream& operator<<(std::ostream& os, Expression const& expression);
  friend bool operator==(Expression const& expression1, Expression const& expression2) { return expression1.m_sum_of_products == expression2.m_sum_of_products; }
  friend bool operator!=(Expression const& expression1, Expression const& expression2) { return !(expression1.m_sum_of_products == expression2.m_sum_of_products); }
};

//inline
Expression Product::operator!() const
{
  return Expression::inverse(*this);
}

} // namespace boolean

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct boolean_simplify;
NAMESPACE_DEBUG_CHANNELS_END
#endif
