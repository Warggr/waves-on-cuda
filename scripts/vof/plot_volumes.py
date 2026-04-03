import numpy as np
import itertools
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection
from utils.plotting import slider_panel
from typing import Callable, Sequence
from vof_bindings import get_intersect, baselines, switching_dim

dx = np.zeros((switching_dim.size, 3))
dx[np.arange(switching_dim.size), switching_dim] = 1

endlines = baselines + dx


def draw_unit_cube_edges(ax):
    edges = np.array(list(zip(baselines, endlines)))
    ax.add_collection3d(Line3DCollection(edges, colors="black", linewidths=1))


def draw_surface(ax: Axes3D, pts: list[np.ndarray]):
    pts = np.unique(np.array(pts), axis=0)
    if len(pts) < 3:
        return
    center = pts.mean(axis=0)
    vecs = pts - center
    planar_coords, _, _ = np.linalg.svd(vecs)
    angles = np.arctan2(planar_coords[:, 1], planar_coords[:, 0])
    pts = pts[np.argsort(angles)]
    triangles = [(0, i, j) for i, j in itertools.pairwise(range(len(pts)))]
    ax.plot_trisurf(
        pts[:, 0], pts[:, 1], pts[:, 2], triangles=triangles, color="blue", alpha=0.3
    )


def draw_cube(ax: Axes3D, values: np.ndarray):
    draw_unit_cube_edges(ax)

    normal = np.array(values[:3])
    normal /= np.linalg.norm(normal)
    intersect, switched_signs = get_intersect(values[3], normal)
    switched_signs = switched_signs[:, np.newaxis]
    segments = dx * intersect[:, np.newaxis]
    starts = np.where(~switched_signs, baselines, endlines)
    stops = np.where(~switched_signs, baselines + segments, endlines - segments)
    col = Line3DCollection(
        [np.array([pt, pt_base]) for pt, pt_base in zip(starts, stops)],
        colors="blue",
        linewidths=3,
    )
    ax.add_collection3d(col)
    filter = np.any(starts != stops, axis=1)
    pts = np.concat([starts[filter], stops[filter]], axis=0)
    for dim in range(3):
        pts_i = pts[pts[:, dim] == 0]
        draw_surface(ax, pts_i)
        pts_i = pts[pts[:, dim] == 1]
        draw_surface(ax, pts_i)
    pts_i = pts[np.any(np.logical_and(pts != 0.0, pts != 1.0, ~np.isnan(pts)), axis=1)]
    draw_surface(ax, pts_i)
    ax.quiver3D(0.5, 0.5, 0.5, *(0.5 * normal), color="red")


def interactive_plot(
    plot_functions: Sequence[Callable[[Axes3D, np.ndarray], None]] = [
        draw_cube,
    ],
):
    fig, axes = plt.subplots(1, len(plot_functions), subplot_kw=dict(projection="3d"))
    if len(plot_functions) > 1:
        axes = axes.flatten()
    else:
        axes = [axes]
    fig.subplots_adjust(left=0.4)

    initial_v = np.zeros(4)

    def update(i, val):
        initial_v[i] = val
        for ax, plotter in zip(axes, plot_functions):
            ax.clear()
            ax.set_xlim(0, 1)
            ax.set_ylim(0, 1)
            ax.set_zlim(0, 1)
            plotter(ax, initial_v)
        fig.canvas.draw_idle()

    bounds = [(-1.0, 1.0)] * 3 + [(0.0, 1.0)]
    slider_panel(fig, 4, bounds=bounds, initial=initial_v, on_change=update)

    plt.show()


if __name__ == "__main__":
    interactive_plot()
