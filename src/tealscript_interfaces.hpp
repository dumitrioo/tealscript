#pragma once

#include "inc/commondefs.hpp"
#include "inc/emhash/hash_set8.hpp"
#include "inc/net/net_utils.hpp"

#include "tealscript_util.hpp"
#include "tealscript_value.hpp"

namespace teal {

    struct obj_services {
        std::function<std::optional<std::string>(valbox const &)> serializer{nullptr};
        std::function<valbox(std::string const &, std::string const &)> deserializer{nullptr};
        std::function<valbox(valbox const &, valbox const &)> comparator{nullptr};
        std::function<valbox(valbox const &)> stringify{nullptr};
        str_map_t<std::function<valbox(valbox &, valbox &)>> binops{};
        str_map_t<std::function<valbox(valbox &)>> unops{};
    };

    class runtime_interface {
    public:
        virtual ~runtime_interface() = default;
        virtual str_map_t<valbox> const *global_constants_dictionary() const = 0;
        virtual str_map_t<valbox> const *global_functions_dictionary() const = 0;
        virtual str_map_t<str_map_t<valbox>> const *global_methods_dictionary() const = 0;
        virtual std::function<bool(std::string)> const &user_functions_search() const = 0;
        virtual std::function<valbox(std::vector<valbox> &)> const &user_function_selector() const = 0;
        virtual valbox get_input(std::string const &) = 0;
        virtual valbox const &get_output(std::string const &) = 0;
        virtual void set_input(std::string const &, valbox const &) = 0;
        virtual void set_output(std::string const &, valbox const &) = 0;
        virtual void clear_input(std::string const &) = 0;
        virtual void clear_output(std::string const &) = 0;
        virtual void clear_inputs() = 0;
        virtual void clear_outputs() = 0;


        virtual void add_object_serializer(
            std::string const &,
            std::function<std::optional<std::string>(valbox const &)> const &
        ) = 0;
        virtual void add_object_deserializer(
            std::string const &,
            std::function<valbox(std::string const &, std::string const &)> const &
        ) = 0;
        virtual void add_object_comparator(
            std::string const &,
            std::function<valbox(valbox const &, valbox const &)> const &
        ) = 0;
        virtual void add_object_stringifier(
            std::string const &,
            std::function<valbox(valbox const &)> const &
        ) = 0;
        virtual void add_object_unary_operation(
            std::string const &, std::string const &,
            std::function<valbox(valbox &)> const &
        ) = 0;
        virtual void add_object_binary_operation(
            std::string const &, std::string const &,
            std::function<valbox(valbox &, valbox &)> const &
        ) = 0;
        virtual void remove_object_services(std::string const &) = 0;
        virtual obj_services const *get_object_services(std::string const &) const = 0;

        virtual void add_function(
            std::string const &,
            std::function<valbox(std::vector<valbox> &)>
        ) = 0;
        virtual void remove_function(std::string const &) = 0;
        virtual void add_var(std::string const &, valbox const &) = 0;
        virtual void remove_var(std::string const &) = 0;
        virtual void add_method(
            std::string const &/*class_name*/,
            std::string const &/*method_name*/,
            std::function<valbox(std::vector<valbox> &)>
        ) = 0;
        virtual void remove_method(
            std::string const &/*class_name*/,
            std::string const &/*method_name*/
        ) = 0;

        // expose values to network directly
        virtual void start_net_server(
            net::address_family /*af*/,
            std::string const &/*bind_addr*/,
            std::uint16_t /*port*/,
            long double /*stale_connections_removal_timeout*/
        ) = 0;
        virtual void stop_net_server() = 0;
        virtual bool net_server_running() const = 0;
        virtual void set_external_cells_update_interval(long double /*seconds*/) = 0;
        virtual long double external_cells_update_interval() const = 0;

#ifdef TEAL_USE_EXTERNAL_VALUES
        // connect to a common network for exchanging values (alternative to the above)
        virtual void net_hub_connect(
            std::string const &/*host_addr*/,
            std::uint16_t /*port*/, std::string const &/*unique_net_name*/
        ) = 0;
#endif
    };

    class extension_interface {
    public:
        virtual ~extension_interface() {}
        virtual void register_runtime(runtime_interface *rt) = 0;
        virtual void unregister_runtime() = 0;
    };

}

extern "C" teal::extension_interface *create_teal_extension();
extern "C" void remove_teal_extension(teal::extension_interface *);
