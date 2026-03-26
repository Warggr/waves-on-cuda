import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolkits.mplot3d.art3d import Line3DCollection
from utils.plotting import slider_panel
from typing import Callable, Sequence
from vof_bindings import get_intersect, baselines, switching_dim

dx = np.zeros((switching_dim.size, 3))
dx[np.arange(switching_dim.size), switching_dim] = 1

endlines = baselines + dx


def draw_cube(ax: Axes3D, values: np.ndarray):
    normal = np.array(values[:3])
    normal /= np.linalg.norm(normal)
    intersect, switched_signs = get_intersect(values[3], normal)
    switched_signs = switched_signs[:, np.newaxis]
    segments = dx * intersect[:, np.newaxis]
    starts = np.where(~switched_signs, baselines, endlines)
    stops = np.where(~switched_signs, baselines + segments, endlines - segments)
    col = Line3DCollection(
        [np.array([pt, pt_base]) for pt, pt_base in zip(starts, stops)]
    )
    ax.add_collection3d(col)


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
