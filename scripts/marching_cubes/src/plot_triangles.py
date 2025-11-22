# coding: utf-8
import itertools
from typing import Callable, Sequence
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
from matplotlib.widgets import Slider, Button

from .lookup_tables import CENTER_EDGEINDEX, EdgeIndex, Midpoint, lookup_table
from .rotations import Point3D, cube_geometry, index_to_corner, bits_to_int, index_to_edge, int_to_bits, permute
from .structures import NB_VERTICES, NB_EDGES

index_to_corner = np.array(index_to_corner())


def position(edge: Midpoint):
    a = index_to_corner[edge[0], :]
    b = index_to_corner[edge[1], :]
    return (a + b) / 2


def draw_cube(ax, corners: list[float] | None = None):
    kwargs = {}
    if corners is not None:
        kwargs['c'] = corners
        kwargs['cmap'] = 'RdBu'
        kwargs['vmin'] = -1; kwargs['vmax'] = 1
    ax.scatter(index_to_corner[:, 0], index_to_corner[:, 1], index_to_corner[:, 2], **kwargs)


def resolve_point(p: EdgeIndex) -> Point3D:
    if p == CENTER_EDGEINDEX:
        return 0.5 * np.ones(3)
    else:
        edge = index_to_edge()[p]
        a, b = index_to_corner[edge.a, :], index_to_corner[edge.b, :]
        return (a + b) / 2

def draw_triangles(ax, ts: list[tuple[EdgeIndex, EdgeIndex, EdgeIndex]]):
    if len(ts) == 0:
        return
    points = np.zeros((len(ts) * 3, 3))
    triangles = np.zeros((len(ts), 3))
    row = 0
    for i, triangle in enumerate(ts):
        for j, pt in enumerate(triangle):
            points[row] = resolve_point(pt)
            triangles[i][j] = row
            row += 1

    ax.plot_trisurf(
        points[:, 0], points[:, 1], points[:, 2],
        triangles=triangles,
    )


def interior_test(v: list[float]) -> bool:
    """Interior test between vertices 1 and 7."""
    a0, d0, b0, c0, a1, d1, b1, c1 = v
    a = (a1-a0)*(c1-c0) - (b1-b0)*(d1-d0)
    b = c0*(a1-a0) + a0*(c1-c0) - d0*(b1-b0) - b0*(d1-d0)
    # c = a0*c0 - b0*d0
    if a >= 0:
        t = -0.5*b/a
        if 0 <= t and t <= 1:
            At = a0 + t*(a1 - a0)
            Bt = b0 + t*(b1 - b0)
            Ct = c0 + t*(c1 - c0)
            Dt = d0 + t*(d1 - d0)
            if (Ct > 0) == (At > 0) and (At*Ct - Bt*Dt > 0):
                return ((At > 0) == (a0 > 0)) or ((Ct > 0) == (c0 > 0));
    return False


def get_marching_cube_triangles(v: list[float], isoLevel: float=0) -> tuple[np.ndarray, list[tuple[int, int, int]], str, str]:
    bits = [(vi > isoLevel) for vi in v]
    index_ = bits_to_int(bits)
    intersect = []
    for i in range(NB_EDGES):
        edge = cube_geometry().edge_definition[i]
        a, b = v[edge.a], v[edge.b]
        midpoint = [edge.x, edge.y, edge.z]
        if a != b:
            midpoint[edge.changing_dim] = (a - isoLevel) / (a - b)
        intersect.append(midpoint)
    case_ptr = lookup_table.case_table[index_]
    intersect = permute(intersect, cube_geometry().all_permutations[case_ptr.permutation].edge_permutation)
    v = permute(v, cube_geometry().all_permutations[case_ptr.permutation].vertex_permutation)
    sign_flip = case_ptr.sign_flip

    case = lookup_table.all_cases[case_ptr._case]
    test = 0
    for i in range(case.num_tests):
        side = case.tests[i]
        if side == 6:
            test += (1 << i) if interior_test(v) else 0
        else:
            a = v[cube_geometry().adjacency[side][0]]
            b = v[cube_geometry().adjacency[side][1]]
            c = v[cube_geometry().adjacency[side][2]]
            d = v[cube_geometry().adjacency[side][3]]
            test += (1 << i) if (a*c - b*d) > isoLevel else 0
    subcase_ptr = case.subcases[test]
    sign_flip = sign_flip ^ subcase_ptr.sign_flip
    subcase = lookup_table.all_subcases[subcase_ptr.subcase]
    intersect = permute(intersect, cube_geometry().all_permutations[subcase_ptr.permutation].edge_permutation)
    intersect.append([0.5, 0.5, 0.5])

    intersect = np.array(intersect)
    triangles = []
    for i in range(subcase.num_triangles):
        triangle_of_indices = subcase.triangles[i]
        if sign_flip:
            triangle_of_indices = permute(triangle_of_indices, [2, 1, 0])
        triangles.append(triangle_of_indices)
        for index in triangle_of_indices:
            assert np.all(0 <= intersect[index]) and np.all(intersect[index] <= 1), (index, intersect)
    return intersect, triangles, f'Case {case_ptr._case}', f'subcase {subcase_ptr.subcase}'


def plot_marching_cube(ax: Axes3D, v: list[float], isoLevel: float = 0) -> None:
    intersect, triangles, case, subcase = get_marching_cube_triangles(v, isoLevel=isoLevel)
    ax.set_title(f'{case}, {subcase}')
    ax.plot_trisurf(
        intersect[:, 0], intersect[:, 1], intersect[:, 2],
        triangles=triangles,
    )


def slider_panel(
    fig, n: int,
    vmin=-1.0, vmax=1.0,
    initial=None, # defaults to 0
    on_change: Callable[[int, float], None]|None=None,
    startx=0.1, width=0.03 # in figure coordinates
) -> list[Slider]:
    if initial is None:
        initial = np.zeros(n)
    sliders = []
    for i in range(n):
        # Each slider gets its own axes area
        ax_slider = fig.add_axes([startx + i * width, 0.25, width * 0.8, 0.6])
        s = Slider(
            ax_slider,
            label=str(i),
            valmin=vmin,
            valmax=vmax,
            valinit=float(initial[i]),
            orientation="vertical",
        )

        # Capture i and slider strongly using a lambda with defaults
        if on_change is not None:
            s.on_changed(lambda val, i=i, sref=s: on_change(i, val))

        sliders.append(s)
    return sliders


def plot_trilinear(ax, v: list[float], isoLevel: float = 0,
    nx=9,
    ny=9,
    nz=9,
):
    x = np.linspace(0, 1, nx+1)
    y = np.linspace(0, 1, ny+1)
    z = np.linspace(0, 1, nz+1)
    X, Y, Z = np.meshgrid(x, y, z)
    # Compute trilinear interpolant
    F = (
        v[0]*(1-X)*(1-Y)*(1-Z) +
        v[4]*X*(1-Y)*(1-Z) +
        v[2]*(1-X)*Y*(1-Z) +
        v[6]*X*Y*(1-Z) +
        v[1]*(1-X)*(1-Y)*Z +
        v[5]*X*(1-Y)*Z +
        v[3]*(1-X)*Y*Z +
        v[7]*X*Y*Z
    )
    for i in range(nx):
        for j in range(ny):
            for k in range(nz):
                v = [F[i+di, j+dj, k+dk] for di, dj, dk in itertools.product((0,1), repeat=3)]
                intersect, triangles, _, _ = get_marching_cube_triangles(v, isoLevel=isoLevel)
                if len(triangles) == 0:
                    continue
                ax.plot_trisurf(
                    (intersect[:, 0] + i) / (nx+1),
                    (intersect[:, 1] + j) / (ny+1),
                    (intersect[:, 2] + k) / (nz+1),
                    triangles=triangles,
                    color="b",
                )


def plot_all_subcases():
    fig, axes = plt.subplots(3, 11, subplot_kw=dict(projection='3d'))
    axes = axes.flatten()

    for ax, subcase in zip(axes, lookup_table.all_subcases):
        draw_cube(ax)
        draw_triangles(ax, subcase.triangles)

    plt.show()


def interactive_plot(plot_functions: Sequence[Callable[[Axes3D, list[float], float], None]] = [plot_marching_cube, plot_trilinear]):
    fig, axes = plt.subplots(1, len(plot_functions), subplot_kw=dict(projection='3d'))
    axes = axes.flatten()
    fig.subplots_adjust(left=0.4)

    initial_v = np.zeros(NB_VERTICES)

    def update(i, val):
        initial_v[i] = val
        for ax, plotter in zip(axes, plot_functions):
            ax.clear()
            ax.set_xlim(0, 1); ax.set_ylim(0, 1); ax.set_zlim(0, 1)
            draw_cube(ax, initial_v)
            plotter(ax, initial_v, 0)
        fig.canvas.draw_idle()

    sliders = slider_panel(fig, NB_VERTICES, vmin=-1, vmax=1, initial=initial_v, on_change=update)

    resetax = fig.add_axes([0.8, 0.025, 0.1, 0.04])
    button = Button(resetax, 'Next pattern', hovercolor='0.975')

    def next_pattern(event):
        bits = [(vi > 0) for vi in initial_v]
        this_pattern = bits_to_int(bits)
        next_pattern = (this_pattern + 1) % 256
        next_bits = int_to_bits(next_pattern)
        new_values = np.array(next_bits) - 0.5
        for i in range(8):
            initial_v[i] = new_values[i]
            sliders[i].set_val(new_values[i])

    button.on_clicked(next_pattern)

    plt.show()

if __name__ == "__main__":
    interactive_plot()
