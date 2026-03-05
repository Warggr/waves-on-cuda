#include "grid.hpp"
#include <gtest/gtest.h>

TEST(GridTest, BasicAssertions) {
    using dtype = int;
    Grid<dtype, 3> grid({2, 2, 2});
    const std::array<std::size_t, 3> coords = {1, 1, 1};
    EXPECT_EQ(&grid[coords[0]][coords[1]][coords[2]], &grid[coords]);
}
