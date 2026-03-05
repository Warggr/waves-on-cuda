#pragma once
#include "grid.hpp"
#include <istream>

template<typename dtype, std::size_t ndim>
Grid<dtype, ndim> load(std::istream& infile);
