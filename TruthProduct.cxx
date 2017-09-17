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
