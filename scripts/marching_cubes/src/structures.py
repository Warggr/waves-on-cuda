from dataclasses import dataclass
import numpy as np

NB_VERTICES = 8
NB_EDGES = 12
NB_FACES = 6

@dataclass
class CubeRotation:
    vertex_permutation: (np.dtype('u1'), NB_VERTICES)
    edge_permutation: (np.dtype('u1'), NB_EDGES)
    face_permutation: (np.dtype('u1'), NB_FACES + 1)

@dataclass
class EdgeDef:
    a: {"base": np.dtype('u4'), "bits": 3}
    b: {"base": np.dtype('u4'), "bits": 3}
    x: {"base": np.dtype('u4'), "bits": 1}
    y: {"base": np.dtype('u4'), "bits": 1}
    z: {"base": np.dtype('u4'), "bits": 1}
    changing_dim: {"base": np.dtype('u4'), "bits": 2}

@dataclass
class CubeGeometry:
    adjacency: (np.dtype('u1'), (6, 4))
    edge_definition: (EdgeDef, NB_EDGES)
    all_permutations: (CubeRotation, 24)

@dataclass
class Subcase:
    triangles: (np.dtype('u1'), (12, 3))
    num_triangles: np.dtype('u1')

@dataclass
class SubcasePtr:
    subcase: np.dtype('u1')
    permutation: np.dtype('u1')
    sign_flip: bool

@dataclass
class Case:
    tests: (np.dtype('u1'), 7)
    num_tests: np.dtype('u1')
    subcases: (SubcasePtr, 128)

@dataclass
class CasePtr:
    _case: int
    permutation: int
    sign_flip: bool

@dataclass
class LookupTable:
    all_cases: (Case, 15)
    all_subcases: (Subcase, 33)
    case_table: (CasePtr, 256)
