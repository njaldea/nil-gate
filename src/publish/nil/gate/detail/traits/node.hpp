#pragma once

#include "../../traits/edgify.hpp"
#include "../../types.hpp"
#include "../DataEdge.hpp"
#include "../validation.hpp"
#include "callable.hpp"

#include <type_traits>

namespace nil::gate
{
    class Core;
}

namespace nil::gate::detail::traits
{
    template <typename T>
    static constexpr bool is_core_valid_v //
        = (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>);

    template <typename T>
    static constexpr bool is_async_valid_v //
        = std::is_same_v<T, std::decay_t<T>>
        || (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>);

    template <typename T>
    using edgify_t = gate::traits::edgify_t<T>;

    template <typename A>
    struct async_checker
    {
        static constexpr bool is_async = false;
    };

    template <typename... A>
    struct async_checker<nil::gate::async_outputs<A...>>
    {
        static constexpr bool is_async = true;
    };

    template <typename I>
    struct input_splitter;

    template <typename... I>
    struct input_splitter<types<I...>>
    {
        using inputs = types<I...>;
        using asyncs = types<>;
        static constexpr bool has_core = false;
        static constexpr bool has_async = false;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_async_valid = true;
    };

    template <typename First, typename... I>
        requires async_checker<std::decay_t<First>>::is_async
    struct input_splitter<types<First, I...>>
    {
        using inputs = types<I...>;
        using asyncs = typify_t<std::decay_t<First>>;
        static constexpr bool has_core = false;
        static constexpr bool has_async = true;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_async_valid = is_async_valid_v<First>;
    };

    template <typename First, typename... I>
        requires std::is_same_v<nil::gate::Core, std::decay_t<First>>
    struct input_splitter<types<First, I...>>
    {
        using inputs = types<I...>;
        using asyncs = types<>;
        static constexpr bool has_core = true;
        static constexpr bool has_async = false;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_async_valid = true;
    };

    template <typename First, typename Second, typename... I>
        requires std::is_same_v<nil::gate::Core, std::decay_t<First>>
        && async_checker<std::decay_t<Second>>::is_async
    struct input_splitter<types<First, Second, I...>>
    {
        using inputs = types<I...>;
        using asyncs = typify_t<std::decay_t<Second>>;
        static constexpr bool has_core = true;
        static constexpr bool has_async = true;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_async_valid = is_async_valid_v<Second>;
    };

    template <typename T>
    struct node_inputs;

    template <typename... I>
    struct node_inputs<types<I...>>
    {
        using edges = nil::gate::inputs<edgify_t<std::decay_t<I>>...>;
        using make_index_sequence = std::make_index_sequence<sizeof...(I)>;
        static constexpr auto size = sizeof...(I);
        static constexpr bool is_valid = (true && ... && (input_validate_v<I>));
    };

    template <typename T>
    struct node_sync_outputs;

    template <typename... S>
    struct node_sync_outputs<types<S...>>
    {
        using edges = std::tuple<nil::gate::edges::ReadOnly<edgify_t<std::decay_t<S>>>*...>;
        using data_edges = std::tuple<detail::edges::Data<edgify_t<std::decay_t<S>>>...>;
        using make_index_sequence = std::make_index_sequence<sizeof...(S)>;
        static constexpr auto size = sizeof...(S);
        static constexpr bool is_valid = (true && ... && (sync_output_validate_v<S>));
    };

    template <typename T>
    struct node_async_outputs;

    template <typename... A>
    struct node_async_outputs<types<A...>>
    {
        using edges = nil::gate::async_outputs<edgify_t<std::decay_t<A>>...>;
        using data_edges = std::tuple<detail::edges::Data<edgify_t<std::decay_t<A>>>...>;
        using make_index_sequence = std::make_index_sequence<sizeof...(A)>;
        static constexpr auto size = sizeof...(A);
        static constexpr bool is_valid = (true && ... && async_output_validate_v<A>);
    };

    template <typename S, typename A>
    struct node_outputs;

    template <typename... S, typename... A>
    struct node_outputs<nil::gate::sync_outputs<S...>, nil::gate::async_outputs<A...>>
    {
        using edges
            = nil::gate::outputs<nil::gate::sync_outputs<S...>, nil::gate::async_outputs<A...>>;
        using make_index_sequence = std::make_index_sequence<sizeof...(S) + sizeof...(A)>;
        static constexpr auto size = sizeof...(S) + sizeof...(A);
    };

    template <typename T>
    struct node final
    {
    private:
        using full_i = typename callable<T>::inputs;
        using split_i = input_splitter<full_i>;

        using final_s = typename callable<T>::outputs;
        using final_a = typename split_i::asyncs;
        using final_i = typename split_i::inputs;

    public:
        using inputs = node_inputs<final_i>;
        using sync_outputs = node_sync_outputs<final_s>;
        using async_outputs = node_async_outputs<final_a>;
        using outputs = node_outputs<typename sync_outputs::edges, typename async_outputs::edges>;

        struct arg_async
        {
            static constexpr bool is_valid = split_i::is_async_valid;
        };

        struct arg_core
        {
            static constexpr bool is_valid = split_i::is_core_valid;
        };

        static constexpr bool has_async = split_i::has_async;
        static constexpr bool has_core = split_i::has_core;
        static constexpr bool is_valid  //
            = arg_async::is_valid       //
            && arg_core::is_valid       //
            && inputs::is_valid         //
            && sync_outputs::is_valid   //
            && async_outputs::is_valid; //
    };
}
