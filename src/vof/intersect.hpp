#include <array>
#include <span>
#include <tuple>

std::tuple<std::array<double, 12>, std::array<bool, 12>>
get_intersect(double volume_fraction, std::span<double, 3> normal);
std::tuple<std::array<double, 3>, std::array<double, 3>>
get_wall_sizes(double volume_fraction, std::span<double, 3> normal);

extern const std::array<std::array<double, 3>, 12> baselines;
extern const std::array<unsigned int, 12> switching_dim;
