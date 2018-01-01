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

#include "sys.h"
#include "TruthProduct.h"

namespace boolean {

TruthProduct& TruthProduct::operator++()
{
  mask_type bit{1};
  for (Variable::id_type i = 0; i < max_number_of_variables; ++i, bit <<= 1)
  {
    if (!(m_variables & bit))   // Variable in use?
    {
      if (!(m_negation & bit))  // Not yet negated?
      {
        m_negation |= bit;
        break;
      }
      m_negation &= ~bit;
    }
  }
  return *this;
}

} // namespace boolean
