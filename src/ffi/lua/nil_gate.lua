local ffi = require("ffi")

ffi.cdef [[
    void *malloc(size_t size);
    void free(void* ptr);
]]

ffi.cdef [[
    typedef struct nil_gate_core
    {
        void* handle;
    } nil_gate_core;

    nil_gate_core nil_gate_core_create(void);
    void nil_gate_core_destroy(nil_gate_core core);
    void nil_gate_core_commit(nil_gate_core core);

    void nil_gate_core_set_runner_immediate(nil_gate_core core);
    void nil_gate_core_set_runner_soft_blocking(nil_gate_core core);
    void nil_gate_core_set_runner_async(nil_gate_core core, uint32_t thread_count);
    void nil_gate_core_unset_runner(nil_gate_core core);

    typedef struct nil_gate_graph
    {
        void* handle;
    } nil_gate_graph;

    typedef struct nil_gate_core_callable
    {
        void (*exec)(nil_gate_graph*, void*);
        void* context;
        void (*cleanup)(void*);
    } nil_gate_core_callable;

    void nil_gate_core_post(nil_gate_core core, nil_gate_core_callable callable);
    void nil_gate_core_apply(nil_gate_core core, nil_gate_core_callable callable);

    typedef struct nil_gate_port_info
    {
        int (*eq)(const void*, const void*);
        void (*destroy)(void*);
    } nil_gate_port_info;

    typedef struct nil_gate_eport
    {
        void* handle;
        nil_gate_port_info info;
    } nil_gate_eport;

    nil_gate_eport nil_gate_graph_port(
        nil_gate_graph graph,
        nil_gate_port_info info,
        void* initial_value
    );

    typedef struct nil_gate_port_infos
    {
        uint8_t size;
        const nil_gate_port_info* infos;
    } nil_gate_port_infos;

    typedef struct nil_gate_rport
    {
        void* handle;
        nil_gate_port_info info;
    } nil_gate_rport;

    typedef struct nil_gate_rports
    {
        uint8_t size;
        const nil_gate_rport* ports;
    } nil_gate_rports;

    typedef struct nil_gate_mport
    {
        void* handle;
        nil_gate_port_info info;
    } nil_gate_mport;

    typedef struct nil_gate_mports
    {
        uint8_t size;
        const nil_gate_mport* ports;
    } nil_gate_mports;

    typedef struct nil_gate_node_args_inputs
    {
        uint8_t size;
        const void* const* data;
    } nil_gate_node_args_inputs;

    typedef struct nil_gate_node_args
    {
        nil_gate_core core;
        nil_gate_node_args_inputs inputs;
        nil_gate_mports outputs;
        void* context;
    } nil_gate_node_args;

    typedef struct nil_gate_node_info
    {
        void (*exec)(const nil_gate_node_args* args);
        nil_gate_rports inputs;
        nil_gate_port_infos outputs;
        void* context;
        void (*cleanup)(void*);
    } nil_gate_node_info;

    typedef struct nil_gate_node
    {
        void* handle;
    } nil_gate_node;

    nil_gate_node nil_gate_graph_node(
        nil_gate_graph graph,
        nil_gate_node_info node_info
    );

    uint8_t nil_gate_node_output_size(nil_gate_node node);
    void nil_gate_node_outputs(nil_gate_node node, nil_gate_rport* outputs);

    const void* nil_gate_rport_value(nil_gate_rport port);
    int nil_gate_rport_has_value(nil_gate_rport port);

    void nil_gate_mport_set_value(nil_gate_mport port, void* new_value);
    void nil_gate_mport_unset_value(nil_gate_mport port);

    nil_gate_rport nil_gate_mport_as_input(nil_gate_mport port);
    nil_gate_rport nil_gate_rport_as_input(nil_gate_rport port);
    nil_gate_rport nil_gate_eport_as_input(nil_gate_eport port);

    nil_gate_mport nil_gate_eport_to_direct(nil_gate_eport port);
]]

local function to_ref_id(id)
    return tonumber(ffi.cast("uintptr_t", id))
end

local function current_file_dir()
    local source = debug.getinfo(1, "S").source
    if source:sub(1, 1) == "@" then
        source = source:sub(2)
    end

    local dir = source:match("(.*/)")
    if dir == nil then
        return "./"
    end

    return dir
end

---@class nil_gate.RPort
---@field value fun(self: nil_gate.RPort): unknown
---@field has_value fun(self: nil_gate.RPort): boolean
----@field as_input fun(self: nil_gate.RPort): nil_gate.RPort

---@class nil_gate.MPort
---@field value fun(self: nil_gate.MPort): unknown
---@field has_value fun(self: nil_gate.MPort): boolean
---@field set_value fun(self: nil_gate.MPort, value: unknown)
---@field unset_value fun(self: nil_gate.MPort)
----@field as_input fun(self: nil_gate.MPort): nil_gate.RPort

---@class nil_gate.EPort
---@field to_direct fun(self: nil_gate.EPort): nil_gate.MPort
----@field as_input fun(self: nil_gate.EPort): nil_gate.RPort

---@alias nil_gate.TypeID fun(l, r): boolean

---@class nil_gate.Node
---@field outputs fun(self: nil_gate.Node): nil_gate.RPort[]

---@class nil_gate.NodeArgs
---@field core nil_gate.Core
---@field inputs unknown[]
---@field outputs nil_gate.MPort[]

---@class nil_gate.Graph
---@field port fun(self: nil_gate.Graph, eq: nil_gate.TypeID, value?: unknown): nil_gate.EPort
---@field node fun(self: nil_gate.Graph, fn: (fun(args: nil_gate.NodeArgs)), inputs: (nil_gate.RPort | nil_gate.MPort | nil_gate.EPort)[], outputs: nil_gate.TypeID[]): nil_gate.Node

---@class nil_gate.Core
---@field commit fun(self: nil_gate.Core)
---@field post fun(self: nil_gate.Core, fn: fun(graph: nil_gate.Graph))
---@field apply fun(self: nil_gate.Core, fn: fun(graph: nil_gate.Graph))
---@field destroy fun(self: nil_gate.Core)

---@class nil_gate.RPort
local function create_rport(refs, gate, rport)
    return {
        _port = rport,
        value = function(self)
            local rid = gate.nil_gate_rport_value(self._port)
            return refs[to_ref_id(rid)].value
        end,
        has_value = function(self)
            return gate.nil_gate_rport_has_value(self._port) ~= 0
        end,
        as_input = function(self)
            return self
        end
    }
end

---@return nil_gate.MPort
local function create_mport(refs, gate, mport, eq)
    return {
        _port = mport,
        value = function(self)
            local rport = gate.nil_gate_mport_as_input(self._port);
            local rid = gate.nil_gate_rport_value(rport)
            return refs[to_ref_id(rid)].value
        end,
        has_value = function(self)
            local rport = gate.nil_gate_mport_as_input(self._port);
            return gate.nil_gate_rport_has_value(rport) ~= 0
        end,
        set_value = function(self, new_value)
            local new_id = ffi.C.malloc(1)
            refs[to_ref_id(new_id)] = { value = new_value, eq = eq }
            gate.nil_gate_mport_set_value(self._port, new_id)
        end,
        unset_value = function(self)
            gate.nil_gate_mport_unset_value(self._port)
        end,
        as_input = function(self)
            return create_rport(refs, gate, gate.nil_gate_mport_as_input(self._port))
        end
    }
end

---@return nil_gate.EPort
local function create_lua_eport(refs, lua_fns, gate, graph, eq, value)
    local port_info = ffi.new("nil_gate_port_info")
    local id = nil

    if (value ~= nil) then
        id = ffi.C.malloc(1)
        refs[to_ref_id(id)] = { value = value, eq = eq }
    end

    port_info.eq = lua_fns.port_eq
    port_info.destroy = lua_fns.cleanup

    return {
        _port = gate.nil_gate_graph_port(graph, port_info, id),
        to_direct = function(self)
            local mport = gate.nil_gate_eport_to_direct(self._port)
            return create_mport(refs, gate, mport, eq)
        end,
        as_input = function(self)
            return create_rport(refs, gate, gate.nil_gate_eport_as_input(self._port))
        end
    }
end

---@return nil_gate.Node
local function create_lua_node(refs, lua_fns, gate, graph, fn, inputs, outputs)
    local id = ffi.C.malloc(1)
    local node_info = ffi.new("nil_gate_node_info")

    local input_rports = nil
    local port_infos = nil

    if (#inputs == 0) then
        node_info.inputs.size = 0
        node_info.inputs.ports = nil
    else
        input_rports = ffi.new("nil_gate_rport[?]", #inputs)
        for i, v in ipairs(inputs) do
            input_rports[i - 1] = v:as_input()._port
        end

        node_info.inputs.size = #inputs
        node_info.inputs.ports = input_rports
    end

    if (#outputs == 0) then
        node_info.outputs.size = 0
        node_info.outputs.infos = nil
    else
        port_infos = ffi.new("nil_gate_port_info[?]", #outputs)

        for i in ipairs(outputs) do
            port_infos[i - 1].eq = lua_fns.port_eq
            port_infos[i - 1].destroy = lua_fns.cleanup
        end

        node_info.outputs.size = #outputs
        node_info.outputs.infos = port_infos
    end

    node_info.exec = lua_fns.node_exec
    node_info.cleanup = lua_fns.cleanup
    node_info.context = id
    local node = gate.nil_gate_graph_node(graph, node_info)

    refs[to_ref_id(id)] = { fn = fn, outputs = outputs }

    return {
        _node = node,
        outputs = function(self)
            local output_size = gate.nil_gate_node_output_size(self._node)

            local output_rports = {}
            if output_size > 0 then
                local node_output_rports = ffi.new("nil_gate_rport[?]", output_size)
                gate.nil_gate_node_outputs(self._node, node_output_rports)
                for i = 1, output_size, 1 do
                    output_rports[i] = create_rport(refs, gate, node_output_rports[i - 1])
                end
            end

            return output_rports
        end
    }
end

---@return nil_gate.Graph
local function to_lua_graph(refs, lua_fns, gate, graph)
    return {
        _graph = graph,
        port = function(self, eq, value)
            return create_lua_eport(refs, lua_fns, gate, self._graph, eq, value)
        end,
        node = function(self, fn, inputs, outputs)
            return create_lua_node(refs, lua_fns, gate, graph, fn, inputs, outputs)
        end
    }
end

---@return nil_gate.Core
local function to_lua_core(refs, lua_fns, gate, core)
    return {
        _core = core,
        destroy = function(self)
            for k in pairs(refs) do
                refs[k] = nil
            end

            gate.nil_gate_core_unset_runner(self._core)
            gate.nil_gate_core_destroy(self._core)
        end,
        post = function(self, fn)
            gate.nil_gate_core_post(self._core, lua_fns.to_core_callable(fn))
        end,
        commit = function(self)
            gate.nil_gate_core_commit(self._core)
        end,
        apply = function(self, fn)
            gate.nil_gate_core_apply(self._core, lua_fns.to_core_callable(fn))
        end
    }
end

local function create_core(gate)
    local refs = {}
    local lua_fns = {}

    local post_exec = ffi.cast(
        "void (*)(nil_gate_graph*, void*)",
        function(graph, id)
            refs[to_ref_id(id)].fn(to_lua_graph(refs, lua_fns, gate, graph[0]))
        end
    )

    local node_exec = ffi.cast(
        "void (*)(const nil_gate_node_args* args)",
        function(args)
            local inputs = {}
            for i = 1, args.inputs.size, 1 do
                inputs[i] = refs[to_ref_id(args.inputs.data[i - 1])].value
            end

            local outputs = {}
            local node = refs[to_ref_id(args.context)]
            for i = 1, args.outputs.size do
                local mport = ffi.new("nil_gate_mport")
                mport.handle = args.outputs.ports[i - 1].handle
                mport.info = args.outputs.ports[i - 1].info
                outputs[i] = create_mport(refs, gate, mport, node.outputs[i])
            end
            refs[to_ref_id(args.context)]
                .fn({
                    core = to_lua_core(refs, lua_fns, gate, args.core),
                    inputs = inputs,
                    outputs = outputs
                })
        end
    )

    local cleanup = ffi.cast(
        "void (*)(void*)",
        function(id)
            if id ~= nil then
                refs[to_ref_id(id)] = nil
                ffi.C.free(id)
            end
        end
    )

    local port_eq = ffi.cast(
        "int (*)(void*, void*)",
        function(l, r)
            if l == nil and r == nil then
                return 1
            end

            if not (l ~= nil and r ~= nil) then
                return 0
            end

            local ll = refs[to_ref_id(l)]
            local rr = refs[to_ref_id(r)]
            if ll.eq ~= rr.eq then
                return 0
            end

            if ll.eq(ll.value, rr.value) then
                return 1
            end

            return 0
        end
    )

    lua_fns = {
        post_exec = post_exec,
        node_exec = node_exec,
        port_eq = port_eq,
        cleanup = cleanup,
        to_core_callable = function (fn)
            local id = ffi.C.malloc(1)

            local callable = ffi.new("nil_gate_core_callable")
            callable.exec = lua_fns.post_exec
            callable.cleanup = lua_fns.cleanup
            callable.context = id

            refs[to_ref_id(id)] = { fn = fn }
            return callable
        end
    }

    local core = gate.nil_gate_core_create()
    gate.nil_gate_core_set_runner_soft_blocking(core)
    return to_lua_core(
        refs,
        lua_fns,
        gate,
        core
    )
end

local gate = ffi.load(current_file_dir() .. "libgate-c-api.so")

return {
    create_core = function()
        return create_core(gate)
    end
}

-- missing
--  - port and node removal
