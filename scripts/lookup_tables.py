from itertools import pairwise
from typing import Generic, Literal, Self, Sequence, TypeAlias, TypeVar
from dataclasses import dataclass

from rotations import (
    chain_cube,
    edge_vertex_adjacency,
    face_containing_corners,
    index_to_corner,
    Point3D,
    all_permutations,
    inverse_cube,
    permute_bitset,
)
from structures import Case, CasePtr, EdgeDef, LookupTable, Subcase, SubcasePtr

T = TypeVar('T')


@dataclass
class Triangulation(Generic[T]):
    points: Sequence[T]
    triangles: list[tuple[int, int, int]]

    def __add__(self, b: Self) -> Self:
        b_point_map = []
        joined_points = list(self.points[:])
        for v in b.points:
            if v in joined_points:
                b_point_map.append(joined_points.index(v))
            else:
                joined_points.append(v)
                b_point_map.append(len(joined_points) - 1)
        joined_triangles = self.triangles[:]
        for corners in b.triangles:
            corners = tuple(b_point_map[i] for i in corners)
            joined_triangles.append(corners)
        return Triangulation(joined_points, joined_triangles)

    @staticmethod
    def empty() -> "Triangulation[T]":
        return Triangulation([], [])

    def get_triangles(self) -> list[tuple[T, T, T]]:
        return [
            tuple(self.points[i] for i in triangle_of_indices)
            for triangle_of_indices in self.triangles
        ]

CENTER = "center"

EdgeIndex: TypeAlias = int
Midpoint: TypeAlias = tuple[int, int]|Literal["center"]

def corner(i) -> Triangulation[Midpoint]:
    corners = (
        (i, i ^ 1),
        (i, i ^ 2),
        (i, i ^ 4),
    )
    order = [(0, 1, 2)] if int.bit_count(i) % 2 == 0 else [(2, 1, 0)]
    return Triangulation(corners, order)


def cyclic_pairwise(li: list):
    all = [li[-1], *li]
    return pairwise(all)


def quadrangle(*vertices: Midpoint):
    assert len(vertices) == 4
    return Triangulation(points=vertices, triangles=[(0,1,2),(0,2,3)])


def center_surface(*args: Midpoint) -> Triangulation:
    return Triangulation(
        points=[CENTER, *args],
        triangles=[(a+1, b+1, 0) for a, b in cyclic_pairwise(list(range(len(args))))]
    )


def bitmask(*corners: int) -> int:
    return sum(1 << i for i in corners)


all_cases: list[tuple[int, Case]] = []
all_subcases: list[Subcase] = []
subcase_by_name: dict[str, Triangulation] = {}

edges_by_name: dict[Midpoint, EdgeIndex] = edge_vertex_adjacency()
edges_by_name['center'] = 12

def add_subcase(test_bits: int, triangulation: Triangulation[Midpoint], name=None) -> tuple[int, int]:
    triangles = [ tuple(edges_by_name[corner] for corner in triangle) for triangle in triangulation.get_triangles()]
    subcase = Subcase(triangles=triangles)
    if name is not None:
        subcase_by_name[name] = triangulation
    all_subcases.append(subcase)
    return test_bits, len(all_subcases) - 1

def add_case(bits: int, tests, subcases: list[tuple[int, int]]) -> None:
    all_cases.append((bits, Case(tests, subcases)))

def add_simple_case(bits: int, subcase: Triangulation[Midpoint], name=None):
    add_case(bits, tests=[], subcases=[add_subcase(0, subcase, name=name)])

CENTER_TEST = 6

# Cases, see https://cds.cern.ch/record/292771/files/cn-95-017.pdf?version=1
# Case 0
add_simple_case(0, Triangulation.empty())

# Case 1
add_simple_case(bitmask(0), corner(0))

# Case 2
add_simple_case(bitmask(0, 1),
    quadrangle((0, 2), (0, 4), (1, 5), (1, 3))
)

# Case 3
add_case(bitmask(0,5), tests=[face_containing_corners(0, 1, 4, 5)], subcases=[
    add_subcase(0, corner(0) + corner(5), '3a'),
    add_subcase(1, quadrangle((0, 2), (0, 4), (4, 5), (5, 7)) + quadrangle((5, 7), (1, 5), (0, 1), (0, 2)), '3b'),
])

# Case 4
case4a = corner(0) + corner(7)
add_case(bitmask(0,7), tests=[CENTER_TEST], subcases=[
    add_subcase(0, case4a, name='4a'),
    add_subcase(1, Triangulation(case4a.points, [(2, 0, 4), (2, 4, 3), (1, 2, 3), (1, 3, 5), (0, 1, 5), (5, 4, 0)])),
])

# Case 5
top_points = [(1, 5), (3, 7), (2, 6)]
add_simple_case(bitmask(1,2,3), Triangulation(top_points, [(0, 1, 2)]) + quadrangle((0,1), (1,5), (2,6), (0,2)), name='5')

# Case 6
midpoints = (0, 2), (0, 4), (1, 5), (1, 3), (6, 7), (6, 4), (6, 2)
case62 = Triangulation(midpoints, [(1,2,5), (2,4,5), (2,3,4), (3,6,4), (3,0,6)])
_, subcase_62 = add_subcase(None, case62)
add_case(bitmask(7,1,0), tests=[face_containing_corners(1, 3, 5, 7), CENTER_TEST], subcases=[
    add_subcase(0, Triangulation(midpoints, [(0, 1, 2), (0, 2, 3), (4, 5, 6)])),
    add_subcase(1, case62 + quadrangle(midpoints[0], midpoints[6], midpoints[5], midpoints[1])),
    (2, subcase_62), (3, subcase_62),
])

# Case 7
big_surface_edges=[(2,0), (0,4), (4,5), (5,7), (7,3), (3,2)]
big_surface = Triangulation(big_surface_edges, triangles=[(0,1,2),(2,3,4),(4,5,0),(0,2,4)])
# The corners of the three corner triangles, in a specific order.
corners = [
    ((1,0),(0,2),(0,4)),
    ((1,5),(5,4),(5,7)),
    ((1,3),(3,7),(3,2)),
]
case742 = Triangulation.empty()
for this_triangle, next_triangle in cyclic_pairwise(corners):
    case742 += Triangulation(this_triangle, [(0,1,2)])
    case742 += quadrangle(this_triangle[2], this_triangle[0], next_triangle[0], next_triangle[1])
subcases=[
    add_subcase(None, subcase_by_name['3a'] + corner(3), '7a'),
    add_subcase(None, subcase_by_name['3b'] + corner(3), '7b'),
    add_subcase(None, center_surface((0,1), (1,5), (1,3), (2,3), (3,7), (7,5), (5,4), (0,4), (0,2)), '7c'),
]
add_case(bitmask(0,3,5),
    tests=[face_containing_corners(0,1,4,5), face_containing_corners(3,1,7,5), face_containing_corners(0,1,2,3), CENTER_TEST],
    subcases=[
        (bitmask(), subcases[0][1]), (bitmask(3), subcases[0][1]),
        (bitmask(0), subcases[1][1]), (bitmask(0,3), subcases[1][1]),
        (bitmask(0,1), subcases[2][1]), (bitmask(0,1,3), subcases[2][1]),
        add_subcase(bitmask(0,1,2), big_surface + corner(1), '7d'),
        add_subcase(bitmask(0,1,2,3), case742, '7e'),
    ]
)

# Case 8
add_simple_case(bitmask(0, 1, 2, 3), quadrangle((0, 4), (1, 5), (3, 7), (2, 6)))

# Case 9
add_simple_case(bitmask(0, 1, 2, 4), quadrangle((1, 3), (2, 3), (2,6), (1,5)) + quadrangle((1,5), (4,5), (4,6), (2,6)))

# Case 10
a, b = ((0, 1), (0, 2), (4, 6), (4, 5)), ((3, 2), (3, 1), (7, 5), (7, 6))
case1011 = quadrangle(*a) + quadrangle(*b)
b_mirrored = [a[1], a[0], a[3], a[2]]
case1012 = sum((quadrangle(a, b, d, c) for (a, b), (c, d) in zip(cyclic_pairwise(b), cyclic_pairwise(b_mirrored))), start=Triangulation.empty())
points = a[1:] + a[:1] + b[1:] + b[:1]
case102 = center_surface(*points)
add_case(bitmask(0, 3, 4, 7), tests=[CENTER_TEST, face_containing_corners(0,1,2,3), face_containing_corners(4,5,6,7)], subcases=[
    add_subcase(0, case1011),
    add_subcase(1, case1012),
    add_subcase(2, case102),
])

# Case 11
points=[(0,1),(0,4),(2,6),(6,7),(7,5),(3,1)]
add_simple_case(bitmask(0,2,3,7),
    Triangulation(points, triangles=[(0,1,2),(2,3,4),(4,5,0),(0,2,4)])
)

# Case 12
points=[(0,4), (4,6), (4,5), (5,1), (3,7), (2,6), (0,2), (0,1)]
add_case(bitmask(1,2,3,4), tests=[
    face_containing_corners(1,4),
    face_containing_corners(2,4),
], subcases=[
    add_subcase(0, subcase_by_name['5'] + corner(4)),
    add_subcase(3, Triangulation(
        points=points,
        triangles=[(0,1,6),(0,6,7),(0,7,2),(2,7,3),(2,3,4),(2,4,1),(1,4,5),(1,5,6)],
    )),
    add_subcase(1, center_surface(*points)),
    add_subcase(2, center_surface((0,1), (1,5), (3,7), (2,6), (4,6), (4,5), (0,4), (0,2))),
    # 12.3 is 12.2 mirrored
])

IMPOSSIBLE = -1

# Case 13
center_independent_subcases = [
    add_subcase(bitmask(), subcase_by_name['7a'] + corner(6)),
    add_subcase(bitmask(2), subcase_by_name['7b'] + corner(6)),
    add_subcase(bitmask(2,5), subcase_by_name['7c'] + corner(6)),
    add_subcase(bitmask(1,4,3), center_surface((0,1),(0,4),(4,6),(4,5),(5,1),(5,7),(7,6),(7,3),(3,1),(3,2),(2,6), (0,2))),
    (bitmask(0,1), IMPOSSIBLE),
    # The reverse case is also necessary because there are two chiral three-face strips with regards to the 1s and 0s
    (bitmask(0,1,2), -2), (bitmask(3,4,5), -3),
]
subcases = []
for bitset, subcase in center_independent_subcases:
    subcases.append((bitset, subcase))
    subcases.append((bitset|(1 << 6), subcase))
subcases += [
    add_subcase(bitmask(0,2,5), subcase_by_name['7d'] + corner(6)),
    add_subcase(bitmask(0,2,5,6), subcase_by_name['7e'] + corner(6)),
]
add_case(bitmask(0,3,5,6), tests=[
    *range(6), # All cubes faces, in the order bottom-up-front-back-left-right
    CENTER_TEST
], subcases=subcases)

# Case 14 (mirror of 11)
points=[(0,1),(1,5),(3,7),(6,7),(6,4),(2,0)]
add_simple_case(bitmask(1,2,3,6),
    Triangulation(points, triangles=[(2,1,0),(4,3,2),(0,5,4),(4,2,0)])
)


def resolve(t: EdgeDef) -> tuple[Point3D, Point3D]:
    if t == 'center':
        return 'center'
    return tuple(index_to_corner[j] for j in t)


case_table: list[CasePtr] = [None for _ in range(256)]

for i, (bitset, case) in enumerate(all_cases):
    neutral_rotations: set[tuple[int, bool]] = {
        (0, False) # The identity rotation at index 0 is neutral.
    }

    for perm_i, perm in enumerate(all_permutations()):
        inverse_perm = inverse_cube(perm)
        inverse_perm_id = all_permutations().index(inverse_perm)
        # perm brings this bitset back to the canonical bitset
        permuted_bitset = permute_bitset(bitset, inverse_perm.vertex_permutation)
        if case_table[permuted_bitset] is not None:
            case_ptr = case_table[permuted_bitset]
            assert case_ptr._case == i
            neutral_rotation = chain_cube(
                inverse_perm,
                all_permutations()[case_ptr.permutation],
            )
            neutral_rotation_id = all_permutations().index(neutral_rotation)
            neutral_rotations.add(
                (neutral_rotation_id, case_ptr.sign_flip)
            )
        else:
            case_table[permuted_bitset] = CasePtr(i, perm_i, sign_flip=False)
            case_table[(~permuted_bitset) % 256] = CasePtr(i, perm_i, sign_flip=True)

    subcases = [None for _ in range(2**len(case.tests))]

    for subcase_test_results, subcase in case.subcases:
        for rotation_id, sign_flip in neutral_rotations:
            rotation = all_permutations()[rotation_id]
            rotated_tests = [rotation.face_permutation[i] for i in case.tests]
            test_rotation = [case.tests.index(test) for test in rotated_tests]
            rotated_test_results = permute_bitset(subcase_test_results, test_rotation)
            if sign_flip:
                rotated_test_results = (~rotated_test_results) % (1 << len(rotated_tests))
            if subcases[rotated_test_results] is None:
                assert isinstance(subcase, int)
                subcases[rotated_test_results] = SubcasePtr(subcase, rotation_id, sign_flip)
    assert all(i is not None for i in subcases)
    case.subcases = subcases

all_cases = [case for _bits, case in all_cases]

assert all(i is not None for i in case_table)

lookup_table = LookupTable(all_cases, all_subcases, case_table)
