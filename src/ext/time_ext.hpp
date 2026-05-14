#pragma once

#include "../inc/commondefs.hpp"

#include "../tealscript_value.hpp"
#include "../tealscript_util.hpp"
#include "../tealscript_interfaces.hpp"

namespace teal {

    class time_ext: public extension_interface {
    public:
        time_ext() = default;
        ~time_ext() {
            unregister_runtime();
        }
        time_ext(time_ext const &) = delete;
        time_ext &operator=(time_ext const &) = delete;
        time_ext(time_ext &&) = delete;
        time_ext &operator=(time_ext &&) = delete;

        void register_runtime(runtime_interface *rt) override {
            std::unique_lock l{rt_mtp_};
            if(rt_ != nullptr) { return; }
            rt_ = rt;
            if(rt_ == nullptr) { return; }

            rt->add_function("timestamp", TEALFUN(args) {
                std::string s{};
                if(args.size() > 0) {
                    if(args[0].is_string_ref()) {
                        return teal::valbox{timespec_wrapper{args[0].as_string()}, "timestamp"};
                    } else if(args[0].is_wstring_ref()) {
                        return teal::valbox{timespec_wrapper{args[0].as_wstring()}, "timestamp"};
                    } else if(args[0].is_any_fp_number()) {
                        return teal::valbox{timespec_wrapper{args[0].cast_to_long_double()}, "timestamp"};
                    } else if(args[0].is_any_int_number()) {
                        return teal::valbox{timespec_wrapper{args[0].cast_to_s64()}, "timestamp"};
                    }
                }
                return teal::valbox{timespec_wrapper::now(), "timestamp"};
            });

            rt->add_object_serializer("timestamp",
                [](valbox const &vb) -> std::optional<std::string> {
                    if(vb.is_class_ref() && vb.class_name() == "timestamp") {
                        return vb.as_class<timespec_wrapper>().as_iso_8601_str();
                    }
                    return {};
                }
            );

            rt->add_object_deserializer("timestamp",
                [](std::string const &class_name, std::string const &serial_form) -> valbox {
                    if(class_name == "timestamp") {
                        return teal::valbox{timespec_wrapper{serial_form}, "timestamp"};
                    }
                    return teal::valbox{valbox_no_initialize::dont_do_it};
                }
            );

            rt->add_object_stringifier("timestamp",
                [](valbox const &v) -> valbox {
                    if(v.class_name() == "timestamp") {
                        return v.as_class<timespec_wrapper>().as_iso_8601_str();
                    }
                    return std::string{};
                }
            );

            rt->add_object_binary_operation("timestamp", "==",
                [](valbox &l, valbox &r) -> valbox {
                    static const auto mb_constr{[](valbox const &vb) -> std::optional<timespec_wrapper> {
                        if(vb.is_any_int_number()) { return timespec_wrapper{vb.cast_to_s64()};
                        } else if(vb.is_any_fp_number()) { return timespec_wrapper{vb.cast_to_long_double()};
                        } else if(vb.is_string_ref()) { return timespec_wrapper{vb.as_string()};
                        } else if(vb.is_wstring_ref()) { return timespec_wrapper{vb.cast_to_string()};
                        } else if(vb.is_undefined()) { return timespec_wrapper{};
                        }
                        return {};
                    }};
                    if(l.class_name() == "timestamp" && r.class_name() == "timestamp") {
                        return l.as_class<timespec_wrapper>() == r.as_class<timespec_wrapper>();
                    } else {
                        if(l.class_name() == "timestamp") {
                            if(auto ortsw{mb_constr(r)}) {
                                return l.as_class<timespec_wrapper>() == *ortsw;
                            }
                        } else if(r.class_name() == "timestamp") {
                            if(auto oltsw{mb_constr(l)}) {
                                return *oltsw == r.as_class<timespec_wrapper>();
                            }
                        }
                    }
                    return false;
                }
            );

            rt->add_object_binary_operation("timestamp", "<=>",
                [](valbox &l, valbox &r) -> valbox {
                    static const auto mb_constr{[](valbox const &vb) -> std::optional<timespec_wrapper> {
                        if(vb.is_any_int_number()) { return timespec_wrapper{vb.cast_to_s64()};
                        } else if(vb.is_any_fp_number()) { return timespec_wrapper{vb.cast_to_long_double()};
                        } else if(vb.is_string_ref()) { return timespec_wrapper{vb.as_string()};
                        } else if(vb.is_wstring_ref()) { return timespec_wrapper{vb.cast_to_string()};
                        } else if(vb.is_undefined()) { return timespec_wrapper{};
                        }
                        return {};
                    }};
                    static const auto cmp{[](timespec_wrapper const &l, timespec_wrapper const &r) -> int {
                        return l == r ? 0: (l < r ? -1: 1);
                    }};
                    if(l.class_name() == "timestamp" && r.class_name() == "timestamp") {
                        return cmp(l.as_class<timespec_wrapper>(), r.as_class<timespec_wrapper>());
                    } else {
                        if(l.class_name() == "timestamp") {
                            if(auto ortsw{mb_constr(r)}) {
                                return cmp(l.as_class<timespec_wrapper>(), *ortsw);
                            }
                        } else if(r.class_name() == "timestamp") {
                            if(auto oltsw{mb_constr(l)}) {
                                return cmp(*oltsw, r.as_class<timespec_wrapper>());
                            }
                        }
                    }
                    return teal::valbox{valbox_no_initialize::dont_do_it};
                }
            );

            rt->add_function("gmtimestamp", TEALFUN() { return teal::valbox{timespec_wrapper::gmtnow(), "timestamp"}; });
            rt->add_method("timestamp", "year", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).year(); });
            rt->add_method("timestamp", "month", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).month(); });
            rt->add_method("timestamp", "day", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).day(); });
            rt->add_method("timestamp", "weekday", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).weekday(); });
            rt->add_method("timestamp", "hour", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).hour(); });
            rt->add_method("timestamp", "min", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).min(); });
            rt->add_method("timestamp", "sec", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).sec_with_subsec(); });
            rt->add_method("timestamp", "seconds", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).seconds(); });
            rt->add_method("timestamp", "fseconds", TEALFUN(args) { return TEALTHIS(args, timespec_wrapper).fseconds(); });
            rt->add_method("timestamp", "to_string", TEALFUN(args) {
                std::size_t prec{static_cast<std::size_t>(9)};
                if(args.size() > 1) { prec = args[1].cast_num_to_num<std::size_t>(); }
                return TEALTHIS(args, timespec_wrapper).as_iso_8601_str(prec);
            });
            rt->add_method("timestamp", "as_iso_8601", TEALFUN(args) {
                std::size_t prec{static_cast<std::size_t>(9)};
                if(args.size() > 1) { prec = args[1].cast_num_to_num<std::size_t>(); }
                return TEALTHIS(args, timespec_wrapper).as_iso_8601_str(prec);
            });
            rt->add_method("timestamp", "as_gmt_iso_8601", TEALFUN(args) {
                std::size_t prec{static_cast<std::size_t>(9)};
                if(args.size() > 1) { prec = args[1].cast_num_to_num<std::size_t>(); }
                return TEALTHIS(args, timespec_wrapper).as_gmt_iso_8601_str(prec);
            });
            rt->add_method("timestamp", "from_string", TEALFUN(args) {
                std::string s{}; if(args.size() > 1) { s = args[1].cast_to_string(); }
                TEALTHIS(args, timespec_wrapper).from_iso_8601(s);
                return args[0];
            });
            rt->add_function("steady_clock", TEALFUN() {
                return static_cast<long double>(std::chrono::steady_clock::now().time_since_epoch().count()) / 1e9L;
            });
            rt->add_function("time", TEALFUN() {
                return timespec_wrapper::now().fseconds();
            });
            rt->add_function("gmtime", TEALFUN() {
                return timespec_wrapper::gmtnow().fseconds();
            });
        }

        void unregister_runtime() override {
            std::unique_lock l{rt_mtp_};
            if(rt_ == nullptr) {
                return;
            }
            rt_->remove_object_services("timestamp");
            rt_->remove_function("timestamp");
            rt_->remove_function("gmtimestamp");
            rt_->remove_function("steady_clock");
            rt_->remove_function("time");
            rt_->remove_function("gmtime");
            rt_->remove_method("timestamp", "year");
            rt_->remove_method("timestamp", "month");
            rt_->remove_method("timestamp", "day");
            rt_->remove_method("timestamp", "weekday");
            rt_->remove_method("timestamp", "hour");
            rt_->remove_method("timestamp", "min");
            rt_->remove_method("timestamp", "sec");
            rt_->remove_method("timestamp", "seconds");
            rt_->remove_method("timestamp", "fseconds");
            rt_->remove_method("timestamp", "as_iso_8601");
            rt_->remove_method("timestamp", "as_gmt_iso_8601");
            rt_->remove_method("timestamp", "from_string");
            rt_ = nullptr;
        }

    private:
        shared_mutex rt_mtp_{};
        runtime_interface *rt_{nullptr};
    };

}
