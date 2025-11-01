from dataclasses import dataclass
import numpy as np

NB_EDGES = 12

@dataclass
class Rotation:
    vertex_permutation: (np.dtype('u1'), 8)
    edge_permutation: (np.dtype('u1'), NB_EDGES)

@dataclass
class EdgeDef:
    a: {"base": int, "bits": 3}
    b: {"base": int, "bits": 3}
    x: {"base": int, "bits": 1}
    y: {"base": int, "bits": 1}
    z: {"base": int, "bits": 1}
    changing_dim: {"base": int, "bits": 2}

@dataclass
class CubeGeometry:
    adjacency: (int, (4, 6))
    edge_definition: (EdgeDef, NB_EDGES)

@dataclass
class Subcase:
    triangles: (int, (9, 3))

@dataclass
class SubcasePtr:
    subcase: int
    sign_flip: bool
    permutation: int

@dataclass
class Case:
    tests: (int, 4)
    subcases: (SubcasePtr, 16)  # up to 16

@dataclass
class CasePtr:
    _case: int
    permutation: int

@dataclass
class LookupTable:
    all_cases: (Case, 14)
    all_subcases: (Subcase, 33)
    all_permutations: (Rotation, 24)
    case_table: (CasePtr, 256)

