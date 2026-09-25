#include "value.hpp"
#include <cstddef>

void ValueArray::write_value(Value value) { m_values.push_back(value); }
