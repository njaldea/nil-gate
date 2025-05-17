# nil/gate

- [Supported Graph](#supported-graph)
- [nil::gate::Core](#nilgatecore)
    - [Port](#port)
        - [ports::ReadOnly](#portsreadonly)
        - [ports::Mutable](#portsmutable)
        - [ports::Batch](#portsbatch)
        - [ports::Compatible](#portscompatible)
        - [nil::gate::Core::port](#nilgatecoreport)
    - [Node](#node)
        - [Input](#input)
        - [Output](#output)
            - [Sync Output](#sync-output)
            - [Async Output](#async-output)
        - [Special Arguments](#special-arguments)
        - [nil::gate::Core::node](#nilgatecorenode)
    - [Commit](#commit)
    - [Batch](#batch)
    - [Runners](#runners)
- [nil::gate::traits](#traits)
    - [compatibility](#compatibility)
    - [portify](#portify)
    - [bias](#bias)
    - [notes](#notes-personal-suggestions)
- [Errors](#errors)

## Supported Graph

**nil/gate** is a library to create a graph with nodes representing a functionality that needs to be executed as the graph is traversed.

Imagine logic gates where ports/connections are not 0/1 (booleans) but can be any customizable types.

**nil/gate** currently only supports **Directed Acyclic Graph**.
It is intended to be simple but easily extensible by creating the proper nodes to support more complex graph.

## `nil::gate::Core`

`nil::gate::Core` is the central object of the library.

It owns and holds all of the nodes and ports registered to it through its API.

## Port

Ports represents a Single Input - Multiple Output connection.

Ports are only created through the following:
- returned by `Core::port`
- output of the Node returned by `Core::node`
- returned by `Core::batch`

### `ports::ReadOnly`

This base class provides an interface to access the value held by the port.

NOTE: `ReadOnly<T>::value()` is only thread safe when accessed inside the node's call operator.

```cpp
#include <nil/gate.hpp>

class Node
{
    std::tuple<float> operator()() const;
};

int main()
{
    nil::gate::Core core;

    auto [ port ] = core.node();
    //     ┗━━━ nil::gate::ports::ReadOnly<float>*

    // will return the current value.
    // since the node is not yet ran, the port does not have a value yet.
    // accessing the value of an port before running the node is undefined behavior.
    port->value();
}
```

### `ports::Mutable`

This type of port is also an `ports::ReadOnly`.

NOTE: `set_value` will only take effect on next `Core::commit()`.

```cpp
#include <nil/gate.hpp>

int main()
{
    nil::gate::Core core;

    int initial_value = 100;
    auto port = core.port(initial_value);
    //   ┗━━━ nil::gate::ports::Mutable<int>*

    port->value(); // will return 100
    // will request to set the value to 200 on next Core::commit()
    port->set_value(200);
    port->value(); // will return 100

    core.commit();
    port->value(); // will return 200
}
```

### `ports::Batch`

This type of port is similar to `ports::Mutable` with the exact same API. These are created when batches are created.

See [Batch](#batch) for more detail.

### `ports::Compatible`

This type of port is used as a wrapper to `ports::ReadOnly` to be used as an input of a node.

This is intended to be created from `ports::ReadOnly` and is not intended for users to instantiate by themselves.

See [traits](#traits) for more detail how to create compatible ports

### `nil::gate::Core::port`

Here are the list of available `port()` signature available to `Core`:

```cpp
// value will be moved to the data inside the port
nil::gate::ports::Mutable<T>* Core::port(T value);

// this will instantiate an port without any value
// Note that if an port does not have a value, the node attached to it will not be invoked.
nil::gate::ports::Mutable<T>* Core::port();
```

Requirements of the type for the port:
- has `operator==`

## Node

A Node is just an object that is Callable (with `operator()`).
Its signature represents the Inputs and Outputs of the Node with some optional special arguments.

NOTE: currently only free function and `T::operator() const` is supported.

### Input

```cpp
#include <nil/gate.hpp>

void free_function_node(float);
//                      ┗━━━ all arguments are treated as an input

int main()
{
    nil::gate::Core core;
    auto port_f = core.port(200.f);
    //   ┗━━━ nil::gate::ports::Mutable<float>*

    core.node(&free_function_node, { port_f });
    //                             ┣━━━ nil::gate::inputs<float>
    //                             ┗━━━ std::tuple<nil::gate::ports::Compatible<float>, ...>;
}
```

### Output

Outputs are separateed into two. Sync and Async.

#### Sync Output

Sync outputs are those that are returned by the node. These are intended to be modified by the node itself by returning the value through the call operator, thus, the returned type during registration is a `nil::gate::ReadOnly<T>*`.

```cpp
#include <nil/gate.hpp>

struct Node
{
    std::tuple<float> operator()() const;
    //         ┗━━━ all tuple types are treated as the sync output
    // returning void is also allowed
    // if non-tuple, it will be treated as one return
};

int main()
{
    nil::gate::Core core;
    
    auto outputs = core.node(Node()).syncs; // Access syncs from the returned object
    //   ┣━━━ nil::gate::sync_outputs<float>
    //   ┗━━━ std::tuple<nil::gate::ports::ReadOnly<float>*, ...>;

    {
        auto port_f = get<0>(outputs);
        //   ┗━━━ nil::gate::ports::ReadOnly<float>*
    }
    {
        auto [ port_f ] = outputs;
        //     ┗━━━ nil::gate::ports::ReadOnly<float>*
    }
}
```

#### Async Output

Async outputs are those that are not returned by the node. These are intended to represent optional output when the node is ran.

```cpp
#include <nil/gate.hpp>

struct Node
{
    void operator()(nil::gate::async_output<float> asyncs) const;
    //              ┣━━━ this is equivalent to `std::tuple<nil::gate::ports::Mutable<T>*...>`
    //              ┗━━━ this is not treated as an input
};

int main()
{
    nil::gate::Core core;
    
    auto outputs = core.node(Node()).asyncs; // Access asyncs from the returned object
    //   ┣━━━ nil::gate::async_outputs<float>
    //   ┗━━━ std::tuple<nil::gate::ports::Mutable<float>*, ...>;

    {
        auto port_f = get<0>(outputs);
        //   ┗━━━ nil::gate::ports::Mutable<float>*
    }
    {
        auto [ port_f ] = outputs;
        //   ┗━━━ nil::gate::ports::Mutable<float>*
    }
}
```

#### Inline Structured Bindings For Mixed Outputs

When you have sync and async outputs at the same time, you can directly access them from the `syncs` and `asyncs` tuple returned by the node registration method.

In case you want to destructure them on the spot, you can do so but take note that the order would be:
- all sync outputs
- all async outputs

You can also use `get<I>` (similar to std::tuple's std::get) where index will follow sync outputs first, then asyncs.

```cpp
#include <nil/gate.hpp>

struct Node
{
    int operator()(nil::gate::async_output<float> asyncs) const;
    //             ┣━━━ this is equivalent to `std::tuple<nil::gate::ports::Mutable<T>*...>`
    //             ┗━━━ this is not treated as an input
};

int main()
{
    nil::gate::Core core;
    
    auto outputs = core.node(Node());
    //   ┣━━━ nil::gate::outputs<nil::gate::sync_outputs<int>, nil::gate::async_outputs<float>>

    {
        auto port_i = get<0>(outputs);
        //   ┗━━━ nil::gate::ports::ReadOnly<int>*
        auto port_f = get<1>(outputs);
        //   ┗━━━ nil::gate::ports::Mutable<float>*
    }
    {
        auto [ port_i, port_f ] = outputs;
        //     ┃       ┗━━━ nil::gate::ports::Mutable<float>*
        //     ┗━━━ nil::gate::ports::ReadOnly<int>*
    }
}
```

### Special Arguments

If the 1st argument is `const nil::gate::Core&`, the `Core` owner will be passed to it.

This can be useful when using async ports, batching, and committing.

See [Batch](#batch) section for more detail.

```cpp
#include <nil/gate.hpp>

struct Node
{
    void operator()(const nil::gate::Core& core, nil::gate::async_output<float> asyncs) const
    //              ┃                            ┣━━━ this is equivalent to `std::tuple<ports::Mutable<T>*...>`
    //              ┃                            ┗━━━ this is not treated as an input
    //              ┗━━━ this is not treated as an input
    {
        auto [ port_f ] = asyncs;
        //     ┗━━━ nil::gate::ports::Mutable<float>*
    }
};

int main()
{
    nil::gate::Core core;

    auto [ port_f ] = core.node(Node());
    //     ┗━━━ nil::gate::ports::Mutable<float>*
}
```

### `nil::gate::Core::node`

Here are the list of available `node()` signature available to `Core`

```cpp
//  Legend:
//   -  I -- Input
//   -  S -- Sync Output
//   -  A -- Async Output
//
// NOTES:
//  1. `T instance` will be moved (or copied) inside the node
//  3. `nil::gate::inputs<T...>` is simply an alias to `std::tuple<nil::gate::ports::Compatible<T>...>`

// has input, no output
void Core::node<T>(T instance, nil::gate::inputs<I...>);

// has input, has output
nil::gate::outputs<
    nil::gate::sync_outputs<S...>,
    nil::gate::async_outputs<A...>
>
    Core::node<T>(T instance, nil::gate::inputs<I...>);
```

## Commit

`Core::commit` applies all of the changes you want to do for the ports.

See [Runners](#runners) section for more detail on when and where this changes are done.

```cpp
#include <nil/gate.hpp>
#include <functional>

struct Node
{
    void operator()(int, float) const;
};

int main()
{
    nil::gate::Core core;

    auto ei = core.port(100);
    //   ┗━━━ nil::gate::ports::Mutable<int>*
    auto ef = core.port(200.0);
    //   ┗━━━ nil::gate::ports::Mutable<float>*

    core.node(Node(), { ei, ef });

    ei->set_value(101);
    ef->set_value(201.0);
    core.commit();
}
```

## Batch

Use `Core::batch` for cases that you need guarantee that changes are done in one go.

```cpp
#include <nil/gate.hpp>
#include <functional>

struct Node
{
    void operator()(int, float) const;
};

int main()
{
    nil::gate::Core core;

    auto ei = core.port(100);
    //   ┗━━━ nil::gate::ports::Mutable<int>*
    auto ef = core.port(200.0);
    //   ┗━━━ nil::gate::ports::Mutable<float>*

    core.node(Node(), { ei, ef });

    {
        auto [ bei, bef ] = core.batch(ei, ef);
        // or
        // auto [ bei, bef ] = core.batch({ei, ef});
        bei->set_value(102);
        bef->set_value(202.0);
    } // make sure that a batch is destroyed to queue the changes
    core.commit();
}
```

## Runners

The behavior how the nodes are executed during `Core::commit()` is defined by a `runner`.

By default, these are the behavior of the library:
- nodes are executed inside the same thread where `Core::commit()` is invoked.
- nodes are executed in order of registration.

Use `Core::set_runner` to override this behavior.

The library provides a runner that can run the nodes in a different thread.

```cpp
// This is the default runner used
#include <nil/gate/runners/Immediate.hpp>
// This will run the graph in a separate thread
#include <nil/gate/runners/NonBlockking.hpp>

// The following runners requires boost dependency

// This is similar to NonBlocking but using boost asio
#include <nil/gate/runners/boost_asio/Serial.hpp>
// will run everyting in a dedicated thread + each node will be ran in a thread pool
#include <nil/gate/runners/boost_asio/Parallel.hpp>

...

nil::gate::Core core;
core.set_runner<nil::gate::runners::Immediate>();
core.set_runner<nil::gate::runners::NonBlocking>();
core.set_runner<nil::gate::runners::boost_asio::Serial>();
core.set_runner<nil::gate::runners::boost_asio::Parallel>(thread_count);
```

Runners are expected to implement the following (WIP: to be revised):
- `run(Core* core, std::function<void()> apply_changes, std::span<INode* const>)`

Make sure that flushing and running are done in a thread safe manner

## `nil::gate::traits`

`nil/gate` provides an overridable traits to allow customization and add rules to graph creation.

### portify

This trait dictates how the type `T` is going to be interpreted for the port creation.

The default trait behavior is that `T` provided to portify will be the same type for the port type.

This also affects the type evaluation for the sync outputs of a node.

Here is an example of the default behavior:

```cpp
#include <nil/gate.hpp>

int main()
{
    nil::gate::Core core;

    auto port_i = core.port(100);
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<int>*

    auto port_p1 = core.port(std::unique_ptr<int>());
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<std::unique_ptr<int>>*
    
    auto port_p2 = core.port(std::unique_ptr<const int>());
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<std::unique_ptr<const int>>*
}
```

Here is an example of how to override the behavior:

```cpp
#include <nil/gate.hpp>

namespace nil::gate::traits
{
    template <typename T>
    struct portify<std::unique_ptr<T>>
    {
        using type = std::unique_ptr<const T>;
    };
}

int main()
{
    nil::gate::Core core;

    auto port_i = core.port(100);
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<int>*

    auto port_p1 = core.port(std::unique_ptr<int>());
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<std::unique_ptr<const int>>*

    auto port_p2 = core.port(std::unique_ptr<const int>());
    //   ┗━━━━━━━━ nil::gate::ports::Mutable<std::unique_ptr<const int>>*
}
```

When building a graph, it is ideal that the data owned by the ports is only be modified by the library (not the nodes).
Allowing the node to modify the object referred through indirection will make the graph execution non-deterministic.

This feature is intended for users to opt-in as the library will not be able to cover all of the types that has indirection.

See [biased](#bias) section for more information of `nil/gate`'s suggested rules.

### compatibility

This trait dictates if an port could be used even if the type of the port does not match with the expected input port of the node.

Here is an example of when this is going to be used:

```cpp
#include <nil/gate.hpp>

std::reference_wrapper<const int> switcher(bool flag, int a, int b);

void consumer(int);

int main()
{
    nil::gate::Core core;

    bool ref = false;
    auto* flag = core.port(std::cref(false));
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<std::reference_wrapper<const bool>>*
    auto* a = core.port(1);
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<int>*
    auto* b = core.port(2);
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<int>*
    const auto [ out ] = core.node(&switcher, {flag, a, b});
    //           ┃                             ┗━━━━━━━━ This will produce a compilation failure.
    //           ┃                                       ReadOnly<std::reference_wrapper<const bool>>
    //           ┃                                       is not convertible to ReadOnly<bool>
    //           ┗━━━━━━━━ nil::gate::ports::ReadOnly<std::reference_wrapper<const int>>*

    core.node(&consumer, { out });
    //                     ┗━━━━━━━━ This will produce a compilation failure.
    //                               ReadOnly<std::reference_wrapper<const int>>
    //                               is not compatible to ReadOnly<int>
}
```

to allow compatibility, users must define how to convert the data by doing the following:

```cpp

#include <nil/gate.hpp>

#include <functional>

namespace nil::gate::traits
{
    template <typename To>
    struct compatibility<
        T
    //  ┣━━━━━━━━ first template type is the type to convert to
    //  ┗━━━━━━━━ this is normally the type expected by the node
        std::reference_wrapper<const T>,
    //  ┗━━━━━━━━ second template type is the type to convert from
    >
    {
        static const T& convert(const std::reference_wrapper<const T>& u)
        {
            return u.get();
        }
    };
    template <typename To>
    struct compatibility<
        std::reference_wrapper<const T>,
    //  ┣━━━━━━━━ first template type is the type to convert to
    //  ┗━━━━━━━━ this is normally the type expected by the node
        T
    //  ┗━━━━━━━━ second template type is the type to convert from
    >
    {
        static std::reference_wrapper<const T> convert(const T& u)
        {
            return u;
        }
    };
}

std::reference_wrapper<const int> switcher(bool flag, int a, int b);

void consumer(int);

int main()
{
    nil::gate::Core core;

    auto* flag = core.port(std::cref(false));
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<std::reference_wrapper<const bool>>*
    auto* a = core.port(1);
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<int>*
    auto* b = core.port(2);
    //    ┗━━━━━━━━ nil::gate::ports::Mutable<int>*
    const auto [ ref ] = core.node(&switcher, {flag, a, b});
    //           ┗━━━━━━━━ nil::gate::ports::ReadOnly<std::reference_wrapper<const int>>*

    core.node(&consumer, { ref });
    //                     ┗━━━━━━━━ since `compatibility` is defined, this should be accepted by the compiler
}
```

### is_port_type_valid

This trait is intended for cases where you want to detect and prevent creation of ports that should not be ports.

By default, all of the port types are valid except for the following:
- T is a pointer type

```cpp
#include <nil/gate.hpp>

namespace nil::gate::traits
{
    template <typename T>
    struct is_port_type_valid<std::reference_wrapper<T>>: std::false_type
    {
    };

    template <typename T>
    struct is_port_type_valid<std::reference_wrapper<const T>>: std::true_type
    {
    };
}
```

With reference to `portify`, if a type is converted from one type to another, it is a good idea to disable the original type.

See [biased](#bias) section for more information of `nil/gate`'s suggested rules.

### bias

`nil/gate` provides a very opinionated setup from these header:
 - [`#include <nil/gate/bias/compatibility.hpp>`](publish/nil/gate/bias/compatibility.hpp)
     - covers conversion between `T` and `std::reference_wrapper<const T>`
     - covers conversion of `std::unique_ptr<const T>` to `const T*`
     - covers conversion of `std::shared_ptr<const T>` to `const T*`
     - covers conversion of `std::optional<const T>` to `const T*`
 - [`#include <nil/gate/bias/portify.hpp>`](publish/nil/gate/bias/portify.hpp)
     - covers the following types:
         - `std::unique_ptr<T>` to `std::unique_ptr<const T>`
         - `std::shared_ptr<T>` to `std::shared_ptr<const T>`
         - `std::optional<T>` to `std::optional<const T>`
         - `std::reference_wrapper<T>` to `std::reference_wrapper<const T>`
 - [`#include <nil/gate/bias/is_port_type_valid.hpp>`](publish/nil/gate/bias/is_port_type_valid.hpp)
     - disables the original types converted by `bias/portify.hpp`
         - `std::unique_ptr<T>`
         - `std::shared_ptr<T>`
         - `std::optional<T>`
         - `std::reference_wrapper<T>`
 - [`#include <nil/gate/bias/nil.hpp>`](publish/nil/gate/bias/nil.hpp)
     - this is a helper header to conform with the author's suggested setup
     - applies all of the author's biases

These are not included from `nil/gate.hpp` and users must opt-in to apply these rules to their graphs.

### NOTES: personal suggestions

- When implementing your own `portify<T>` traits, avoid converting `T` to something different that is not related to `T`.
    - This will produce weird behavior when defining nodes.
    - `portify<T>` is mainly intended for things with indirections like pointer-like objects.

## Errors

The library tries its best to provide undestandable error messages as much as possible.

The example below is a result of having and invalid `Core` signature. `Core` should always be `const Core&`.

```cpp
float deferred(const nil::gate::Core core, nil::gate::async_outputs<int> z, bool a);
//             ┗━━━━━━━━ this should always be `const nil::gate::Core&`
```

### gcc

```bash
./nil/gate/Core.hpp:50:26: error: use of deleted function ‘nil::gate::errors::Error::Error(nil::gate::errors::Check<false>)’
   50 |             Error core = Check<traits::arg_core::is_valid>();
```

### clang

```bash
./nil/gate/Core.hpp:50:26: fatal error: conversion function from 'Check<traits::arg_core::is_valid>' to 'Error' invokes a deleted function
   50 |             Error core = Check<traits::arg_core::is_valid>();
```

These are the errors detected with similar error message:
 -  any input type is invalid
 -  any sync output type is invalid
 -  any async output type is invalid
 -  async_outputs is invalid
 -  core argument is invalid
 -  input `ports::Readable<T>*` is not compatible to the expected input port of the node
