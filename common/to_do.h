#pragma once
#include <stdexcept>
#include <string>

#define TO_DO() throw std::logic_error("TO_DO " __FILE__ " " + std::to_string(__LINE__))
