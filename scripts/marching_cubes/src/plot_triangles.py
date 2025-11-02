# coding: utf-8
from typing import Callable
import matplotlib.pyplot as plt
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


def marching_cube(ax, v: list[float], isoLevel: float=0):
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
            test += 1 if True else 0 # TODO
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
    if len(triangles) == 0:
        return
    ax.set_title(f'Case {case_ptr._case}, subcase {subcase_ptr.subcase}')
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


def plot_all_subcases():
    fig, axes = plt.subplots(3, 11, subplot_kw=dict(projection='3d'))
    axes = axes.flatten()

    for ax, subcase in zip(axes, lookup_table.all_subcases):
        draw_cube(ax)
        draw_triangles(ax, subcase.triangles)

    plt.show()

def interactive_plot():
    fig = plt.figure()
    ax = fig.add_subplot(projection='3d')
    fig.subplots_adjust(left=0.4)

    initial_v = np.zeros(NB_VERTICES)

    def update(i, val):
        initial_v[i] = val
        ax.clear()
        ax.set_xlim(0, 1); ax.set_ylim(0, 1); ax.set_zlim(0, 1)
        draw_cube(ax, initial_v)
        marching_cube(ax, initial_v)
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
