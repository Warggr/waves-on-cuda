# coding: utf-8
import numpy as np
from scipy.spatial.transform import Rotation
from structures import NB_EDGES, NB_FACES, NB_VERTICES, CubeGeometry, CubeRotation, EdgeDef
from typing import Generator, Sequence, TypeAlias
from itertools import product
from functools import cache

Permutation = list[int]

Point3D: TypeAlias = tuple[float, float, float]
Vector3D: TypeAlias = tuple[float, float, float]
Segment: TypeAlias = tuple[Point3D, Point3D]


def face_containing_corners(*points) -> int:
    faces = [
        face_id
        for face_id, face_pts in enumerate(face_vertex_adjacency())
        if all(pt in face_pts for pt in points)
    ]
    if len(faces) != 1:
        raise ValueError('Face not found or ambiguous')
    face, = faces
    return face

@cache
def index_to_corner() -> Sequence[Point3D]:
    return list(product((0, 1), repeat=3))

@cache
def corner_to_index() -> dict[Point3D, int]:
    return {corner: index for index, corner in enumerate(index_to_corner())}

@cache
def index_to_edge() -> list[EdgeDef]:
    all_edges = []
    for changing_dim in range(3):
        for corner in get_four_corners(changing_dim, 0):
            a = corner
            b = list(corner); b[changing_dim] = 1; b = tuple(b)
            x, y, z = a
            a, b = corner_to_index()[a], corner_to_index()[b]

            all_edges.append(EdgeDef(a, b, x, y, z, changing_dim))
    return all_edges

@cache
def face_vertex_adjacency() -> (int, (6, 4)):
    result = []
    for fixed_dim in range(3):
        for fixed_value in (0, 1):
            adj_thisface = [
                corner_to_index()[corner]
                for corner in get_four_corners(fixed_dim=fixed_dim, fixed_value=fixed_value)
            ]
            result.append(adj_thisface)
    return result

@cache
def edge_vertex_adjacency() -> dict[tuple[int, int], int]:
    result = {}
    for i, edge in enumerate(index_to_edge()):
        result[(edge.a, edge.b)] = i
        result[(edge.b, edge.a)] = i
    return result

@cache
def all_permutations() -> list[CubeRotation]:
    all_permutations = [rotation_to_permutation(rot) for rot in all_rotations()]
    return all_permutations

@cache
def cube_geometry() -> CubeGeometry:
    return CubeGeometry(face_vertex_adjacency(), index_to_edge(), all_permutations())


def get_four_corners(fixed_dim: int, fixed_value: int) -> Generator[Point3D, None, None]:
    changing_dims = set(range(3)) - { fixed_dim }
    all_combos = product((0, 1), repeat=2)
    for combo in all_combos:
        xyz = [0, 0, 0]
        xyz[fixed_dim] = fixed_value
        for di, vi in zip(changing_dims, combo):
            xyz[di] = vi
        yield tuple(xyz)


def axis_angle(axis, angle):
    norm_axis = axis / np.linalg.norm(axis)
    return norm_axis * angle


@cache
def all_rotations() -> list[Rotation]:
    all_cube_rotations = []
    all_cube_rotations.append(Rotation.identity())

    for axis in np.eye(3):  # all three unit vectors
        for angle in [np.pi / 2, np.pi, 3*np.pi/2]:
            all_cube_rotations.append(Rotation.from_rotvec(axis_angle(axis, angle)))

    for axis in [
        np.array([1, 1, 0]),
        np.array([0, 1, 1]),
        np.array([1, 0, 1]),
        np.array([-1, 1, 0]),
        np.array([0, -1, 1]),
        np.array([-1, 0, 1]),
    ]:
        all_cube_rotations.append(Rotation.from_rotvec(axis_angle(axis, np.pi)))

    for axis in [
        np.array([1, 1, 1]),
        np.array([1, 1, -1]),
        np.array([1, -1, 1]),
        np.array([-1, 1, 1]),
    ]:
        for angle in [2*np.pi/3, 4*np.pi/3]:
            all_cube_rotations.append(Rotation.from_rotvec(axis_angle(axis, angle)))
    return all_cube_rotations


center = 0.5 * np.ones((3,))


def rotation_to_permutation(rot: Rotation) -> CubeRotation:
    vertex_permutation: Permutation = [None for _ in range(NB_VERTICES)]
    for i in range(NB_VERTICES):
        pt = np.array(index_to_corner()[i])
        pt = 2*(pt - center)
        pt = rot.apply(pt)
        pt = 0.5*pt + center
        pt = np.round(pt, decimals=2)  # to compensate for numeric errors
        permuted_index = corner_to_index()[tuple(pt)]
        vertex_permutation[i] = permuted_index

    edge_permutation: Permutation = [None for _ in range(NB_EDGES)]
    for i in range(NB_EDGES):
        edge = index_to_edge()[i]
        a, b = edge.a, edge.b
        a_perm, b_perm = vertex_permutation[a], vertex_permutation[b]
        permuted_index = edge_vertex_adjacency()[(a_perm, b_perm)]
        edge_permutation[i] = permuted_index

    face_permutation: Permutation = [None for _ in range(NB_FACES)]
    for i in range(NB_FACES):
        face_corners = face_vertex_adjacency()[i]
        permuted_face_corners = [vertex_permutation[i] for i in face_corners]
        permuted_index = face_containing_corners(*permuted_face_corners)
        face_permutation[i] = permuted_index
    # The center permutation is added to the end of the permutation and never changes.
    face_permutation.append(len(face_permutation))

    return CubeRotation(vertex_permutation, edge_permutation, face_permutation)


def inverse_permutation(original: Permutation) -> Permutation:
    inverse = [None for _ in original]
    for i in range(len(original)):
        inverse[original[i]] = i
    return inverse

def inverse_cube(original: CubeRotation) -> CubeRotation:
    return CubeRotation(
        face_permutation=inverse_permutation(original.face_permutation),
        edge_permutation=inverse_permutation(original.edge_permutation),
        vertex_permutation=inverse_permutation(original.vertex_permutation),
    )

def chain_permutation(perm1, perm2) -> Permutation:
    result = [None for _ in range(len(perm1))]
    for i in range(len(perm1)):
        result[i] = perm2[perm1[i]]
    return result


def chain_cube(perm1, perm2) -> CubeRotation:
    return CubeRotation(
        face_permutation=chain_permutation(perm1.face_permutation, perm2.face_permutation),
        edge_permutation=chain_permutation(perm1.edge_permutation, perm2.edge_permutation),
        vertex_permutation=chain_permutation(perm1.vertex_permutation, perm2.vertex_permutation),
    )

def permute(li: list, perm: Permutation) -> list:
    result = [None for _ in li]
    for i, j in enumerate(perm):
        result[j] = li[i]
    return result


def int_to_bits(bitset: int, size: int = 8) -> list[int]:
    return [(bitset >> i) % 2 for i in range(size)]

def bits_to_int(bits: list[int]) -> int:
    return sum((1 << i) for i, bit in enumerate(bits) if bit)

def permute_bitset(bitset: int, perm: Permutation) -> int:
    bits = int_to_bits(bitset, size=len(perm))
    permuted_bits = permute(bits, perm)
    result = bits_to_int(permuted_bits)
    return result
