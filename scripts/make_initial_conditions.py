import numpy as np
import math

scenarios = {}


def register_scenario(fun):
    assert fun.__name__.startswith("get_")
    scenarios[fun.__name__.removeprefix("get_")] = fun
    return fun


@register_scenario
def get_full(size: int, buffer: np.ndarray):
    buffer[:, :, :] = 1


@register_scenario
def get_still(size: int, buffer: np.ndarray):
    buffer[:, :, :] = 0.0
    buffer[:, :, : math.floor(size / 2)] = 1.0


@register_scenario
def get_dambreak(size: int, buffer: np.ndarray):
    buffer[:, :, :] = 0.0
    buffer[:, : math.ceil(size / 5), :] = 1.0


@register_scenario
def get_drop(size: int, buffer: np.ndarray):
    buffer.fill(0)
    buffer[math.ceil(size / 2), math.ceil(size / 2), math.ceil(size / 2)] = 1.0


@register_scenario
def get_wave(size: int, buffer: np.ndarray):
    buffer.fill(0)
    for i in range(size):
        height = size / 2 + size / 4 * math.sin(i / size)
        buffer[i, :, : math.floor(height)] = 1.0
        buffer[i, :, math.floor(height)] = height - math.floor(height)


if __name__ == "__main__":
    import sys

    size = int(sys.argv[1])

    for name, fun in scenarios.items():
        buffer = np.zeros((size, size, size))
        fun(size, buffer)
        with open(f"data/{name}.npy", "wb") as file:
            np.save(file, buffer)
