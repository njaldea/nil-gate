#pragma once

#include "../../traits/portify.hpp"
#include "../../types.hpp"
#include "../Port.hpp"
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
    concept is_opt_valid_v //
        = std::is_same_v<T, std::decay_t<T>>
        || (std::is_reference_v<T> && std::is_const_v<std::remove_reference_t<T>>);

    template <typename T>
    struct is_opt_arg: std::false_type
    {
    };

    template <typename... T>
    struct is_opt_arg<std::tuple<gate::ports::Mutable<T>*...>>: std::true_type
    {
    };

    template <typename T>
    using portify_t = gate::traits::portify_t<T>;

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
    struct input_splitter<xalt::tlist<I...>> final
    {
        using inputs = xalt::tlist<I...>;
        using opts = xalt::tlist<>;
        static constexpr bool has_core = false;
        static constexpr bool has_opt = false;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_opt_valid = true;
    };

    template <typename First, typename... I>
        requires(is_opt_arg<std::decay_t<First>>::value)
    struct input_splitter<xalt::tlist<First, I...>> final
    {
        using inputs = xalt::tlist<I...>;
        using opts = xalt::to_tlist<std::decay_t<First>>::type::template apply<typify_t>;
        static constexpr bool has_core = false;
        static constexpr bool has_opt = true;
        static constexpr bool is_core_valid = true;
        static constexpr bool is_opt_valid = is_opt_valid_v<First>;
    };

    template <typename First, typename... I>
        requires(std::is_same_v<Core, std::decay_t<First>>)
    struct input_splitter<xalt::tlist<First, I...>> final
    {
        using inputs = xalt::tlist<I...>;
        using opts = xalt::tlist<>;
        static constexpr bool has_core = true;
        static constexpr bool has_opt = false;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_opt_valid = true;
    };

    template <typename First, typename Second, typename... I>
        requires std::is_same_v<Core, std::decay_t<First>>
        && (is_opt_arg<std::decay_t<Second>>::value)
    struct input_splitter<xalt::tlist<First, Second, I...>> final
    {
        using inputs = xalt::tlist<I...>;
        using opts = xalt::to_tlist<std::decay_t<Second>>::type::template apply<typify_t>;
        static constexpr bool has_core = true;
        static constexpr bool has_opt = true;
        static constexpr bool is_core_valid = is_core_valid_v<First>;
        static constexpr bool is_opt_valid = is_opt_valid_v<Second>;
    };

    template <typename T>
    struct node_inputs;

    template <typename... I>
    struct node_inputs<xalt::tlist<I...>> final
    {
        using types = xalt::tlist<I...>;
        using ports = inputs<portify_t<std::decay_t<I>>...>;
        using make_index_sequence = std::index_sequence_for<I...>;
        static constexpr auto size = sizeof...(I);
        static constexpr bool is_valid = (true && ... && (input_validate_v<I>));
    };

    template <typename T>
    struct node_req_outputs;

    template <typename... F>
    struct node_req_outputs<xalt::tlist<F...>> final
    {
        using types = xalt::tlist<F...>;
        using ports = req_outputs<portify_t<std::decay_t<F>>...>;
        using data_ports = std::tuple<detail::Port<portify_t<std::decay_t<F>>>...>;
        using make_index_sequence = std::index_sequence_for<F...>;
        static constexpr auto size = sizeof...(F);
        static constexpr bool is_valid = (true && ... && (req_output_validate_v<F>));
    };

    template <typename T>
    struct node_opt_outputs;

    template <typename... O>
    struct node_opt_outputs<xalt::tlist<O...>> final
    {
        using types = xalt::tlist<O...>;
        using ports = opt_outputs<portify_t<std::decay_t<O>>...>;
        using data_ports = std::tuple<detail::Port<portify_t<std::decay_t<O>>>...>;
        using make_index_sequence = std::index_sequence_for<O...>;
        static constexpr auto size = sizeof...(O);
        static constexpr bool is_valid = (true && ... && opt_output_validate_v<O>);
    };

    template <typename F, typename O>
    struct node_outputs;

    template <typename... F, typename... O>
    struct node_outputs<req_outputs<F...>, opt_outputs<O...>> final
    {
        using types = xalt::tlist<F..., O...>;
        using ports = outputs<F..., O...>;
        using make_index_sequence = std::index_sequence_for<F..., O...>;
        static constexpr auto size = sizeof...(F) + sizeof...(O);
    };

    template <typename T>
    struct node final
    {
    private:
        using full_i = typename callable<T>::inputs;
        using split_i = input_splitter<full_i>;

        using final_s = typename callable<T>::outputs;
        using final_a = typename split_i::opts;
        using final_i = typename split_i::inputs;

    public:
        using inputs = node_inputs<final_i>;
        using req_outputs = node_req_outputs<final_s>;
        using opt_outputs = node_opt_outputs<final_a>;
        using outputs = node_outputs<typename req_outputs::ports, typename opt_outputs::ports>;

        struct arg_opt
        {
            static constexpr bool is_valid = split_i::is_opt_valid;
        };

        struct arg_core
        {
            static constexpr bool is_valid = split_i::is_core_valid;
        };

        static constexpr bool has_opt = split_i::has_opt;
        static constexpr bool has_core = split_i::has_core;
        static constexpr bool is_valid //
            = arg_opt::is_valid        //
            && arg_core::is_valid      //
            && inputs::is_valid        //
            && req_outputs::is_valid   //
            && opt_outputs::is_valid;  //
    };
}
