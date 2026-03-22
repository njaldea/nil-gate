from __future__ import annotations

import gc
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
PY_FFI_DIR = REPO_ROOT / "src" / "ffi" / "python"

sys.path.insert(0, str(PY_FFI_DIR))

import nil_gate

def call_gc(tag: int) -> None:
    before = len(gc.get_objects())
    print("Objects before:", tag, before)
    gc.collect()
    after = len(gc.get_objects())
    print("Objects after :", tag, after)


def main() -> None:
    core = nil_gate.create_core()

    options = {
        "check_mem": False,
        "check_post": True,
    }

    if options["check_mem"]:
        def exec_cb(graph: nil_gate.Graph) -> None:
            print("exec callback called")

        for u in range(1, 201):
            for _ in range(1, 201):
                core.post(exec_cb)
                core.post(exec_cb)
                core.commit()
            if u % 25 == 0:
                print("mem loop progress:", u)

        call_gc(0)

    if options["check_post"]:
        def data_eq(left: Any, right: Any) -> bool:
            return left == right

        port: nil_gate.EPort = None
        node_port: nil_gate.Node = None

        def setup_graph(graph: nil_gate.Graph) -> None:
            nonlocal port, node_port
            print("- exec callback called")
            port = graph.port(data_eq, 100)

            node_port = graph.node(
                lambda args: (
                    print("node 1 called", args.core, len(args.inputs), len(args.outputs)),
                    args.outputs[0].set_value(args.inputs[0]),
                ),
                [port],
                [data_eq],
            ).outputs()[0]

            graph.node(
                lambda args: print(
                    "node 2 called", args.core, args.inputs[0], len(args.outputs)
                ),
                [node_port],
                [],
            )

            for _ in range(1, 201):
                print("- to mport")
                mport = port.to_direct()
                print("- has_value")
                print(mport.has_value())
                print("- set_value")
                mport.set_value(100)
                print("- has_value")
                print(mport.has_value())
                print("- value")
                print(mport.value())
                print("- done")

        core.post(setup_graph)
        core.commit()

        core.post(lambda graph: (print("- exec callback called"), port.to_direct().set_value(200)))
        core.commit()
        call_gc(0)

    call_gc(0)
    core.destroy()
    call_gc(0)


if __name__ == "__main__":
    main()
