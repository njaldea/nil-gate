package.path = package.path .. ";../src/ffi/lua/?.lua"

local nil_gate = require("nil_gate")


local function call_gc(u)
    print("Memory before :", u, collectgarbage("count"))
    collectgarbage("collect")
    print("Memory after  :", u, collectgarbage("count"))
end

local core = nil_gate.create_core()

local options = {
    check_mem = false,
    check_post = true
}

if options.check_mem then
    local function exec(graph, ctx)
        print("exec callback called")
    end
    for u = 1, 200, 1 do
        for i = 1, 200, 1 do
            core:post(exec)
            core:post(exec)
            core:commit()
        end
    end
    
    call_gc(0)
end

if options.check_post then
    local function data_eq(l, r)
        return l == r
    end

    ---@type EPort
    local port = nil

    ---@type RPort
    local node_port = nil

    core:post(function(graph)
        print("- exec callback called")
        port = graph:port(data_eq, 100)
        node_port = graph:node(
            function(arg)
                print("node 1 called", arg.core, #arg.inputs, #arg.outputs)
                arg.outputs[1]:set_value(arg.inputs[1])
            end,
            {port},
            {data_eq} -- output count
        ):outputs()[1]
        graph:node(
            function(arg)
                print("node 2 called", arg.core, arg.inputs[1], #arg.outputs)
            end,
            {node_port},
            {} -- output count
        )
        for u = 1, 200, 1 do
            print("- to mport")
            local mport = port:to_direct()
            print("- has_value")
            print(mport:has_value())
            print("- set_value")
            mport:set_value(100)
            print("- has_value")
            print(mport:has_value())
            print("- value")
            print(mport:value())
            print("- done")
        end
    end)
    core:commit()

    core:post(function(graph)
        print("- exec callback called")
        port:to_direct():set_value(200)
    end)
    core:commit()
    call_gc(0)
end

call_gc(0)
core:destroy()
call_gc(0)