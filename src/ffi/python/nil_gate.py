from __future__ import annotations

import ctypes
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional

TypeEq = Callable[[Any, Any], bool]
NodeFn = Callable[["NodeArgs"], None]
PostFn = Callable[["Graph"], None]


class nil_gateCore(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


class nil_gateGraph(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]


NIL_GATE_CORE_CALLABLE_EXEC = ctypes.CFUNCTYPE(None, ctypes.POINTER(nil_gateGraph), ctypes.c_void_p)
NIL_GATE_CLEANUP = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
NIL_GATE_EQ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


class nil_gateCoreCallable(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_GATE_CORE_CALLABLE_EXEC),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_GATE_CLEANUP),
    ]


class nil_gatePortInfo(ctypes.Structure):
    _fields_ = [
        ("eq", NIL_GATE_EQ),
        ("destroy", NIL_GATE_CLEANUP),
    ]


class nil_gateEPort(ctypes.Structure):
    _fields_ = [
        ("handle", ctypes.c_void_p),
        ("info", nil_gatePortInfo),
    ]


class nil_gatePortInfos(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint8),
        ("infos", ctypes.POINTER(nil_gatePortInfo)),
    ]


class nil_gateRPort(ctypes.Structure):
    _fields_ = [
        ("handle", ctypes.c_void_p),
        ("info", nil_gatePortInfo),
    ]


class nil_gateRPorts(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint8),
        ("ports", ctypes.POINTER(nil_gateRPort)),
    ]


class nil_gateMPort(ctypes.Structure):
    _fields_ = [
        ("handle", ctypes.c_void_p),
        ("info", nil_gatePortInfo),
    ]


class nil_gateMPorts(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint8),
        ("ports", ctypes.POINTER(nil_gateMPort)),
    ]


class nil_gateNodeArgsInputs(ctypes.Structure):
    _fields_ = [
        ("size", ctypes.c_uint8),
        ("data", ctypes.POINTER(ctypes.c_void_p)),
    ]


class nil_gateNodeArgs(ctypes.Structure):
    _fields_ = [
        ("core", nil_gateCore),
        ("inputs", nil_gateNodeArgsInputs),
        ("outputs", nil_gateMPorts),
        ("context", ctypes.c_void_p),
    ]


NIL_GATE_NODE_EXEC = ctypes.CFUNCTYPE(None, ctypes.POINTER(nil_gateNodeArgs))


class nil_gateNodeInfo(ctypes.Structure):
    _fields_ = [
        ("exec", NIL_GATE_NODE_EXEC),
        ("inputs", nil_gateRPorts),
        ("outputs", nil_gatePortInfos),
        ("context", ctypes.c_void_p),
        ("cleanup", NIL_GATE_CLEANUP),
    ]


class nil_gateNode(ctypes.Structure):
    _fields_ = [("handle", ctypes.c_void_p)]

def _to_ref_id(ptr: Any) -> int:
    return int(ctypes.cast(ptr, ctypes.c_void_p).value)

@dataclass
class _PortState:
    __slots__ = ("value", "eq")
    value: Any
    eq: TypeEq

@dataclass
class _NodeState:
    __slots__ = ("fn", "outputs")
    fn: NodeFn
    outputs: List[TypeEq]

@dataclass
class _CallableState:
    __slots__ = ("fn")
    fn: PostFn

RefState = _PortState | _NodeState | _CallableState

@dataclass
class _FnsState:
    __slots__ = ("post_exec", "node_exec", "port_eq", "cleanup", "to_core_callable")
    post_exec: Any
    node_exec: Any
    port_eq: Any
    cleanup: Any
    to_core_callable: Any


@dataclass
class NodeArgs:
    __slots__ = ("core", "inputs", "outputs")
    core: "Core"
    inputs: List[Any]
    outputs: List["MPort"]


def _configure_signatures(gate: Any) -> None:
    gate.nil_gate_core_create.argtypes = []
    gate.nil_gate_core_create.restype = nil_gateCore

    gate.nil_gate_core_destroy.argtypes = [nil_gateCore]
    gate.nil_gate_core_destroy.restype = None

    gate.nil_gate_core_commit.argtypes = [nil_gateCore]
    gate.nil_gate_core_commit.restype = None

    gate.nil_gate_core_set_runner_immediate.argtypes = [nil_gateCore]
    gate.nil_gate_core_set_runner_immediate.restype = None

    gate.nil_gate_core_set_runner_soft_blocking.argtypes = [nil_gateCore]
    gate.nil_gate_core_set_runner_soft_blocking.restype = None

    gate.nil_gate_core_set_runner_async.argtypes = [nil_gateCore, ctypes.c_uint32]
    gate.nil_gate_core_set_runner_async.restype = None

    gate.nil_gate_core_unset_runner.argtypes = [nil_gateCore]
    gate.nil_gate_core_unset_runner.restype = None

    gate.nil_gate_core_post.argtypes = [nil_gateCore, nil_gateCoreCallable]
    gate.nil_gate_core_post.restype = None

    gate.nil_gate_core_apply.argtypes = [nil_gateCore, nil_gateCoreCallable]
    gate.nil_gate_core_apply.restype = None

    gate.nil_gate_graph_port.argtypes = [nil_gateGraph, nil_gatePortInfo, ctypes.c_void_p]
    gate.nil_gate_graph_port.restype = nil_gateEPort

    gate.nil_gate_graph_node.argtypes = [nil_gateGraph, nil_gateNodeInfo]
    gate.nil_gate_graph_node.restype = nil_gateNode

    gate.nil_gate_node_output_size.argtypes = [nil_gateNode]
    gate.nil_gate_node_output_size.restype = ctypes.c_uint8

    gate.nil_gate_node_outputs.argtypes = [nil_gateNode, ctypes.POINTER(nil_gateRPort)]
    gate.nil_gate_node_outputs.restype = None

    gate.nil_gate_rport_value.argtypes = [nil_gateRPort]
    gate.nil_gate_rport_value.restype = ctypes.c_void_p

    gate.nil_gate_rport_has_value.argtypes = [nil_gateRPort]
    gate.nil_gate_rport_has_value.restype = ctypes.c_int

    gate.nil_gate_mport_set_value.argtypes = [nil_gateMPort, ctypes.c_void_p]
    gate.nil_gate_mport_set_value.restype = None

    gate.nil_gate_mport_unset_value.argtypes = [nil_gateMPort]
    gate.nil_gate_mport_unset_value.restype = None

    gate.nil_gate_mport_as_input.argtypes = [nil_gateMPort]
    gate.nil_gate_mport_as_input.restype = nil_gateRPort

    gate.nil_gate_rport_as_input.argtypes = [nil_gateRPort]
    gate.nil_gate_rport_as_input.restype = nil_gateRPort

    gate.nil_gate_eport_as_input.argtypes = [nil_gateEPort]
    gate.nil_gate_eport_as_input.restype = nil_gateRPort

    gate.nil_gate_eport_to_direct.argtypes = [nil_gateEPort]
    gate.nil_gate_eport_to_direct.restype = nil_gateMPort


class RPort:
    __slots__ = ("_refs", "_gate", "_port")

    def __init__(self, refs: Dict[int, RefState], gate: Any, rport: nil_gateRPort) -> None:
        self._refs = refs
        self._gate = gate
        self._port = rport

    def value(self) -> Any:
        rid = self._gate.nil_gate_rport_value(self._port)
        return self._refs[_to_ref_id(rid)].value

    def has_value(self) -> bool:
        return self._gate.nil_gate_rport_has_value(self._port) != 0

    def as_input(self) -> "RPort":
        return self


class MPort:
    __slots__ = ("_refs", "_gate", "_libc", "_eq", "_port")

    def __init__(self, refs: Dict[int, RefState], gate: Any, libc: Any, mport: nil_gateMPort, eq: TypeEq) -> None:
        self._refs = refs
        self._gate = gate
        self._libc = libc
        self._eq = eq
        self._port = mport

    def value(self) -> Any:
        rport = self._gate.nil_gate_mport_as_input(self._port)
        rid = self._gate.nil_gate_rport_value(rport)
        return self._refs[_to_ref_id(rid)].value

    def has_value(self) -> bool:
        rport = self._gate.nil_gate_mport_as_input(self._port)
        return self._gate.nil_gate_rport_has_value(rport) != 0

    def set_value(self, new_value: Any) -> None:
        new_id = self._libc.malloc(1)
        if not new_id:
            raise MemoryError("malloc failed")
        self._refs[_to_ref_id(new_id)] = _PortState(value=new_value, eq=self._eq)
        self._gate.nil_gate_mport_set_value(self._port, new_id)

    def unset_value(self) -> None:
        self._gate.nil_gate_mport_unset_value(self._port)

    def as_input(self) -> RPort:
        return RPort(self._refs, self._gate, self._gate.nil_gate_mport_as_input(self._port))


class EPort:
    __slots__ = ("_refs", "_gate", "_libc", "_eq", "_port")

    def __init__(self, refs: Dict[int, RefState], gate: Any, libc: Any, eport: nil_gateEPort, eq: TypeEq) -> None:
        self._refs = refs
        self._gate = gate
        self._libc = libc
        self._eq = eq
        self._port = eport

    def to_direct(self) -> MPort:
        mport = self._gate.nil_gate_eport_to_direct(self._port)
        return MPort(self._refs, self._gate, self._libc, mport, self._eq)

    def as_input(self) -> RPort:
        return RPort(self._refs, self._gate, self._gate.nil_gate_eport_as_input(self._port))


class Node:
    __slots__ = ("_refs", "_gate", "_node")

    def __init__(self, refs: Dict[int, RefState], gate: Any, node: nil_gateNode) -> None:
        self._refs = refs
        self._gate = gate
        self._node = node

    def outputs(self) -> List[RPort]:
        output_size = int(self._gate.nil_gate_node_output_size(self._node))
        out: List[RPort] = []
        if output_size > 0:
            rports = (nil_gateRPort * output_size)()
            self._gate.nil_gate_node_outputs(self._node, rports)
            for i in range(output_size):
                out.append(RPort(self._refs, self._gate, rports[i]))
        return out


class Graph:
    __slots__ = ("_refs", "_fns", "_gate", "_libc", "_graph")

    def __init__(self, refs: Dict[int, RefState], fns: _FnsState, gate: Any, libc: Any, graph: nil_gateGraph) -> None:
        self._refs = refs
        self._fns = fns
        self._gate = gate
        self._libc = libc
        self._graph = graph

    def port(self, eq: TypeEq, value: Optional[Any] = None) -> EPort:
        ptr = ctypes.c_void_p()
        if value is not None:
            allocated = self._libc.malloc(1)
            if not allocated:
                raise MemoryError("malloc failed")
            ptr = ctypes.c_void_p(allocated)
            self._refs[_to_ref_id(ptr)] = _PortState(value=value, eq=eq)

        port_info = nil_gatePortInfo(eq=self._fns.port_eq, destroy=self._fns.cleanup)
        handle = self._gate.nil_gate_graph_port(self._graph, port_info, ptr)
        return EPort(self._refs, self._gate, self._libc, handle, eq)

    def node(self, fn: NodeFn, inputs: List[Any], outputs: List[TypeEq]) -> Node:
        ctx_id = self._libc.malloc(1)
        if not ctx_id:
            raise MemoryError("malloc failed")

        input_rports = None
        if len(inputs) == 0:
            c_inputs = nil_gateRPorts(size=0, ports=None)
        else:
            input_rports = (nil_gateRPort * len(inputs))()
            for i, value in enumerate(inputs):
                input_rports[i] = value.as_input()._port
            c_inputs = nil_gateRPorts(size=len(inputs), ports=input_rports)

        output_infos = None
        if len(outputs) == 0:
            c_outputs = nil_gatePortInfos(size=0, infos=None)
        else:
            output_infos = (nil_gatePortInfo * len(outputs))()
            for i in range(len(outputs)):
                output_infos[i].eq = self._fns.port_eq
                output_infos[i].destroy = self._fns.cleanup
            c_outputs = nil_gatePortInfos(size=len(outputs), infos=output_infos)

        node_info = nil_gateNodeInfo(
            exec=self._fns.node_exec,
            inputs=c_inputs,
            outputs=c_outputs,
            context=ctypes.c_void_p(ctx_id),
            cleanup=self._fns.cleanup,
        )

        node = self._gate.nil_gate_graph_node(self._graph, node_info)
        self._refs[_to_ref_id(ctx_id)] = _NodeState(
            fn=fn,
            outputs=outputs
        )
        return Node(self._refs, self._gate, node)


class Core:
    __slots__ = ("_refs", "_fns", "_gate", "_libc", "_core")

    def __init__(self, refs: Dict[int, RefState], fns: _FnsState, gate: Any, libc: Any, core: nil_gateCore) -> None:
        self._refs = refs
        self._fns = fns
        self._gate = gate
        self._libc = libc
        self._core = core

    def destroy(self) -> None:
        self._refs.clear()
        self._gate.nil_gate_core_unset_runner(self._core)
        self._gate.nil_gate_core_destroy(self._core)

    def post(self, fn: PostFn) -> None:
        self._gate.nil_gate_core_post(self._core, self._fns.to_core_callable(fn))

    def apply(self, fn: PostFn) -> None:
        self._gate.nil_gate_core_apply(self._core, self._fns.to_core_callable(fn))

    def commit(self) -> None:
        self._gate.nil_gate_core_commit(self._core)


class Gate:
    __slots__ = ("_gate", "_refs", "_fns", "_libc")

    def __init__(self, gate: Any) -> None:
        self._gate = gate
        self._refs: Dict[int, RefState] = {}
        self._fns: _FnsState | None = None

        self._libc = ctypes.CDLL(None)
        self._libc.malloc.argtypes = [ctypes.c_size_t]
        self._libc.malloc.restype = ctypes.c_void_p
        self._libc.free.argtypes = [ctypes.c_void_p]
        self._libc.free.restype = None

        @NIL_GATE_CORE_CALLABLE_EXEC
        def post_exec(graph_ptr: Any, ptr: Any) -> None:
            graph = graph_ptr.contents
            node = self._refs[_to_ref_id(ptr)]
            node.fn(Graph(self._refs, self._fns, self._gate, self._libc, graph))

        @NIL_GATE_NODE_EXEC
        def node_exec(args_ptr: Any) -> None:
            args = args_ptr.contents

            in_values: List[Any] = []
            for i in range(int(args.inputs.size)):
                in_values.append(self._refs[_to_ref_id(args.inputs.data[i])].value)

            node = self._refs[_to_ref_id(args.context)]
            out_ports: List[MPort] = []
            for i in range(int(args.outputs.size)):
                out_ports.append(
                    MPort(self._refs, self._gate, self._libc, args.outputs.ports[i], node.outputs[i])
                )

            node.fn(
                NodeArgs(
                    core=Core(self._refs, self._fns, self._gate, self._libc, args.core),
                    inputs=in_values,
                    outputs=out_ports,
                )
            )

        @NIL_GATE_CLEANUP
        def cleanup(ptr: Any) -> None:
            if ptr:
                self._refs.pop(_to_ref_id(ptr), None)
                self._libc.free(ctypes.c_void_p(ptr))

        @NIL_GATE_EQ
        def port_eq(left: Any, right: Any) -> int:
            if not left and not right:
                return 1

            if not (left and right):
                return 0

            l_value = self._refs[_to_ref_id(left)]
            r_value = self._refs[_to_ref_id(right)]
            if l_value.eq is not r_value.eq:
                return 0

            return 1 if l_value.eq(l_value.value, r_value.value) else 0

        def to_core_callable(fn: PostFn) -> nil_gateCoreCallable:
            ctx_id = self._libc.malloc(1)
            if not ctx_id:
                raise MemoryError("malloc failed")

            callable_value = nil_gateCoreCallable(
                exec=self._fns.post_exec,
                context=ctypes.c_void_p(ctx_id),
                cleanup=self._fns.cleanup,
            )

            self._refs[_to_ref_id(ctx_id)] = _CallableState(fn=fn)
            return callable_value

        self._fns = _FnsState(
            post_exec=post_exec,
            node_exec=node_exec,
            port_eq=port_eq,
            cleanup=cleanup,
            to_core_callable=to_core_callable
        )

    def create_core(self) -> Core:
        core = self._gate.nil_gate_core_create()
        self._gate.nil_gate_core_set_runner_soft_blocking(core)
        return Core(self._refs, self._fns, self._gate, self._libc, core)


def _load_gate_from_module_dir() -> Gate:
    lib_path = Path(__file__).resolve().parent / "libgate-c-api.so"
    gate = ctypes.CDLL(str(lib_path))
    _configure_signatures(gate)
    return Gate(gate)


_GATE = _load_gate_from_module_dir()


def create_core() -> Core:
    return _GATE.create_core()


__all__ = ["create_core", "Gate", "Core", "Graph", "Node", "NodeArgs", "EPort", "MPort", "RPort"]
