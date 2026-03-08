from dataclasses import dataclass
import numpy as np
from typing import Annotated
from .type_hints import Array


NB_VERTICES = 8
NB_EDGES = 12
NB_FACES = 6


@dataclass
class CubeRotation:
    vertex_permutation: Array[np.dtype("u1"), NB_VERTICES]
    edge_permutation: Array[np.dtype("u1"), NB_EDGES]
    face_permutation: Array[np.dtype("u1"), NB_FACES + 1]


@dataclass
class EdgeDef:
    a: Annotated[np.dtype("u4"), {"bits": 3}]
    b: Annotated[np.dtype("u4"), {"bits": 3}]
    x: Annotated[np.dtype("u4"), {"bits": 1}]
    y: Annotated[np.dtype("u4"), {"bits": 1}]
    z: Annotated[np.dtype("u4"), {"bits": 1}]
    changing_dim: Annotated[np.dtype("u4"), {"bits": 2}]


@dataclass
class CubeGeometry:
    adjacency: Array[np.dtype("u1"), (6, 4)]
    edge_definition: Array[EdgeDef, NB_EDGES]
    all_permutations: Array[CubeRotation, 24]


@dataclass
class Subcase:
    triangles: Array[np.dtype("u1"), (12, 3)]
    num_triangles: np.dtype("u1")


@dataclass
class SubcasePtr:
    subcase: np.dtype("u1")
    permutation: np.dtype("u1")
    sign_flip: bool


@dataclass
class Case:
    tests: Array[np.dtype("u1"), 7]
    num_tests: np.dtype("u1")
    subcases: Array[SubcasePtr, 128]


@dataclass
class CasePtr:
    _case: int
    permutation: int
    sign_flip: bool


@dataclass
class LookupTable:
    all_cases: Array[Case, 15]
    all_subcases: Array[Subcase, 33]
    case_table: Array[CasePtr, 256]
