#pragma once

#include "../../traits/edgify.hpp"
#include "../../types.hpp"
#include "../DataEdge.hpp"
#include "../validation.hpp"
#include "callable.hpp"

#include <nil/xalt/checks.hpp>
#include <nil/xalt/tlist.hpp>

#include <type_traits>
#include <utility>

namespace nil::gate
{
    class Core;
}

namespace nil::gate::detail::traits
{
    template <typename T>
    concept is_core_valid_v //
        = (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>);

    template <typename T>
    concept is_async_valid_v //
        = std::is_same_v<T, std::decay_t<T>>
        || (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>);

    template <typename T>
    struct is_async_arg: std::false_type
    {
    };

    template <typename... T>
    struct is_async_arg<std::tuple<gate::edges::Mutable<T>*...>>: std::true_type
    {
    };

    template <typename T>
    using edgify_t = gate::traits::edgify_t<T>;

    template <typename T>
    struct typify;

    template <template <typename> typename B, typename T>
    struct typify<B<T>*> final
    {
        using type = T;
    };

    template <typename T>
    using typify_t = typename typify<T>::type;

    template <typename I>
    struct input_splitter;

    template <typename... I>
    struct input_splitter<xalt::tlist_types<I...>> final
    {
        using inputs = xalt::tlist_types<I...>;
        using asyncs = xalt::tlist_types<>;
        static constexpr bool has_core = false;
        static constexpr bool has_async = false;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_async_valid = true;
    };

    template <typename First, typename... I>
        requires(is_async_arg<std::decay_t<First>>::value)
    struct input_splitter<xalt::tlist_types<First, I...>> final
    {
        using inputs = xalt::tlist_types<I...>;
        using asyncs = xalt::to_tlist_types<std::decay_t<First>>::type::template apply<typify_t>;
        static constexpr bool has_core = false;
        static constexpr bool has_async = true;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_async_valid = is_async_valid_v<First>;
    };

    template <typename First, typename... I>
        requires(std::is_same_v<Core, std::decay_t<First>>)
    struct input_splitter<xalt::tlist_types<First, I...>> final
    {
        using inputs = xalt::tlist_types<I...>;
        using asyncs = xalt::tlist_types<>;
        static constexpr bool has_core = true;
        static constexpr bool has_async = false;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_async_valid = true;
    };

    template <typename First, typename Second, typename... I>
        requires std::is_same_v<Core, std::decay_t<First>>
        && (is_async_arg<std::decay_t<Second>>::value)
    struct input_splitter<xalt::tlist_types<First, Second, I...>> final
    {
        using inputs = xalt::tlist_types<I...>;
        using asyncs = xalt::to_tlist_types<std::decay_t<Second>>::type::template apply<typify_t>;
        static constexpr bool has_core = true;
        static constexpr bool has_async = true;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_async_valid = is_async_valid_v<Second>;
    };

    template <typename T>
    struct node_inputs;

    template <typename... I>
    struct node_inputs<xalt::tlist_types<I...>> final
    {
        using types = xalt::tlist_types<I...>;
        using edges = inputs<edgify_t<std::decay_t<I>>...>;
        using make_index_sequence = std::index_sequence_for<I...>;
        static constexpr auto size = sizeof...(I);
        static constexpr bool is_valid = (true && ... && (input_validate_v<I>));
    };

    template <typename T>
    struct node_sync_outputs;

    template <typename... S>
    struct node_sync_outputs<xalt::tlist_types<S...>> final
    {
        using types = xalt::tlist_types<S...>;
        using edges = sync_outputs<edgify_t<std::decay_t<S>>...>;
        using data_edges = std::tuple<detail::edges::Data<edgify_t<std::decay_t<S>>>...>;
        using make_index_sequence = std::index_sequence_for<S...>;
        static constexpr auto size = sizeof...(S);
        static constexpr bool is_valid = (true && ... && (sync_output_validate_v<S>));
    };

    template <typename T>
    struct node_async_outputs;

    template <typename... A>
    struct node_async_outputs<xalt::tlist_types<A...>> final
    {
        using types = xalt::tlist_types<A...>;
        using edges = async_outputs<edgify_t<std::decay_t<A>>...>;
        using data_edges = std::tuple<detail::edges::Data<edgify_t<std::decay_t<A>>>...>;
        using make_index_sequence = std::index_sequence_for<A...>;
        static constexpr auto size = sizeof...(A);
        static constexpr bool is_valid = (true && ... && async_output_validate_v<A>);
    };

    template <typename S, typename A>
    struct node_outputs;

    template <typename... S, typename... A>
    struct node_outputs<sync_outputs<S...>, async_outputs<A...>> final
    {
        using types = xalt::tlist_types<S..., A...>;
        using edges = outputs<sync_outputs<S...>, async_outputs<A...>>;
        using make_index_sequence = std::index_sequence_for<S..., A...>;
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
