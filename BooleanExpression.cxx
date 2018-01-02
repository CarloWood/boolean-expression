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

#include "sys.h"
#include "debug.h"
#include "BooleanExpression.h"
#include "TruthProduct.h"
#include "utils/macros.h"
#include <ostream>
#include <algorithm>

namespace boolean {

std::ostream& operator<<(std::ostream& os, VariableData const& variable_data)
{
  os << '{' << variable_data.m_user_id << ", " << variable_data.m_name << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, Variable const& variable)
{
  os << Context::instance()(variable.m_id);
  return os;
}

std::string Product::to_string(bool html) const
{
  if (is_literal())
    return is_one() ? "1" : "0";

  // Set to true to use quote instead of an overline.
  bool constexpr use_quote = false;

  int const i = use_quote ? 1 : html ? 3 : 0;
  static struct { char const* pre; char const* post; } negated_str[4] = {
      { "\e[53;4m", "\e[0m" },
      { "", "'" },
      { "", "&#x305;" },
      { "<U>", "</U>" }
    };

  std::string result;
  for (Variable::id_type id = 0; id < Product::max_number_of_variables; ++id)
  {
    mask_type variable = to_mask(id);
    if (!(m_variables & variable))      // Is this variable used?
    {
      bool negated = m_negation & variable;
      for (char c : Context::instance()(id).name())
      {
        if (negated)
          result += negated_str[i].pre;
        result += c;
        if (negated)
          result += negated_str[i].post;
      }
    }
  }
  return result;
}

std::ostream& operator<<(std::ostream& os, Expression const& expression)
{
  bool first = true;
  for (auto&& product : expression.m_sum_of_products)
  {
    if (!first)
      os << " + ";
    os << product;
    first = false;
  }
  return os;
}

std::string Expression::as_html_string() const
{
  std::string result;
  bool first = true;
  for (auto&& product : m_sum_of_products)
  {
    if (!first)
      result += '+';
    result += product.to_string(true);
    first = false;
  }
  return result;
}

Variable Context::create_variable(std::string const& name, int user_id)
{
  auto res = m_variables.emplace(Variable(), VariableData(name, user_id));
  return res.first->first;
}

VariableData const& Context::operator()(Variable::id_type id) const
{
  variables_type::const_iterator res = m_variables.find(id);
  // Don't call this for Variable's that weren't created with Context::create_variable.
  ASSERT(res != m_variables.end());
  return res->second;
}

bool Expression::add(Product const& product)
{
  bool product_is_non_zero = !product.is_zero();
  if (product_is_non_zero)
  {
    sum_of_products_type::iterator insert_point =
      std::find_if(m_sum_of_products.begin(), m_sum_of_products.end(), [&product](Product const& term){ return less(term, product); });
    m_sum_of_products.insert(insert_point, product);
  }
  return product_is_non_zero;
}

Expression& Expression::operator+=(Product const& product)
{
  if (AI_UNLIKELY(is_zero()))
    *this = product;
  else if (!is_one())
  {
    if (!product.is_one())       // add may not be called with one.
    {
      if (add(product))
        simplify();
    }
    else
      *this = true;
  }
#ifdef CWDEBUG
  sanity_check();
#endif
  return *this;
}

Expression Expression::operator*(Product const& product) const
{
  if (AI_UNLIKELY(is_literal() || product.is_literal()))
  {
    if (is_zero() || product.is_zero())
      return false;
    return is_one() ? Expression(product) : copy();
  }
  Expression result;
  bool non_zero = false;
  for (auto&& term : m_sum_of_products)
    non_zero |= result.add(term * product);
  if (AI_LIKELY(non_zero))
    result.simplify();
  else
    result = false;
#ifdef CWDEBUG
  result.sanity_check();
#endif
  return result;
}

Expression Expression::times(Expression const& expression) const
{
  if (AI_UNLIKELY(is_literal() || expression.is_literal()))
  {
    if (is_zero() || expression.is_zero())
      return false;
    return is_one() ? expression.copy() : copy();
  }
  Expression result;
  bool non_zero = false;
  for (auto&& term1 : m_sum_of_products)
    for (auto&& term2 : expression.m_sum_of_products)
      non_zero |= result.add(term1 * term2);
  if (AI_LIKELY(non_zero))
    result.simplify();
  else
    result = false;
#ifdef CWDEBUG
  result.sanity_check();
#endif
  return result;
}

//static
Expression Expression::inverse(Product const& product)
{
  // The input product may not be a literal.
  ASSERT(!product.is_literal());

  Expression result;

  Product::mask_type const variables = ~product.m_variables;
  Product::mask_type variable{1};                       // The first variable (with id 0).
  Product::mask_type constexpr end = (Product::mask_type)1 << 63;    // The 64th variable (with id 63).

  do
  {
    if (AI_UNLIKELY((variables & variable) != 0))
      result.m_sum_of_products.emplace_back(~variable, ~(product.m_negation & variable));
    variable <<= 1;     // Next variable.
  }
  while (variable != end);

  return result;
}

Expression Expression::inverse() const
{
  Expression result{true};

  if (is_literal())
  {
    if (is_one())
      result = false;
  }
  else
  {
    for (auto&& term1 : m_sum_of_products)
      result = result.times(inverse(term1));
  }

  return result;
}

Expression Expression::operator()(TruthProduct const& truth_product) const
{
  if (is_literal())
    return this->copy();
  Expression result{false};
  for (Product term : m_sum_of_products)
  {
    // If any variable occurs in both but has different negation, then the term becomes 0.
    if ((~(term.m_variables | truth_product.m_variables) & (truth_product.m_negation ^ term.m_negation)))
      continue;

    // Remove all variables in truth_product from the term (they are all 1).
    term.m_variables |= ~truth_product.m_variables;
    term.m_negation |= ~truth_product.m_variables;

    // If all variables in term occur in truth_product then the term and therefore the sum becomes true.
    if (term.m_variables == Product::full_mask)
      return true;

    // Add remaining term to result;
    result += term;
  }
  return result;
}

bool zip(Expression& output, Expression const& expression0, Expression const& expression1)
{
  size_t size[2] = { expression0.m_sum_of_products.size(), expression1.m_sum_of_products.size() };

  // An empty vector means the Expression is undefined!
  ASSERT(size[0] > 0 && size[1] > 0);

  // Merge the two ordered input vectors into a new (ordered) vector output.
  output.m_sum_of_products.reserve(size[0] + size[1]);
  Expression::sum_of_products_type const* input[2] = { &expression0.m_sum_of_products, &expression1.m_sum_of_products };

  if (expression0.is_literal() || expression1.is_literal())     // Is either side a literal?
  {
    // Truth table: (E = non-literal Expression; F=is zero, T=is one)
    // --------------------------------------------------------------
    // input[0],F,T  | input[1],F,T  | output | input to be copied
    //    E     0 0  |    1     0 1  |    1   |       1
    //    E     0 0  |    0     1 0  |    E   |       0
    //    1     0 1  |    E     0 0  |    1   |       0
    //    1     0 1  |    1     0 1  |    1   |       either
    //    1     0 1  |    0     1 0  |    1   |       0
    //    0     1 0  |    E     0 0  |    E   |       1
    //    0     1 0  |    1     0 1  |    1   |       1
    //    0     1 0  |    0     1 0  |    0   |       either
    //          ^ ^             ^ ^                   ^
    //          A B             C D                   Y
    //
    // From which we can see that (see http://www.32x8.com/circuits4---A-B-C-D----m-1-8-9-----d-0-3-5-7-10-11-12-13-14-15):
    //   Y = D + A
    output.m_sum_of_products = *input[expression1.is_one() || expression0.is_zero()];
    return false;
  }

  // Zip the two vectors into eachother so the result is still ordered.
  size_t term_of_input[2] = { 0, 0 }; // The current indices into the vector m_sum_of_products of both inputs.
  int largest_input;                  // Which input has currently the "largest_input" term of m_sum_of_products.
  do
  {
    // Sort large to small (many variables to few), so that when simplify() removes variables from
    // a term we get something that might still combine with a term that it still has to process.
    largest_input = Expression::less(expression0.m_sum_of_products[term_of_input[0]], expression1.m_sum_of_products[term_of_input[1]]) ? 1 : 0;
    output.m_sum_of_products.push_back((*input[largest_input])[term_of_input[largest_input]]);
  }
  while (++term_of_input[largest_input] < size[largest_input]);
  int remaining_input = 1 - largest_input;        // Only one input left (largest_input is consumed).
  do
  {
    output.m_sum_of_products.push_back((*input[remaining_input])[term_of_input[remaining_input]]);
  }
  while (++term_of_input[remaining_input] < size[remaining_input]);
  return true;
}

Expression operator+(Expression const& expression0, Expression const& expression1)
{
  Expression output;
  if (zip(output, expression0, expression1))
    output.simplify();
  return output;
}

bool Product::is_single_negation_different_from(Product const& product)
{
  mask_type negation_difference = m_negation ^ product.m_negation;
  return m_variables == product.m_variables &&                          // The same variables are used in both products.
         negation_difference &&                                         // There is at least one negation difference.
         (((negation_difference - 1) & negation_difference) == 0);      // There is exactly one negation difference (also true when there are none).
}

bool Product::includes_all_of(Product const& product)
{
  mask_type negation_difference = m_negation ^ product.m_negation;
  return (m_variables | product.m_variables) == product.m_variables &&  // Common variables are equal to variables in product, aka all
                                                                        //   variables use in product also occur in this Product.
         !(negation_difference & ~product.m_variables);                 // None of those variables have a different negation.
}

bool Product::has_different_negation_for_single_variable(Product const& product)
{
  if (((product.m_variables + 1) | product.m_variables) == full_mask && // Is product just a single variable?
      (m_variables | product.m_variables) != full_mask)
  {
    mask_type negation_difference = m_negation ^ product.m_negation;
    return negation_difference & ~product.m_variables;
  }
  return false;
}

//static
Product Product::common_factor(Product const& product1, Product const& product2)
{
  Product result;
  mask_type negation_difference = product1.m_negation ^ product2.m_negation;
  result.m_variables = product1.m_variables | product2.m_variables | negation_difference;       // Common variables without negation difference.
  result.m_negation = product1.m_negation | result.m_variables;                                 // The negation of only those variables.
  if (result.m_variables == full_mask)  // No common factors?
    result.m_negation = empty_mask;     // Return 'one'.
  return result;
}

//static
Product Product::remove_variable(Product const& product, Product const& variable)
{
  Product result;
  result.m_variables = product.m_variables | ~variable.m_variables;
  result.m_negation = product.m_negation | ~variable.m_variables;
  ASSERT(result.is_sane());
  return result;
}

void Expression::insert_after(Product const& term, int after, int retest, int& size, int& first_removed)
{
  Dout(dc::boolean_simplify, " ... and inserting " << term);
  sum_of_products_type::iterator iter = m_sum_of_products.begin() + (after + 1);
  for (int k = after + 1; k <= size; ++k, ++iter)
  {
    if (k < size && m_sum_of_products[k].m_variables == 0)      // k exists but is removed.
      continue;
    if (k == size || less(*iter, term))
    {
      // Insert term before the first element that is less than term.
      m_sum_of_products.insert(iter, term);
      ++size;
      break;
    }
  }
#ifdef CWDEBUG
  Dout(dc::boolean_simplify|continued_cf, " ... with result ");
  int i1 = 0;
  bool first = true;
  for (auto&& term1 : m_sum_of_products)
  {
    if (m_sum_of_products[i1].m_variables != 0) // Not removed.
    {
      Dout(dc::continued, (!first ? " + " : "") << term1);
      first = false;
    }
    ++i1;
  }
  Dout(dc::finish, "");
#endif
  // Retest new term with all terms before retest (which can be considered removed at this point).
  for (int i = 0; i < retest; ++i)
  {
    if (!m_sum_of_products[i].m_variables)      // Removed?
      continue;
    Dout(dc::boolean_simplify, "Comparing " << m_sum_of_products[i] << " with " << term);
    if (m_sum_of_products[i].has_different_negation_for_single_variable(term))
    {
      Dout(dc::boolean_simplify, "Removing the first because it contains the single variable of the second but with a different negation.");
      Product shorter_term = Product::remove_variable(m_sum_of_products[i], term);
      m_sum_of_products[i].m_variables = 0;   // Remove i.
      if (i < first_removed) first_removed = i;
      insert_after(shorter_term, i, i, size, first_removed);    // Insert shorter_term after i.
      break;
    }
    if (m_sum_of_products[i].includes_all_of(term))
    {
      Dout(dc::boolean_simplify, "Removing the first because it includes all of the second.");
      m_sum_of_products[i].m_variables = 0;   // Remove i.
      if (i < first_removed) first_removed = i;
      break;
    }
  }
}

void Expression::simplify()
{
  DoutEntering(dc::boolean_simplify, "Expression::simplify() [this = " << *this << "]");
  int size = m_sum_of_products.size();
  // An empty vector means the Expression is undefined!
  ASSERT(size > 0);
  if (size == 1)
  {
    Dout(dc::boolean_simplify, "No simplification possible.");
    return;
  }

  // Comparing the logical OR (+) between a pair of boolean products can lead to the following simplifications,
  //
  // ABCD   + ABCD'    = ABC    Both terms must be removed and replaced with ABC.
  // A      + A'       = True   Special case: the whole sum becomes true.
  // ABCXYZ + ABC      = ABC    First term must be removed.
  // ABC    + ABC      = ABC    (Same as above (first term can be removed))
  //
  // Here ABC and XYZ stand for 'any boolean product', while just A and D stand for a single indeterminate boolean Variable.
  //
  // Simplification for the following still fails:
  //
  //   B  + B'A + A'  <--
  //   B' + B A + A'  <--
  //
  //   D' + DC + C'B + B'A + A' = 1
  //

  int first_removed = -1;
  for (int i = 0; i < size - 1; ++i)
  {
    if (!m_sum_of_products[i].m_variables)      // Removed?
      continue;
    for (int j = i + 1; j < size; ++j)
    {
      if (!m_sum_of_products[j].m_variables)    // Removed?
        continue;
      Dout(dc::boolean_simplify, "Comparing " << m_sum_of_products[i] << " with " << m_sum_of_products[j]);
      if (m_sum_of_products[i].is_single_negation_different_from(m_sum_of_products[j])) // Ie, i = A'BCD' and j = A'BC'D' (only negation of C is different).
      {
        Dout(dc::boolean_simplify, "Removing both because only the negation of a single variable is different.");
        // Replace both terms with one that has the common factor.
        Product common_factor = Product::common_factor(m_sum_of_products[i], m_sum_of_products[j]);
        m_sum_of_products[i].m_variables = 0;   // Remove i.
        m_sum_of_products[j].m_variables = 0;   // Remove j.
        if (first_removed < 0) first_removed = i;
        if (common_factor.is_one())
        {
          // A + A' is true;
          *this = true;
          Dout(dc::boolean_simplify, "result: " << *this);
          return;
        }
        insert_after(common_factor, j, i, size, first_removed); // Insert common_factor after j.
        break;
      }
      if (m_sum_of_products[i].has_different_negation_for_single_variable(m_sum_of_products[j])) // Ie, i = AB'C and j = B. j must be a single variable.
      {
        Dout(dc::boolean_simplify, "Removing the first because it contains the single variable of the second but with a different negation.");
        Product shorter_term = Product::remove_variable(m_sum_of_products[i], m_sum_of_products[j]);
        m_sum_of_products[i].m_variables = 0;   // Remove i.
        if (first_removed < 0) first_removed = i;
        insert_after(shorter_term, i, i, size, first_removed);  // Insert shorter_term after i.
        break;
      }
      if (m_sum_of_products[i].includes_all_of(m_sum_of_products[j]))  // Ie, i = AB'C'XY'Z and j = AB'C' (same negation!).
      {
        Dout(dc::boolean_simplify, "Removing the first because it includes all of the second.");
        // Term i is guaranteed to be the one with the most variables (i < j).
        m_sum_of_products[i].m_variables = 0;   // Remove i.
        if (first_removed < 0) first_removed = i;
        break;
      }
    }
  }
  // If any elements were removed, move rest into place.
  if (first_removed >= 0)
  {
    int sz = first_removed;
    for (int i = first_removed + 1; i < size; ++i)
      if (m_sum_of_products[i].m_variables != 0)        // Not removed.
        m_sum_of_products[sz++] = m_sum_of_products[i];
    m_sum_of_products.resize(sz);
  }
  Dout(dc::boolean_simplify, "result: " << *this);
}

#ifdef CWDEBUG
bool Product::is_sane() const
{
  if (m_variables == empty_mask && m_negation == full_mask)
  {
    ASSERT(is_literal() && is_zero());
    return true;
  }
  if (m_variables == full_mask && m_negation == empty_mask)
  {
    ASSERT(is_literal() && is_one());
    return true;
  }
  ASSERT(!is_literal());
  mask_type not_used = ~all_variables;
  // Unused variables have their bit set.
  ASSERT((m_variables & not_used) == not_used);
  // Also in m_negation.
  ASSERT((m_negation & not_used) == not_used);
  ASSERT((m_negation & m_variables) == m_variables);
  return true;
}

void Expression::sanity_check() const
{
  ASSERT(!m_sum_of_products.empty());
  ASSERT(m_sum_of_products[0].is_sane());
  ASSERT(!m_sum_of_products[0].is_literal() || m_sum_of_products.size() == 1);
  for (auto&& term : m_sum_of_products)
    ASSERT(term.is_sane());
  for (auto iter = m_sum_of_products.begin(); iter != m_sum_of_products.end(); ++iter)
  {
    ASSERT(iter == m_sum_of_products.begin() || !iter->is_literal());
    auto next = iter + 1;
    if (next == m_sum_of_products.end())
      break;
    ASSERT(*next != *iter);
    ASSERT(less(*next, *iter));
    ASSERT(!less(*iter, *next));
  }
}
#endif

// Brute force comparison of two boolean expressions.
bool Expression::equivalent(Expression const& expression) const
{
  mask_type all_variables = 0;
  for (auto&& product : m_sum_of_products)
    if (!product.is_literal())
      all_variables |= ~product.m_variables;
  for (auto&& product : expression.m_sum_of_products)
    if (!product.is_literal())
      all_variables |= ~product.m_variables;
  int number_of_variables = 0;
  int variable_id[Product::max_number_of_variables];
  for (Variable::id_type id = 0; id < Product::max_number_of_variables; ++id)
  {
    mask_type bit = Product::to_mask(id);
    if ((all_variables & bit))
      variable_id[number_of_variables++] = id;
  }
  // Now we can find back the bit(mask) of a variable with: Product::to_mask(variable_id[variable]).
  int number_of_permutations = 1 << number_of_variables;
  for (int permutation = 0; permutation < number_of_permutations; ++permutation)
  {
    mask_type set_variables = 0;
    for (int variable = 0; variable < number_of_variables; ++variable)
    {
      int permbit = 1 << variable;
      if ((permutation & permbit))
        set_variables |= Product::to_mask(variable_id[variable]);
    }
    bool result1 = is_one();
    if (!is_literal())
    {
      result1 = false;
      for (auto&& product : m_sum_of_products)
        if ((~product.m_variables & (set_variables ^ product.m_negation)) == ~product.m_variables)
        {
          result1 = true;
          break;
        }
    }
    bool result2 = expression.is_one();
    if (!expression.is_literal())
    {
      result2 = false;
      for (auto&& product : expression.m_sum_of_products)
        if ((~product.m_variables & (set_variables ^ product.m_negation)) == ~product.m_variables)
        {
          result2 = true;
          break;
        }
    }
    if (result1 != result2)
      return false;
  }
  return true;
}

//static
Variable::id_type Variable::s_next_id;

//static
Expression Expression::s_zero(false);
//static
Expression Expression::s_one(true);

namespace {

// The Context singleton.
SingletonInstance<Context> dummy __attribute__ ((__unused__));

} // namespace

} // namespace boolean

#ifdef CWDEBUG
NAMESPACE_DEBUG_CHANNELS_START
channel_ct boolean_simplify("BOOLEAN");
NAMESPACE_DEBUG_CHANNELS_END
#endif
