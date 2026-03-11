import numpy as np
import pytest
from vof_bindings import get_wall_sizes, get_intersect


@pytest.mark.parametrize("seed", list(range(5)))
def test_wall_sizes_full_cube(seed: int):
    rng = np.random.default_rng(seed=seed)
    normal = rng.normal(size=(3,))
    normal /= np.linalg.norm(normal)

    wall_sizes_early, wall_sizes_late = get_wall_sizes(1.0, normal)
    assert np.all(wall_sizes_early == 1.0)
    assert np.all(wall_sizes_late == 1.0)


@pytest.mark.parametrize("seed", list(range(5)))
def test_wall_sizes_empty_cube(seed: int):
    rng = np.random.default_rng(seed=seed)
    normal = rng.normal(size=(3,))
    normal /= np.linalg.norm(normal)

    wall_sizes_early, wall_sizes_late = get_wall_sizes(0.0, normal)
    assert np.all(wall_sizes_early == 0.0)
    assert np.all(wall_sizes_late == 0.0)


@pytest.mark.parametrize("seed", list(range(5)))
def test_normal(seed: int):
    rng = np.random.default_rng(seed=seed)
    normal = rng.normal(size=(3,))
    normal /= np.linalg.norm(normal)

    volume_fraction = rng.uniform(0.01, 0.99)

    ix = get_intersect(volume_fraction, normal)
    points = np.array(
        [
            [ix[0], 0, 0],
            [0, ix[1], 0],
            [0, 0, ix[2]],
            [1, ix[3], 0],
            [1, 0, ix[4]],
            [0, 1, ix[5]],
            [ix[6], 1, 0],
            [ix[7], 0, 1],
            [0, ix[8], 1],
            [1, 1, ix[9]],
            [ix[10], 1, 1],
            [1, ix[11], 1],
        ]
    )
    points = points[np.any((points != 0.0) & (points != 1.0), axis=1)]
    vectors = points[1:] - points[0]
    assert np.linalg.matrix_rank(vectors) < 3, "Points should be coplanar"
    assert len(vectors) >= 2
    normal = np.abs(
        normal
    )  # get_intersect operates on a cube in a "canonical" orientation
    np.testing.assert_allclose(np.dot(vectors, normal), 0, atol=1e-6, rtol=1e-6)


if __name__ == "__main__":
    test_normal(4)
    test_normal(3)
