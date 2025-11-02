from dataclasses import is_dataclass, fields
from collections.abc import Sequence
from jinja2 import Environment, FileSystemLoader
from pathlib import Path
import numpy as np


def type_to_ctype(t):
    if t in (int,):
        return "int", ""
    elif t in (float,):
        return "float", ""
    elif t in (bool,):
        return "bool", ""
    elif t in (np.dtype('u1'),):
        return "unsigned char", ""
    elif t in (np.dtype('u4'),):
        return "unsigned int", ""
    elif type(t) is tuple:
        base, shape = t
        base, suff = type_to_ctype(base)
        if type(shape) is int:
            base = f'std::array<{base}, {shape}>'
        else:
            for d in reversed(shape):
                base = f'std::array<{base}, {d}>'
        return base, suff
    elif type(t) is dict:
        base, suff = type_to_ctype(t["base"])
        assert suff == ""
        return base, f": {t['bits']}"
    elif type(t) is type:
        return 'struct ' + t.__name__, ''
    else:
        raise ValueError(repr(t))

def c_struct_def(cls, name):
    assert is_dataclass(cls)
    lines = [f"struct {name} {{"]
    suffix = ""
    for f in fields(cls):
        t = f.type
        ctype, suffix = type_to_ctype(t)
        lines.append(f"    {ctype} {f.name} {suffix};")
    lines.append("};")
    return "\n".join(lines)


def c_init(value, indent=0) -> str:
    """Recursively format Python dataclass/containers into C initializers."""
    spaces = " " * indent
    if value is None:
        result = "0"
    # For some reason isinstance(True, int) = True, so we first need to check for bool
    elif isinstance(value, bool):
        result = "1" if value else "0"
    elif isinstance(value, (int, float)):
        result = str(value)
    elif isinstance(value, str):
        result = f"\"{value}\""
    elif is_dataclass(value):
        parts = []
        for f in fields(value):
            fv = getattr(value, f.name)
            parts.append(f".{f.name} = {c_init(fv, indent + 2)}")
        inner = (",\n" + spaces).join(parts)
        result = "{" + inner + "\n" + spaces + "}"
    elif isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        elems = (",\n" + spaces).join(c_init(v, indent + 2) for v in value)
        result = "{{" + elems + "\n" + spaces + "}}"
    else:
        raise TypeError(f"Unsupported type: {type(value)}")
    assert "True" not in result
    return result


def get_c_lib(**kwargs):
    env = Environment(loader=FileSystemLoader(Path(__file__).parent.parent), trim_blocks=True, lstrip_blocks=True)
    env.globals['c_init'] = c_init
    template = env.get_template("cache.cpp.jinja")
    output = template.render(**kwargs)
    return output


def get_c_header():
    output = ["#pragma once", "#include <array>"]
    from . import structures

    for symbol, obj in vars(structures).items():
        if symbol.startswith('__') or str(type(obj)) == "<class 'module'>":
            continue
        if hasattr(obj, '__module__') and not obj.__module__.endswith("structures"):
            continue
        if type(obj) in (int, float):
            output.append(f"constexpr {type(obj).__name__} {symbol} = {obj};")
        elif type(obj) is type:
            output.append(c_struct_def(obj, symbol))
    output.append("extern struct LookupTable lookup_table; extern struct CubeGeometry cube_geometry;")
    return '\n'.join(output)


if __name__ == "__main__":
    import argparse
    from .rotations import cube_geometry as get_cube_geometry

    parser = argparse.ArgumentParser()
    parser.add_argument("what", choices=["h", "c"])
    parser.add_argument("-o", type=Path, help="output path")
    args = parser.parse_args()

    if args.what == "h":
        output = get_c_header()
        out_default_filename = Path("generated") / "cache.h"

    else:
        from .lookup_tables import lookup_table

        cube_geometry = get_cube_geometry()

        output = get_c_lib(cube_geometry=cube_geometry, lookup_table=lookup_table)
        out_default_filename = Path("generated") / "cache.c"

    out_file = args.o or out_default_filename
    out_file.parent.mkdir(exist_ok=True, parents=True)

    with open(out_file, "w") as f:
        f.write(output)
