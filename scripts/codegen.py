from dataclasses import is_dataclass, fields
from collections.abc import Sequence
from jinja2 import Environment, FileSystemLoader
import sys
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
    elif type(t) is tuple:
        base, shape = t
        base, suff = type_to_ctype(base)
        if type(shape) is int:
            base = f'std::array<{base}, {shape}>'
        else:
            for d in shape:
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


def c_init(value, indent=0):
    """Recursively format Python dataclass/containers into C initializers."""
    space = " " * indent

    # None
    if value is None:
        return "0"

    # Primitive
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, bool):
        return "True" if value else "False"

    if isinstance(value, str):
        return f"\"{value}\""

    if is_dataclass(value):
        parts = []
        for f in fields(value):
            fv = getattr(value, f.name)
            parts.append(f".{f.name} = {c_init(fv, indent + 2)}")
        inner = ", ".join(parts)
        return f"{{ {inner} }}"

    if isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        elems = ", ".join(c_init(v, indent + 2) for v in value)
        return "{{" + elems + "}}"

    raise TypeError(f"Unsupported type: {type(value)}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("what", choices=["h", "c"])
    parser.add_argument("-o", type=Path, help="output path")
    args = parser.parse_args()
    
    if args.what == "h":
        out_default_filename = Path("generated") / "cache.h"
        output = ["#pragma once", "#include <array>"]
        import structures

        for symbol, obj in vars(structures).items():
            if symbol.startswith('__') or str(type(obj)) == "<class 'module'>":
                continue
            if hasattr(obj, '__module__') and obj.__module__ != "structures":
                continue
            if type(obj) in (int, float):
                output.append(f"constexpr {type(obj).__name__} {symbol} = {obj};")
            elif type(obj) is type:
                output.append(c_struct_def(obj, symbol))
        output.append("extern struct LookupTable lookup_table; extern struct CubeGeometry cube_geometry;")
        output = '\n'.join(output)
    else:
        from structures import LookupTable, CubeGeometry, EdgeDef, NB_EDGES

        cube_geometry = CubeGeometry(
            adjacency=[[0]*4 for _ in range(6)],
            edge_definition=[EdgeDef(a=0,b=1,x=0,y=0,z=0,changing_dim=0) for _ in range(NB_EDGES)]
        )

        lookup_table = LookupTable(
            all_cases=[],
            all_subcases=[],
            all_permutations=[],
            case_table=[]
        )

        env = Environment(loader=FileSystemLoader(Path(__file__).parent), trim_blocks=True, lstrip_blocks=True)
        env.globals['c_init'] = c_init
        template = env.get_template("cache.cpp.jinja")
        output = template.render(cube_geometry=cube_geometry, lookup_table=lookup_table, CubeGeometry=CubeGeometry, LookupTable=LookupTable)
        out_default_filename = Path("generated") / "cache.c"

    out_file = args.o or out_default_filename
    out_file.parent.mkdir(exist_ok=True, parents=True)

    with open(out_file, "w") as f:
        f.write(output)
