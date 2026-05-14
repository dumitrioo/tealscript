#pragma once

#include "inc/commondefs.hpp"
#include "inc/file_util.hpp"
#include "inc/str_util.hpp"
#include "inc/json.hpp"
#include "inc/math/vector4.hpp"
#include "inc/math/matrix4.hpp"
#include "inc/math/math_util.hpp"

#include "tealscript_util.hpp"

using std::any_cast;
using std::any;

namespace teal {

    enum class valbox_no_initialize {
        dont_do_it
    };

    class valbox {
    public:
        enum class type {
            BOOL,
            CHAR,
            S8,
            U8,
            S16,
            U16,
            WCHAR,
            S32,
            U32,
            S64,
            U64,
            FLOAT,
            DOUBLE,
            LONG_DOUBLE,
            VEC4,
            MAT4,
            POINTER,
            CLASS,
            FUNC,
            ARRAY,
            OBJECT,
            STRING,
            WSTRING,
            UNDEFINED,
            VALBOX,
        };

        struct valbox_type_hash {
            std::size_t operator()(const type &t) const noexcept {
                return static_cast<std::size_t>(t);
            }
        };

        static type str_to_type(std::string const &idr) {
            static str_map_t<type> const tmap{
                {"bool", type::BOOL},
                {"char", type::CHAR},
                {"wchar", type::WCHAR},
                {"string", type::STRING},
                {"wstring", type::WSTRING},
                {"i8", type::S8},
                {"u8", type::U8},
                {"i16", type::S16},
                {"u16", type::U16},
                {"i32", type::S32},
                {"u32", type::U32},
                {"i64", type::S64},
                {"u64", type::U64},
                {"vec4", type::VEC4},
                {"mat4", type::MAT4},
                {"pointer", type::POINTER},
                {"f32", type::FLOAT},
                {"f64", type::DOUBLE},
                {"float", type::LONG_DOUBLE},
                {"func", type::FUNC},
                {"array", type::ARRAY},
                {"object", type::OBJECT},
                {"undefined", type::UNDEFINED},
                {"class", type::CLASS},
                {"valbox", type::VALBOX},
            };
            auto it{tmap.find(idr)};
            if(it == tmap.end()) {
                throw std::runtime_error{"invalid type name"};
            }
            return it->second;
        }

        static std::string type_to_str(type t) {
            static std::array<std::string_view, 25> const names{
                "bool", "char", "i8", "u8", "i16", "u16", "wchar", "i32", "u32", "i64", "u64",
                "f32", "f64", "float", "vec4", "mat4", "pointer", "class", "func",
                "array",  "object", "string", "wstring", "undefined", "valbox",
            };
            auto i{static_cast<std::size_t>(t)};
            if(i > 24) {
                throw std::runtime_error{"invalid type"};
            }
            return std::string{names[i]};
        }

        static bool is_type(std::string const &idr) {
            static std::set<std::string> const tset{
                "object", "array", "bool", "char", "wchar", "string", "wstring", "u8", "i8", "u16"
                "i16", "u32", "i32", "u64", "i64", "f32", "f64", "float", "vec4", "mat4", "func",
                "pointer", "undefined", "valbox"
            };
            return tset.find(idr) != tset.end();
        }

    private:
        static size_t type_size(type t) {
            static std::array<size_t, 25> const sizes{
                sizeof(bool), sizeof(char), sizeof(int8_t), sizeof(uint8_t), sizeof(int16_t), sizeof(uint16_t),
                sizeof(wchar_t), sizeof(int32_t), sizeof(uint32_t), sizeof(int64_t), sizeof(uint64_t),
                sizeof(float), sizeof(double), sizeof(long double), sizeof(vec4_t), sizeof(mat4_t),
                sizeof(void *), sizeof(void *), sizeof(std::function<valbox(std::vector<valbox> &)>),
                sizeof(array_t), sizeof(object_t), sizeof(std::string), sizeof(std::wstring), 0, 0,
            };
            auto i{static_cast<std::size_t>(t)};
            if(i > 24) {
                throw std::runtime_error{"invalid type"};
            }
            return sizes[i];
        }

    public:
        using vec4_t = teal::math::vector4<double>;
        using mat4_t = teal::math::matrix4<double>;
        using array_t = std::deque<valbox>;
        using object_t = str_map_t<valbox>;

    private:
        using value_t = std::variant<
            bool,
            char,
            std::int8_t,
            std::uint8_t,
            std::int16_t,
            std::uint16_t,
            wchar_t,
            std::int32_t,
            std::uint32_t,
            std::int64_t,
            std::uint64_t,
            float,
            double,
            long double,
            void *,
            std::function<valbox(std::vector<valbox> &)>,
            array_t,
            object_t,
            std::string,
            std::wstring,
            any
        >;

        struct box_data {
            box_data() = default;
            box_data(
                value_t const &v, type t, type pointed_type = type::UNDEFINED,
                std::string const &c = {}, std::string const &func_name = {},
                bool user_func = false
            ):
                value_{v}, type_{t}, pointed_type_{pointed_type},
                class_{c}, func_name_{func_name}, user_func_{user_func}
            {
            }
            box_data(
                value_t &&v, type t, type pointed_type = type::UNDEFINED,
                std::string const &c = {}, std::string const &func_name = {},
                bool user_func = false
            ):
                value_{std::move(v)}, type_{t}, pointed_type_{pointed_type},
                class_{c}, func_name_{func_name}, user_func_{user_func}
            {
            }
            void undefine() {
                type_ = type::UNDEFINED;
                pointed_type_ = type::UNDEFINED;
                class_.clear();
                func_name_.clear();
                user_func_ = false;
            }
            value_t value_{nullptr};
            type type_{type::UNDEFINED};
            type pointed_type_{type::UNDEFINED};
            std::string class_{};
            std::string func_name_{};
            bool user_func_{false};
        };

    public:
        valbox(): box_{std::make_shared<box_data>(value_t{}, type::UNDEFINED)} {}
        valbox(valbox_no_initialize) {}
        valbox(bool v): box_{std::make_shared<box_data>(v, type::BOOL)} {}
        valbox(bool *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::BOOL)} {}
        valbox(float v): box_{std::make_shared<box_data>(v, type::FLOAT)} {}
        valbox(float *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::FLOAT)} {}
        valbox(double v): box_{std::make_shared<box_data>(v, type::DOUBLE)} {}
        valbox(double *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::DOUBLE)} {}
        valbox(long double v): box_{std::make_shared<box_data>(v, type::LONG_DOUBLE)} {}
        valbox(long double *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::LONG_DOUBLE)} {}
        valbox(char v): box_{std::make_shared<box_data>(v, type::CHAR)} {}
        valbox(char *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::CHAR)} {}
        valbox(wchar_t v): box_{std::make_shared<box_data>(v, type::WCHAR)} {}
        valbox(wchar_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::WCHAR)} {}
        valbox(std::string const &v): box_{std::make_shared<box_data>(v, type::STRING)} {}
        valbox(std::string *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::STRING)} {}
        valbox(char const *v): box_{std::make_shared<box_data>(std::string{v}, type::STRING)} {}
        valbox(std::wstring const &v): box_{std::make_shared<box_data>(v, type::WSTRING)} {}
        valbox(std::wstring *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::WSTRING)} {}
        valbox(wchar_t const *v): box_{std::make_shared<box_data>(std::wstring{v}, type::WSTRING)} {}
        valbox(std::string &&v): box_{std::make_shared<box_data>(std::move(v), type::STRING)} {}
        valbox(std::wstring &&v): box_{std::make_shared<box_data>(std::move(v), type::WSTRING)} {}
        valbox(std::int8_t v): box_{std::make_shared<box_data>(v, type::S8)} {}
        valbox(std::int8_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::S8)} {}
        valbox(std::uint8_t v): box_{std::make_shared<box_data>(v, type::U8)} {}
        valbox(std::uint8_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::U8)} {}
        valbox(std::int16_t v): box_{std::make_shared<box_data>(v, type::S16)} {}
        valbox(std::int16_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::S16)} {}
        valbox(std::uint16_t v): box_{std::make_shared<box_data>(v, type::U16)} {}
        valbox(std::uint16_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::U16)} {}
        valbox(std::int32_t v): box_{std::make_shared<box_data>(v, type::S32)} {}
        valbox(std::int32_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::S32)} {}
        valbox(std::uint32_t v): box_{std::make_shared<box_data>(v, type::U32)} {}
        valbox(std::uint32_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::U32)} {}
        valbox(std::int64_t v): box_{std::make_shared<box_data>(v, type::S64)} {}
        valbox(std::int64_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::S64)} {}
        valbox(std::uint64_t v): box_{std::make_shared<box_data>(v, type::U64)} {}
        valbox(std::uint64_t *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::U64)} {}
        valbox(vec4_t const &v): box_{std::make_shared<box_data>(any{v}, type::VEC4)} {}
        valbox(mat4_t const &v): box_{std::make_shared<box_data>(any{v}, type::MAT4)} {}
        valbox(long long v): box_{std::make_shared<box_data>((std::int64_t)v, type::S64)} {}
        valbox(long long *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::S64)} {}
        valbox(unsigned long long v): box_{std::make_shared<box_data>((std::uint64_t)v, type::U64)} {}
        valbox(unsigned long long *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::U64)} {}
        valbox(json const &v): box_{std::make_shared<box_data>(value_t{}, type::UNDEFINED)} { from_json(v); }
        valbox(object_t const &v): box_{std::make_shared<box_data>(v, type::OBJECT)} {}
        valbox(void *v, type pointed_type = type::POINTER): box_{std::make_shared<box_data>(v, type::POINTER, pointed_type)} {}
        valbox(valbox *v): box_{std::make_shared<box_data>((void *)v, type::POINTER, type::VALBOX)} {}
        template<typename T>
        valbox(T &&v, std::string const &classname):
            box_{std::make_shared<box_data>(any{std::forward<T>(v)}, type::CLASS, type::UNDEFINED, classname)}
        {
        }
        valbox(std::function<valbox(std::vector<valbox> &)> const &v, std::string const &func_name, bool user_func):
            box_{
                std::make_shared<box_data>(
                    v, type::FUNC, type::UNDEFINED,
                    std::string{}, func_name, user_func
                )
            }
        {
        }
        valbox(std::function<valbox(std::vector<valbox> &)> &&v, std::string const &func_name, bool user_func):
            box_{
                std::make_shared<box_data>(std::move(v), type::FUNC,
                type::UNDEFINED, std::string{}, func_name, user_func)
            }
        {
        }

        void construct(array_t const &arr) {
            box_ = std::make_shared<box_data>(arr, type::ARRAY);
            pointed_box_.reset();
            array_t &a{as_array()};
            for(auto &&v: arr) {
                a.push_back(v.clone());
            }
            a.resize(arr.size());
        }

        void construct(array_t &&arr) {
            box_ = std::make_shared<box_data>(std::move(arr), type::ARRAY);
        }

        template<typename T
#if (__cplusplus < 202000L)
                 , std::enable_if_t<std::is_constructible_v<valbox, typename T::value_type> &&
                    !std::is_same_v<valbox, typename T::value_type>, bool> = true
#endif
                 >
#if (__cplusplus >= 202000L)
            requires(
                std::is_constructible_v<valbox, typename T::value_type> &&
                !std::is_same_v<valbox, typename T::value_type>
            )
#endif
        valbox(T &&container): box_{std::make_shared<box_data>(array_t{}, type::ARRAY)} {
            auto &a{as_array()};
            for(auto &&v: container) {
                a.push_back(v);
            }
        }

        template<typename T
#if (__cplusplus < 202000L)
                 , std::enable_if_t<std::is_constructible_v<valbox, typename T::value_type> &&
                    !std::is_same_v<valbox, typename T::value_type>, bool> = true
#endif
                 >
#if (__cplusplus >= 202000L)
            requires(
                std::is_constructible_v<valbox, typename T::value_type> &&
                !std::is_same_v<valbox, typename T::value_type>
                )
#endif
        valbox(T const &container): box_{std::make_shared<box_data>(array_t{}, type::ARRAY)} {
            auto &a{as_array()};
            for(auto &&v: container) {
                a.push_back(v);
            }
        }

#if 0
        valbox(std::initializer_list<valbox> initlist):
            box_{std::make_shared<box_data>(array_t{}, type::ARRAY)}
        {
            for(auto &&v: initlist) {
                as_array().push_back(v);
            }
        }
#endif
        ~valbox() = default;
        valbox(valbox const &that):
            box_{that.box_},
            pointed_box_{that.pointed_box_}
        {
        }
        valbox(valbox &&that) noexcept:
            box_{std::move(that.box_)},
            pointed_box_{std::move(that.pointed_box_)}
        {
        }
        valbox &operator=(valbox const &that) {
            if(&that != this) {
                pointed_box_ = that.pointed_box_;
                box_ = that.box_;
            }
            return *this;
        }
        valbox &operator=(valbox &&that) noexcept {
            if(&that != this) {
                std::swap(box_, that.box_);
                std::swap(pointed_box_, that.pointed_box_);
            }
            return *this;
        }

        bool is_another_ref(valbox const &other) const {
            return box_.get() != nullptr && box_.get() == other.box_.get();
        }

        valbox clone() const {
            valbox const &dr{deref()};
            type drt{dr.val_or_pointed_type()};
            if(drt == type::OBJECT) {
                object_t o{};
                for(auto &&p: dr.as_object()) {
                    o[p.first] = p.second.clone();
                }
                valbox res{std::make_shared<box_data>(
                        std::move(o),
                        dr.box_->type_,
                        dr.box_->pointed_type_,
                        dr.box_->class_,
                        dr.box_->func_name_,
                        dr.box_->user_func_
                    )
                };
                res.pointed_box_ = dr.pointed_box_;
                return res;
            } else if(drt == type::ARRAY) {
                array_t a{};
                for(auto &&i: dr.as_array()) {
                    a.push_back(i.clone());
                }
                a.resize(dr.as_array().size());
                valbox res{std::make_shared<box_data>(
                        std::move(a),
                        dr.box_->type_,
                        dr.box_->pointed_type_,
                        dr.box_->class_,
                        dr.box_->func_name_,
                        dr.box_->user_func_
                    )
                };
                res.pointed_box_ = dr.pointed_box_;
                return res;
            } else if(dr.box_) {
                valbox res{std::make_shared<box_data>(
                        dr.box_->value_,
                        dr.box_->type_,
                        dr.box_->pointed_type_,
                        dr.box_->class_,
                        dr.box_->func_name_,
                        dr.box_->user_func_
                    )
                };
                res.pointed_box_ = dr.pointed_box_;
                return res;
            } else {
                return valbox{valbox_no_initialize::dont_do_it};
            }
        }

        void undefine() {
            if(box_) {
                box_->undefine();
            }
            pointed_box_.reset();
        }

        type val_type() const { return box_ ? box_->type_ : type::UNDEFINED; }
        type pointed_type() const {
            return box_ ?
                (box_->type_ == type::POINTER ? box_->pointed_type_ : type::UNDEFINED) :
                type::UNDEFINED;
        }
        type val_or_pointed_type() const {
            if(!box_) { return type::UNDEFINED; }
            return box_->type_ == type::POINTER ?
               (
                    box_->pointed_type_ == type::VALBOX ?
                    reinterpret_cast<valbox const *>(std::get<void *>(box_->value_))->val_or_pointed_type() :
                    box_->pointed_type_
               ) :
               box_->type_;
        }

        std::string val_class_name() const {
            return box_ ? box_->class_ : std::string{};
        }
        std::string ref_class_name() const {
            valbox const &dr{deref()};
            return dr.box_ ? dr.box_->class_ : std::string{};
        }

        template<typename T>
        std::remove_cv_t<T> &deref_ptr() {
            return *reinterpret_cast<std::remove_cv_t<T> *>(as_ptr());
        }
        template<typename T>
        std::remove_cv_t<T> const &deref_ptr() const {
            return *reinterpret_cast<std::remove_cv_t<T> const *>(as_ptr());
        }

        void *as_ptr() {
            if(!box_) { return nullptr; }
            return std::get<void *>(box_->value_);
        }
        void const *as_ptr() const {
            if(!box_) { return nullptr; }
            return std::get<void *>(box_->value_);
        }
        template<typename T>
        std::remove_cv_t<T> *as_typed_ptr() {
            if(!box_) { return nullptr; }
            return reinterpret_cast<std::remove_cv_t<T> *>(std::get<void *>(box_->value_));
        }
        template<typename T>
        std::remove_cv_t<T> const *as_typed_ptr() const {
            if(!box_) { return nullptr; }
            return reinterpret_cast<std::remove_cv_t<T> *>(std::get<void *>(box_->value_));
        }
        bool is_ptr() const { return box_ && box_->type_ == type::POINTER; }

        box_data *as_valbox_ptr() const {
            box_data const *box{box_.get()};
            return reinterpret_cast<box_data *>(
                static_cast<uintptr_t>(
                    box != nullptr &&
                    box->type_ == type::POINTER &&
                    box->pointed_type_ == type::VALBOX ? 1 : 0
                )
                *
                reinterpret_cast<uintptr_t>(box)
            );
        }
        valbox &deref() {
            box_data *bptr{as_valbox_ptr()};
            if(bptr != nullptr) {
                void *p{std::get<void *>(bptr->value_)};
                if(p == reinterpret_cast<void *>(this)) {
                    throw std::runtime_error{"reference loop"};
                }
                return reinterpret_cast<valbox *>(p)->deref();
            }
            return *this;
        }
        valbox const &deref() const {
            box_data *bptr{as_valbox_ptr()};
            if(bptr != nullptr) {
                void *p{std::get<void *>(bptr->value_)};
                if(p == reinterpret_cast<void const *>(this)) {
                    throw std::runtime_error{"reference loop"};
                }
                return reinterpret_cast<valbox const *>(p)->deref();
            }
            return *this;
        }

        template<typename T>
        T &as_class() {
            if(!box_) {
                throw std::runtime_error{"not an object"};
            }
            return any_cast<T &>(std::get<any>(deref().box_->value_));
        }
        template<typename T>
        T const &as_class() const {
            if(!box_) {
                throw std::runtime_error{"not an object"};
            }
            return any_cast<T &>(std::get<any>(deref().box_->value_));
        }
        bool is_class_ref() const { return val_or_pointed_type() == type::CLASS; }
        std::string class_name() const { return box_ ? deref().box_->class_ : std::string{}; }

        bool is_string_ref() const { return val_or_pointed_type() == type::STRING; }
        std::string &as_string() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::STRING || dr.box_->pointed_type_ == type::STRING))
                throw std::runtime_error{"not a string"};
            return dr.box_->pointed_type_ == type::STRING ?
                        dr.deref_ptr<std::string>() :
                        std::get<std::string>(dr.box_->value_);
        }
        std::string const &as_string() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::STRING || dr.box_->pointed_type_ == type::STRING))
                throw std::runtime_error{"not a string"};
            return dr.box_->pointed_type_ == type::STRING ?
                        dr.deref_ptr<std::string>() :
                        std::get<std::string>(dr.box_->value_);
        }

        bool is_wstring_ref() const { return val_or_pointed_type() == type::WSTRING; }
        std::wstring &as_wstring() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::WSTRING || dr.box_->pointed_type_ == type::WSTRING))
                throw std::runtime_error{"not a wide string"};
            return dr.box_->pointed_type_ == type::WSTRING ?
                        dr.deref_ptr<std::wstring>() :
                        std::get<std::wstring>(dr.box_->value_);
        }
        std::wstring const &as_wstring() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::WSTRING || dr.box_->pointed_type_ == type::WSTRING))
                throw std::runtime_error{"not a wide string"};
            return dr.box_->pointed_type_ == type::WSTRING ?
                        dr.deref_ptr<std::wstring>() :
                        std::get<std::wstring>(dr.box_->value_);
        }

        bool is_long_double_ref() const { return val_or_pointed_type() == type::LONG_DOUBLE; }
        long double &as_long_double() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::LONG_DOUBLE || dr.box_->pointed_type_ == type::LONG_DOUBLE))
                throw std::runtime_error{"not a long double value"};
            return dr.box_->pointed_type_ == type::LONG_DOUBLE ?
                        dr.deref_ptr<long double>() :
                        std::get<long double>(dr.box_->value_);
        }
        long double const &as_long_double() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::LONG_DOUBLE || dr.box_->pointed_type_ == type::LONG_DOUBLE))
                throw std::runtime_error{"not a long double value"};
            return dr.box_->pointed_type_ == type::LONG_DOUBLE ?
                        dr.deref_ptr<long double>() :
                        std::get<long double>(dr.box_->value_);
        }

        bool is_double_ref() const { return val_or_pointed_type() == type::DOUBLE; }
        double &as_double() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::DOUBLE || dr.box_->pointed_type_ == type::DOUBLE))
                throw std::runtime_error{"not a double value"};
            return dr.box_->pointed_type_ == type::DOUBLE ?
                        dr.deref_ptr<double>() :
                        std::get<double>(dr.box_->value_);
        }
        double as_double() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::DOUBLE || dr.box_->pointed_type_ == type::DOUBLE))
                throw std::runtime_error{"not a double value"};
            return dr.box_->pointed_type_ == type::DOUBLE ?
                        dr.deref_ptr<double>() :
                        std::get<double>(dr.box_->value_);
        }

        bool is_float_ref() const { return val_or_pointed_type() == type::FLOAT; }
        float &as_float() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::FLOAT || dr.box_->pointed_type_ == type::FLOAT))
                throw std::runtime_error{"not a floating point value"};
            return dr.box_->pointed_type_ == type::FLOAT ?
                        dr.deref_ptr<float>() :
                        std::get<float>(dr.box_->value_);
        }
        float as_float() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::FLOAT || dr.box_->pointed_type_ == type::FLOAT))
                throw std::runtime_error{"not a floating point value"};
            return dr.box_->pointed_type_ == type::FLOAT ?
                        dr.deref_ptr<float>() :
                        std::get<float>(dr.box_->value_);
        }

        bool is_bool_ref() const { return val_or_pointed_type() == type::BOOL; }
        bool &as_bool() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::BOOL || dr.box_->pointed_type_ == type::BOOL))
                throw std::runtime_error{"not a boolean value"};
            return dr.box_->pointed_type_ == type::BOOL ?
                        dr.deref_ptr<bool>() :
                        std::get<bool>(dr.box_->value_);
        }
        bool as_bool() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::BOOL || dr.box_->pointed_type_ == type::BOOL))
                throw std::runtime_error{"not a boolean value"};
            return dr.box_->pointed_type_ == type::BOOL ?
                        dr.deref_ptr<bool>() :
                        std::get<bool>(dr.box_->value_);
        }

        bool is_char_ref() const { return val_or_pointed_type() == type::CHAR; }
        char &as_char() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::CHAR || dr.box_->pointed_type_ == type::CHAR))
                throw std::runtime_error{"not a character"};
            return dr.box_->pointed_type_ == type::CHAR ?
                        dr.deref_ptr<char>() :
                        std::get<char>(dr.box_->value_);
        }
        char as_char() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::CHAR || dr.box_->pointed_type_ == type::CHAR))
                throw std::runtime_error{"not a character"};
            return dr.box_->pointed_type_ == type::CHAR ?
                        dr.deref_ptr<char>() :
                        std::get<char>(dr.box_->value_);
        }

        bool is_wchar_ref() const { return val_or_pointed_type() == type::WCHAR; }
        wchar_t &as_wchar() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::WCHAR || dr.box_->pointed_type_ == type::WCHAR))
                throw std::runtime_error{"not a wide character"};
            return dr.box_->pointed_type_ == type::WCHAR ?
                        dr.deref_ptr<wchar_t>() :
                        std::get<wchar_t>(dr.box_->value_);
        }
        wchar_t as_wchar() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::WCHAR || dr.box_->pointed_type_ == type::WCHAR))
                throw std::runtime_error{"not a wide character"};
            return dr.box_->pointed_type_ == type::WCHAR ?
                        dr.deref_ptr<wchar_t>() :
                        std::get<wchar_t>(dr.box_->value_);
        }

        bool is_u8_ref() const { return val_or_pointed_type() == type::U8; }
        std::uint8_t &as_u8() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U8 || dr.box_->pointed_type_ == type::U8))
                throw std::runtime_error{"not a 8-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U8 ?
                        dr.deref_ptr<std::uint8_t>() :
                        std::get<std::uint8_t>(dr.box_->value_);
        }
        std::uint8_t as_u8() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U8 || dr.box_->pointed_type_ == type::U8))
                throw std::runtime_error{"not a 8-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U8 ?
                        dr.deref_ptr<std::uint8_t>() :
                        std::get<std::uint8_t>(dr.box_->value_);
        }

        bool is_s8_ref() const { return val_or_pointed_type() == type::S8; }
        std::int8_t &as_s8() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S8 || dr.box_->pointed_type_ == type::S8))
                throw std::runtime_error{"not a 8-bit signed integer"};
            return dr.box_->pointed_type_ == type::S8 ?
                        dr.deref_ptr<std::int8_t>() :
                        std::get<std::int8_t>(dr.box_->value_);
        }
        std::int8_t as_s8() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S8 || dr.box_->pointed_type_ == type::S8))
                throw std::runtime_error{"not a 8-bit signed integer"};
            return dr.box_->pointed_type_ == type::S8 ?
                        dr.deref_ptr<std::int8_t>() :
                        std::get<std::int8_t>(dr.box_->value_);
        }

        bool is_u16_ref() const { return val_or_pointed_type() == type::U16; }
        std::uint16_t &as_u16() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U16 || dr.box_->pointed_type_ == type::U16))
                throw std::runtime_error{"not a 16-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U16 ?
                        dr.deref_ptr<std::uint16_t>() :
                        std::get<std::uint16_t>(dr.box_->value_);
        }
        std::uint16_t as_u16() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U16 || dr.box_->pointed_type_ == type::U16))
                throw std::runtime_error{"not a 16-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U16 ?
                        dr.deref_ptr<std::uint16_t>() :
                        std::get<std::uint16_t>(dr.box_->value_);
        }

        bool is_s16_ref() const { return val_or_pointed_type() == type::S16; }
        std::int16_t &as_s16() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S16 || dr.box_->pointed_type_ == type::S16))
                throw std::runtime_error{"not a 16-bit signed integer"};
            return dr.box_->pointed_type_ == type::S16 ?
                        dr.deref_ptr<std::int16_t>() :
                        std::get<std::int16_t>(dr.box_->value_);
        }
        std::int16_t as_s16() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S16 || dr.box_->pointed_type_ == type::S16))
                throw std::runtime_error{"not a 16-bit signed integer"};
            return dr.box_->pointed_type_ == type::S16 ?
                        dr.deref_ptr<std::int16_t>() :
                        std::get<std::int16_t>(dr.box_->value_);
        }

        bool is_u32_ref() const { return val_or_pointed_type() == type::U32; }
        std::uint32_t &as_u32() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U32 || dr.box_->pointed_type_ == type::U32))
                throw std::runtime_error{"not a 32-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U32 ?
                        dr.deref_ptr<std::uint32_t>() :
                        std::get<std::uint32_t>(dr.box_->value_);
        }
        std::uint32_t as_u32() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U32 || dr.box_->pointed_type_ == type::U32))
                throw std::runtime_error{"not a 32-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U32 ?
                        dr.deref_ptr<std::uint32_t>() :
                        std::get<std::uint32_t>(dr.box_->value_);
        }

        bool is_s32_ref() const { return val_or_pointed_type() == type::S32; }
        std::int32_t &as_s32() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S32 || dr.box_->pointed_type_ == type::S32))
                throw std::runtime_error{"not a 32-bit signed integer"};
            return dr.box_->pointed_type_ == type::S32 ?
                        dr.deref_ptr<std::int32_t>() :
                        std::get<std::int32_t>(dr.box_->value_);
        }
        std::int32_t as_s32() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S32 || dr.box_->pointed_type_ == type::S32))
                throw std::runtime_error{"not a 32-bit signed integer"};
            return dr.box_->pointed_type_ == type::S32 ?
                        dr.deref_ptr<std::int32_t>() :
                        std::get<std::int32_t>(dr.box_->value_);
        }

        bool is_u64_ref() const { return val_or_pointed_type() == type::U64; }
        std::uint64_t &as_u64() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U64 || dr.box_->pointed_type_ == type::U64))
                throw std::runtime_error{"not a 64-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U64 ?
                        dr.deref_ptr<std::uint64_t>() :
                        std::get<std::uint64_t>(dr.box_->value_);
        }
        std::uint64_t as_u64() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::U64 || dr.box_->pointed_type_ == type::U64))
                throw std::runtime_error{"not a 64-bit unsigned integer"};
            return dr.box_->pointed_type_ == type::U64 ?
                        dr.deref_ptr<std::uint64_t>() :
                        std::get<std::uint64_t>(dr.box_->value_);
        }

        bool is_s64_ref() const { return val_or_pointed_type() == type::S64; }
        std::int64_t &as_s64() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S64 || dr.box_->pointed_type_ == type::S64))
                throw std::runtime_error{"not a 64-bit signed integer"};
            return dr.box_->pointed_type_ == type::S64 ?
                        dr.deref_ptr<std::int64_t>() :
                        std::get<std::int64_t>(dr.box_->value_);
        }
        std::int64_t as_s64() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::S64 || dr.box_->pointed_type_ == type::S64))
                throw std::runtime_error{"not a 64-bit signed integer"};
            return dr.box_->pointed_type_ == type::S64 ?
                        dr.deref_ptr<std::int64_t>() :
                        std::get<std::int64_t>(dr.box_->value_);
        }

        std::string func_name() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !dr.is_func_ref()) {
                throw std::runtime_error{"not a function"};
            }
            return dr.box_->func_name_;
        }
        bool is_user_func() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !dr.is_func_ref()) {
                return false;
            }
            return dr.box_->user_func_;
        }
        bool is_func_ref() const { return val_or_pointed_type() == type::FUNC; }
        std::function<valbox(std::vector<valbox> &)> const &as_func() {
            valbox &dr{deref()};
            if(!dr.box_) { throw std::runtime_error{"not a function"}; }
            return std::get<std::function<valbox(std::vector<valbox> &)>>(dr.box_->value_);
        }
        std::function<valbox(std::vector<valbox> &)> const &as_func() const {
            valbox const &dr{deref()};
            if(!dr.box_) { throw std::runtime_error{"not a function"}; }
            return std::get<std::function<valbox(std::vector<valbox> &)>>(dr.box_->value_);
        }

        bool is_vec4_ref() const { return val_or_pointed_type() == type::VEC4; }
        vec4_t &as_vec4() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::VEC4 || dr.box_->pointed_type_ == type::VEC4))
                throw std::runtime_error{"not a vec4"};
            return dr.box_->pointed_type_ == type::VEC4 ?
                        std::any_cast<vec4_t &>(dr.deref_ptr<any>()) :
                        std::any_cast<vec4_t &>(std::get<any>(dr.box_->value_));
        }
        vec4_t const &as_vec4() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::VEC4 || dr.box_->pointed_type_ == type::VEC4))
                throw std::runtime_error{"not a vec4"};
            return dr.box_->pointed_type_ == type::VEC4 ?
                        std::any_cast<vec4_t const &>(dr.deref_ptr<any>()) :
                        std::any_cast<vec4_t const &>(std::get<any>(dr.box_->value_));
        }

        bool is_mat4_ref() const { return val_or_pointed_type() == type::MAT4; }
        mat4_t &as_mat4() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::MAT4 || dr.box_->pointed_type_ == type::MAT4))
                throw std::runtime_error{"not a mat4"};
            return dr.box_->pointed_type_ == type::MAT4 ?
                        std::any_cast<mat4_t &>(dr.deref_ptr<any>()) :
                        std::any_cast<mat4_t &>(std::get<any>(dr.box_->value_));
        }
        mat4_t const &as_mat4() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::MAT4 || dr.box_->pointed_type_ == type::MAT4))
                throw std::runtime_error{"not a mat4"};
            return dr.box_->pointed_type_ == type::MAT4 ?
                        std::any_cast<mat4_t const &>(dr.deref_ptr<any>()) :
                        std::any_cast<mat4_t const &>(std::get<any>(dr.box_->value_));
        }

        bool is_array_ref() const { return val_or_pointed_type() == type::ARRAY; }
        array_t &as_array() {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::ARRAY || dr.box_->pointed_type_ == type::ARRAY))
                throw std::runtime_error{"not an array"};
            return dr.box_->pointed_type_ == type::ARRAY ?
                        dr.deref_ptr<array_t>() :
                        std::get<array_t>(dr.box_->value_);
        }
        array_t const &as_array() const {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::ARRAY || dr.box_->pointed_type_ == type::ARRAY))
                throw std::runtime_error{"not an array"};
            return dr.box_->pointed_type_ == type::ARRAY ?
                        dr.deref_ptr<array_t>() :
                        std::get<array_t>(dr.box_->value_);
        }

        valbox subarray(std::size_t start = 0, std::size_t n = std::numeric_limits<std::size_t>::max()) const {
            if(!is_array_ref()) {
                throw std::runtime_error{"not na array"};
            }
            array_t const &a{as_array()};
            valbox res{valbox_no_initialize::dont_do_it};
            res.become_array();
            array_t &resarr{res.as_array()};
            if(n > 0 && start < a.size()) {
                std::size_t cnt{0};
                for(auto it{a.begin() + start}; it != a.end() && cnt < n; ++it) {
                    resarr.push_back(it->clone());
                    ++cnt;
                }
            }
            return res;
        }

        bool is_object_ref() const { return val_or_pointed_type() == type::OBJECT; }
        object_t &as_object() & {
            valbox &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::OBJECT || dr.box_->pointed_type_ == type::OBJECT))
                throw std::runtime_error{"not an object"};
            return dr.box_->pointed_type_ == type::OBJECT ?
                        dr.deref_ptr<object_t>() :
                        std::get<object_t>(dr.box_->value_);
        }
        object_t const &as_object() const & {
            valbox const &dr{deref()};
            if(!dr.box_ || !(dr.box_->type_ == type::OBJECT || dr.box_->pointed_type_ == type::OBJECT))
                throw std::runtime_error{"not an object"};
            return dr.box_->pointed_type_ == type::OBJECT ?
                        dr.deref_ptr<object_t>() :
                        std::get<object_t>(dr.box_->value_);
        }

        bool is_undefined() const { return !box_ || box_->type_ == type::UNDEFINED; }
        bool is_undefined_ref() const { return val_or_pointed_type() == type::UNDEFINED; }

        valbox &become_array() {
            valbox &dr{deref()};
            if(!dr.box_) {
                dr.box_ = std::make_shared<box_data>(array_t{}, type::ARRAY);
                dr.pointed_box_.reset();
            } else if(dr.val_or_pointed_type() != type::ARRAY) {
                dr.box_->value_ = array_t{};
                dr.box_->type_ = type::ARRAY;
                dr.box_->class_.clear();
                dr.box_->pointed_type_ = type::UNDEFINED;
                dr.box_->func_name_.clear();
                dr.box_->user_func_ = false;
                dr.pointed_box_.reset();
            }
            return *this;
        }

        valbox &become_object() {
            valbox &dr{deref()};
            if(!dr.box_) {
                dr.box_ = std::make_shared<box_data>(object_t{}, type::OBJECT);
            } else if(dr.val_or_pointed_type() != type::OBJECT) {
                dr.box_->value_ = object_t{};
                dr.box_->type_ = type::OBJECT;
                dr.box_->class_.clear();
                dr.box_->pointed_type_ = type::UNDEFINED;
                dr.box_->func_name_.clear();
                dr.box_->user_func_ = false;
                dr.pointed_box_.reset();
            }
            return *this;
        }

        bool is_numeric() const {
            int t{(int)val_or_pointed_type()};
            return t >= (int)type::BOOL && t <= (int)type::LONG_DOUBLE;
        }

        bool is_any_fp_number() const {
            int t{(int)val_or_pointed_type()};
            return t >= (int)type::FLOAT && t <= (int)type::LONG_DOUBLE;
        }

        bool is_any_int_number() const {
            int t{(int)val_or_pointed_type()};
            return t >= (int)type::CHAR && t <= (int)type::U64;
        }

        bool is_any_signed_int_number() const {
            auto t{val_or_pointed_type()};
            return t == type::CHAR || t == type::S8 || t == type::S16 || t == type::S32 || t == type::S64;
        }

        bool is_any_unsigned_int_number() const {
            auto t{val_or_pointed_type()};
            return t == type::WCHAR || t == type::U8 || t == type::U16 || t == type::U32 || t == type::U64;
        }


        static bool is_numeric_type(type t) {
            int i{static_cast<int>(t)};
            return i >= static_cast<int>(type::BOOL) && i <= static_cast<int>(type::LONG_DOUBLE);
        }

        static bool is_any_fp_number_type(type t) {
            int i{static_cast<int>(t)};
            return i >= static_cast<int>(type::FLOAT) && i <= static_cast<int>(type::LONG_DOUBLE);
        }

        static bool is_any_int_number_type(type t) {
            int i{static_cast<int>(t)};
            return i >= static_cast<int>(type::CHAR) && i <= static_cast<int>(type::U64);
        }

        static bool is_any_signed_int_number_type(type t) {
            return t == type::CHAR || t == type::S8 || t == type::S16 || t == type::S32 || t == type::S64;
        }

        static bool is_any_unsigned_int_number_type(type t) {
            return t == type::WCHAR || t == type::U8 || t == type::U16 || t == type::U32 || t == type::U64;
        }

        static bool is_any_string_type(type t) {
            return t == type::WSTRING || t == type::STRING;
        }

        valbox operator[](valbox const &indx) {
            valbox const &indr{indx.deref()};
            valbox &der{deref()};
            auto t_of_indx{indr.val_or_pointed_type()};
            auto t{val_or_pointed_type()};
            if(t_of_indx == type::STRING) {
                if(is_undefined_ref()) {
                    become_object();
                }
                if(is_object_ref()) {
                    return as_object()[indx.as_string()];
                }
            } else if(t_of_indx == type::WSTRING) {
                if(t == type::UNDEFINED) {
                    become_object();
                }
                if(t == type::OBJECT) {
                    return as_object()[teal::str_util::to_utf8(indx.as_wstring())];
                }
            } else if(is_numeric_type(t_of_indx)) {
                std::uint64_t i{indx.cast_to_u64()};
                if(t == type::OBJECT) {
                    object_t &o{as_object()};
                    if(o.size() > i) {
                        std::size_t curr_indx{0};
                        for(auto &&kv: o) {
                            if(curr_indx == i) {
                                return &kv.second;
                            }
                            ++curr_indx;
                        }
                    } else {
                        throw std::range_error{"index out of range"};
                    }
                } else if(t == type::STRING) {
                    std::string &s{as_string()};
                    if(s.size() > i) {
                        return &s[i];
                    } else {
                        throw std::range_error{"index out of range"};
                    }
                } else if(t == type::WSTRING) {
                    std::wstring &s{as_wstring()};
                    if(s.size() > i) {
                        return &s[i];
                    } else {
                        throw std::range_error{"index out of range"};
                    }
                } else {
                    if(t == type::UNDEFINED) {
                        der.become_array();
                        t = type::ARRAY;
                    }
                    if(t == type::ARRAY) {
                        auto &a{as_array()};
                        if(a.size() <= i) { a.resize(i + 1); }
                        return a[i];
                    } else if(t == type::MAT4) {
                        mat4_t &a{as_mat4()};
                        if(i < 4) {
                            return a[i].ptr();
                        } else {
                            throw std::range_error{"index out of range"};
                        }
                    } else if(t == type::VEC4) {
                        vec4_t &a{as_vec4()};
                        valbox res{&a[i]};
                        return res;
                    } else if(der.is_ptr()) {
                        if(der.pointed_type() == type::LONG_DOUBLE) {
                            valbox res{der.as_typed_ptr<long double>() + i};
                            return res;
                        } else if(der.pointed_type() == type::BOOL) {
                            valbox res{der.as_typed_ptr<bool>() + i};
                            return res;
                        } else if(der.pointed_type() == type::CHAR) {
                            valbox res{der.as_typed_ptr<char>() + i};
                            return res;
                        } else if(der.pointed_type() == type::S8) {
                            valbox res{der.as_typed_ptr<std::int8_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::U8) {
                            valbox res{der.as_typed_ptr<std::uint8_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::S16) {
                            valbox res{der.as_typed_ptr<std::int16_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::U16) {
                            valbox res{der.as_typed_ptr<std::uint16_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::WCHAR) {
                            valbox res{der.as_typed_ptr<wchar_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::S32) {
                            valbox res{der.as_typed_ptr<std::int32_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::U32) {
                            valbox res{der.as_typed_ptr<std::uint32_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::S64) {
                            valbox res{der.as_typed_ptr<std::int64_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::U64) {
                            valbox res{der.as_typed_ptr<std::uint64_t>() + i};
                            return res;
                        } else if(der.pointed_type() == type::FLOAT) {
                            valbox res{der.as_typed_ptr<float>() + i};
                            return res;
                        } else if(der.pointed_type() == type::DOUBLE) {
                            valbox res{der.as_typed_ptr<double>() + i};
                            return res;
                        } else if(der.pointed_type() == type::LONG_DOUBLE) {
                            valbox res{der.as_typed_ptr<long double>() + i};
                            return res;
                        }
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        valbox object_key_at(uint64_t indx) {
            valbox &der{deref()};
            if(der.val_or_pointed_type() == type::OBJECT) {
                object_t &o{as_object()};
                if(o.size() > indx) {
                    std::size_t curr_indx{0};
                    for(auto &&kv: o) {
                        if(curr_indx == indx) {
                            return kv.first;
                        }
                        ++curr_indx;
                    }
                } else {
                    throw std::range_error{"index out of range"};
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        valbox object_value_at(uint64_t indx) {
            valbox &der{deref()};
            if(der.val_or_pointed_type() == type::OBJECT) {
                object_t &o{as_object()};
                if(o.size() > indx) {
                    std::size_t curr_indx{0};
                    for(auto &&kv: o) {
                        if(curr_indx == indx) {
                            return kv.second;
                        }
                        ++curr_indx;
                    }
                } else {
                    throw std::range_error{"index out of range"};
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        valbox &become_undefined() {
            valbox &thisref{deref()};
            if(!thisref.is_undefined()) {
                thisref.box_->type_ = type::UNDEFINED;
                thisref.box_->pointed_type_ = type::UNDEFINED;
                thisref.box_->class_.clear();
                thisref.box_->value_ = value_t{nullptr};
                thisref.box_->func_name_.clear();
                thisref.box_->user_func_ = false;
                thisref.pointed_box_.reset();
            }
            return *this;
        }

        valbox &become_same_type_as(valbox const &that) {
            valbox const &thatref{that.deref()};
            valbox &thisref{deref()};
            auto thatt{thatref.val_or_pointed_type()};
            auto thist{thisref.val_or_pointed_type()};
            if(thatt == thist) {
                return *this;
            }
            if(thisref.is_undefined()) {
                thisref.become_type(thatt);
                return *this;
            }
            switch(thatt) {
                case type::U64:         thisref.box_->value_ = thisref.cast_to_u64(); break;
                case type::S64:         thisref.box_->value_ = thisref.cast_to_s64(); break;
                case type::CHAR:        thisref.box_->value_ = thisref.cast_to_char(); break;
                case type::U8:          thisref.box_->value_ = thisref.cast_to_u8(); break;
                case type::S8:          thisref.box_->value_ = thisref.cast_to_s8(); break;
                case type::U16:         thisref.box_->value_ = thisref.cast_to_u16(); break;
                case type::S16:         thisref.box_->value_ = thisref.cast_to_s16(); break;
                case type::U32:         thisref.box_->value_ = thisref.cast_to_u32(); break;
                case type::S32:         thisref.box_->value_ = thisref.cast_to_s32(); break;
                case type::FLOAT:       thisref.box_->value_ = thisref.cast_to_float(); break;
                case type::DOUBLE:      thisref.box_->value_ = thisref.cast_to_double(); break;
                case type::LONG_DOUBLE: thisref.box_->value_ = thisref.cast_to_long_double(); break;
                case type::BOOL:        thisref.box_->value_ = thisref.cast_to_bool(); break;
                case type::WCHAR:       thisref.box_->value_ = thisref.cast_to_wchar(); break;
                case type::STRING:      thisref.box_->value_ = thisref.cast_to_string(); break;
                case type::WSTRING:     thisref.box_->value_ = thisref.cast_to_wstring(); break;
                case type::UNDEFINED:   thisref.undefine(); return *this;
                case type::ARRAY:       thisref.box_->value_ = thisref.cast_to_array(); break;
                case type::OBJECT:      thisref.box_->value_ = thisref.cast_to_object(); break;
                case type::VEC4:        thisref.box_->value_ = vec4_t{}; break;
                case type::MAT4:        thisref.box_->value_ = mat4_t{}; break;
                default: throw std::runtime_error{"assigning is needed to become a given type"};;
            }
            thisref.pointed_box_.reset();
            thisref.box_->type_ = thatt;
            thisref.box_->func_name_.clear();
            thisref.box_->user_func_ = false;
            return *this;
        }

        valbox &become_type(type t) {
            valbox &vref{deref()};
            if(t == vref.val_or_pointed_type()) {
                return *this;
            }
            if(!vref.box_) {
                switch(t) {
                    case type::U64:         vref.box_ = std::make_shared<box_data>((std::uint64_t)0, type::U64); break;
                    case type::S64:         vref.box_ = std::make_shared<box_data>((std::int64_t)0, type::S64); break;
                    case type::CHAR:        vref.box_ = std::make_shared<box_data>((char)0, type::CHAR); break;
                    case type::U8:          vref.box_ = std::make_shared<box_data>((std::uint8_t)0, type::U8); break;
                    case type::S8:          vref.box_ = std::make_shared<box_data>((std::int8_t)0, type::S8); break;
                    case type::U16:         vref.box_ = std::make_shared<box_data>((std::uint16_t)0, type::U16); break;
                    case type::S16:         vref.box_ = std::make_shared<box_data>((std::int16_t)0, type::S16); break;
                    case type::U32:         vref.box_ = std::make_shared<box_data>((std::uint32_t)0, type::U32); break;
                    case type::S32:         vref.box_ = std::make_shared<box_data>((std::int32_t)0, type::S32); break;
                    case type::FLOAT:       vref.box_ = std::make_shared<box_data>((float)0, type::FLOAT); break;
                    case type::DOUBLE:      vref.box_ = std::make_shared<box_data>((double)0, type::DOUBLE); break;
                    case type::LONG_DOUBLE: vref.box_ = std::make_shared<box_data>((long double)0, type::LONG_DOUBLE); break;
                    case type::BOOL:        vref.box_ = std::make_shared<box_data>(false, type::BOOL); break;
                    case type::WCHAR:       vref.box_ = std::make_shared<box_data>((wchar_t)0, type::WCHAR); break;
                    case type::STRING:      vref.box_ = std::make_shared<box_data>(std::string{}, type::STRING); break;
                    case type::WSTRING:     vref.box_ = std::make_shared<box_data>(std::wstring{}, type::WSTRING); break;
                    case type::ARRAY:       vref.box_ = std::make_shared<box_data>(array_t{}, type::ARRAY); break;
                    case type::OBJECT:      vref.box_ = std::make_shared<box_data>(object_t{}, type::OBJECT); break;
                    case type::VEC4:        vref.box_ = std::make_shared<box_data>(vec4_t{}, type::VEC4); break; break;
                    case type::MAT4:        vref.box_ = std::make_shared<box_data>(mat4_t{}, type::MAT4); break; break;
                    default: throw std::runtime_error{"assigning is needed to become a given type"};
                }
                vref.pointed_box_.reset();
                return *this;
            }
            switch(t) {
                case type::U64:         vref.box_->value_ = cast_to_u64(); break;
                case type::S64:         vref.box_->value_ = cast_to_s64(); break;
                case type::CHAR:        vref.box_->value_ = cast_to_char(); break;
                case type::U8:          vref.box_->value_ = cast_to_u8(); break;
                case type::S8:          vref.box_->value_ = cast_to_s8(); break;
                case type::U16:         vref.box_->value_ = cast_to_u16(); break;
                case type::S16:         vref.box_->value_ = cast_to_s16(); break;
                case type::U32:         vref.box_->value_ = cast_to_u32(); break;
                case type::S32:         vref.box_->value_ = cast_to_s32(); break;
                case type::FLOAT:       vref.box_->value_ = cast_to_float(); break;
                case type::DOUBLE:      vref.box_->value_ = cast_to_double(); break;
                case type::LONG_DOUBLE: vref.box_->value_ = cast_to_long_double(); break;
                case type::BOOL:        vref.box_->value_ = cast_to_bool(); break;
                case type::WCHAR:       vref.box_->value_ = cast_to_wchar(); break;
                case type::STRING:      vref.box_->value_ = cast_to_string(); break;
                case type::WSTRING:     vref.box_->value_ = cast_to_wstring(); break;
                case type::UNDEFINED:   vref.undefine(); return *this;
                case type::ARRAY:       vref.box_->value_ = cast_to_array(); break;
                case type::OBJECT:      vref.box_->value_ = cast_to_object(); break;
                case type::VEC4:        vref.box_->value_ = vec4_t(); break;
                case type::MAT4:        vref.box_->value_ = mat4_t(); break;
                default: throw std::runtime_error{"assigning is needed to become a given type"};
            }
            vref.pointed_box_.reset();
            vref.box_->type_ = t;
            return *this;
        }

        template<typename T>
        T cast_num_to_num() const {
            static std::array<std::function<T(valbox const *)>, 25> const funcs{
                [](valbox const *p) -> T { return p->as_bool() ? 1 : 0; },
                [](valbox const *p) -> T { return p->as_char(); },
                [](valbox const *p) -> T { return p->as_s8(); },
                [](valbox const *p) -> T { return p->as_u8(); },
                [](valbox const *p) -> T { return p->as_s16(); },
                [](valbox const *p) -> T { return p->as_u16(); },
                [](valbox const *p) -> T { return p->as_wchar(); },
                [](valbox const *p) -> T { return p->as_s32(); },
                [](valbox const *p) -> T { return p->as_u32(); },
                [](valbox const *p) -> T { return p->as_s64(); },
                [](valbox const *p) -> T { return p->as_u64(); },
                [](valbox const *p) -> T { return p->as_float(); },
                [](valbox const *p) -> T { return p->as_double(); },
                [](valbox const *p) -> T { return p->as_long_double(); },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
                [](valbox const * ) -> T { return T{}; },
            };
            return funcs[static_cast<int>(val_or_pointed_type())](this);
        }

        valbox operator-() const {
            type t{val_or_pointed_type()};
            switch(t) {
                case type::LONG_DOUBLE: return -as_long_double();
                case type::DOUBLE: return -as_double();
                case type::FLOAT: return -as_float();
                case type::CHAR: return -as_char();
                case type::WCHAR: return -as_wchar();
                case type::S8: return -as_s8();
                case type::U8: return -as_u8();
                case type::S16: return -as_s16();
                case type::U16: return -as_u16();
                case type::S32: return -as_s32();
                case type::U32: return -as_u32();
                case type::S64: return -as_s64();
                case type::U64: return -as_u64();
                case type::VEC4: return -as_vec4();
                case type::MAT4: return -as_mat4();
                case type::UNDEFINED: return valbox{};
                default: throw std::runtime_error{"operation not applicable"};
            }
        }

        valbox &operator++() {
            type t{val_or_pointed_type()};
            switch(t) {
                case type::S64: ++as_s64(); break;
                case type::U64: ++as_u64(); break;
                case type::S32: ++as_s32(); break;
                case type::U32: ++as_u32(); break;
                case type::S8: ++as_s8(); break;
                case type::U8: ++as_u8(); break;
                case type::S16: ++as_s16(); break;
                case type::U16: ++as_u16(); break;
                case type::CHAR: ++as_char(); break;
                case type::WCHAR: ++as_wchar(); break;
                case type::BOOL: as_bool() = true; break;
                case type::UNDEFINED: {
                    valbox &dr{deref()};
                    if(!dr.box_) {
                        dr.box_ = std::make_shared<box_data>(static_cast<std::int64_t>(1), type::S64);
                    } else {
                        dr.box_->value_ = static_cast<std::int64_t>(1);
                        dr.box_->type_ = type::S64;
                    }
                    break;
                }
                default:
                    throw std::runtime_error{"operation not applicable"};
            }
            return *this;
        }

        valbox operator++(int) {
            type t{val_or_pointed_type()};
            switch(t) {
                case type::S64: { valbox res{as_s64()}; ++as_s64(); return res; }
                case type::U64: { valbox res{as_u64()}; ++as_u64(); return res; }
                case type::S32: { valbox res{as_s32()}; ++as_s32(); return res; }
                case type::U32: { valbox res{as_u32()}; ++as_u32(); return res; }
                case type::S8: { valbox res{as_s8()}; ++as_s8(); return res; }
                case type::U8: { valbox res{as_u8()}; ++as_u8(); return res; }
                case type::S16: { valbox res{as_s16()}; ++as_s16(); return res; }
                case type::U16: { valbox res{as_u16()}; ++as_u16(); return res; }
                case type::CHAR: { valbox res{as_char()}; ++as_char(); return res; }
                case type::WCHAR: { valbox res{as_wchar()}; ++as_wchar(); return res; }
                case type::BOOL: { valbox res{as_bool()}; as_bool() = true; return res; }
                case type::UNDEFINED: {
                    valbox res{static_cast<std::int64_t>(0)};
                    valbox &dr{deref()};
                    dr.pointed_box_.reset();
                    if(!dr.box_) {
                        dr.box_ = std::make_shared<box_data>(static_cast<std::int64_t>(1), type::S64);
                    } else {
                        dr.box_->value_ = static_cast<std::int64_t>(1);
                        dr.box_->type_ = type::S64;
                    }
                    return res;
                }
                default:
                    throw std::runtime_error{"operation not applicable"};
            }
        }

        valbox &operator--() {
            type t{val_or_pointed_type()};
            switch(t) {
                case type::S64: --as_s64(); break;
                case type::U64: --as_u64(); break;
                case type::S32: --as_s32(); break;
                case type::U32: --as_u32(); break;
                case type::S8: --as_s8(); break;
                case type::U8: --as_u8(); break;
                case type::S16: --as_s16(); break;
                case type::U16: --as_u16(); break;
                case type::CHAR: --as_char(); break;
                case type::WCHAR: --as_wchar(); break;
                case type::BOOL: as_bool() = false; break;
                case type::UNDEFINED: {
                    auto dr{deref()};
                    dr.pointed_box_.reset();
                    if(!dr.box_) {
                        dr.box_ = std::make_shared<box_data>(static_cast<std::int64_t>(-1), type::S64);
                    } else {
                        dr.box_->value_ = static_cast<std::int64_t>(-1);
                        dr.box_->type_ = type::S64;
                    }
                    break;
                }
                default:
                    throw std::runtime_error{"operation not applicable"};
            }
            return *this;
        }

        valbox operator--(int) {
            type t{val_or_pointed_type()};
            switch(t) {
                case type::S64: { valbox res{as_s64()}; --as_s64(); return res; }
                case type::U64: { valbox res{as_u64()}; --as_u64(); return res; }
                case type::S32: { valbox res{as_s32()}; --as_s32(); return res; }
                case type::U32: { valbox res{as_u32()}; --as_u32(); return res; }
                case type::S8: { valbox res{as_s8()}; --as_s8(); return res; }
                case type::U8: { valbox res{as_u8()}; --as_u8(); return res; }
                case type::S16: { valbox res{as_s16()}; --as_s16(); return res; }
                case type::U16: { valbox res{as_u16()}; --as_u16(); return res; }
                case type::CHAR: { valbox res{as_char()}; --as_char(); return res; }
                case type::WCHAR: { valbox res{as_wchar()}; --as_wchar(); return res; }
                case type::BOOL: { valbox res{as_bool()}; as_bool() = false; return res; }
                case type::UNDEFINED: {
                    valbox res{static_cast<std::int64_t>(0)};
                    valbox &dr{deref()};
                    dr.pointed_box_.reset();
                    if(!dr.box_) {
                        dr.box_ = std::make_shared<box_data>(static_cast<std::int64_t>(-1), type::S64);
                    } else {
                        dr.box_->value_ = static_cast<std::int64_t>(-1);
                        dr.box_->type_ = type::S64;
                    }
                    return res;
                }
                default:
                    throw std::runtime_error{"operation not applicable"};
            }
        }

        valbox operator~() const {
            auto t{val_or_pointed_type()};
            switch(t) {
                case type::CHAR: return (char)~as_char();
                case type::WCHAR: return (wchar_t)~as_wchar();
                case type::BOOL: return !as_bool();
                case type::S8: return (std::int8_t)~as_s8();
                case type::U8: return (std::uint8_t)~as_u8();
                case type::S16: return (std::int16_t)~as_s16();
                case type::U16: return (std::uint16_t)~as_u16();
                case type::S32: return (std::int32_t)~as_s32();
                case type::U32: return (std::uint32_t)~as_u32();
                case type::S64: return (std::int64_t)~as_s64();
                case type::U64: return (std::uint64_t)~as_u64();
                default: throw std::runtime_error{"operation not applicable"};
            }
        }

        valbox &operator+() {
            return *this;
        }

        static bool stronger_numeric_type(type t1, type t2, type &res) {
            int it1{static_cast<int>(t1)};
            int it2{static_cast<int>(t2)};
            if(
                it1 >= static_cast<int>(type::BOOL) && it2 >= static_cast<int>(type::BOOL)
                &&
                it1 <= static_cast<int>(type::LONG_DOUBLE) && it2 <= static_cast<int>(type::LONG_DOUBLE)
            ) {
                res = it1 >= it2 ? t1 : t2;
                return true;
            }
            return false;
        }

        static type stronger_type(type t1, type t2) {
            return static_cast<int>(t1) > static_cast<int>(t2) ? t1 : t2;
        }

        valbox &operator+=(valbox const &other) {
            valbox &thisref{deref()};
            valbox const &thatref{other.deref()};
            auto thist{thisref.val_or_pointed_type()};
            auto thatt{thatref.val_or_pointed_type()};
            switch(thist) {
                case type::BOOL:
                    switch(thatt) {
                        case type::BOOL: as_bool() += thatref.as_bool(); break;
                        case type::CHAR: as_bool() += thatref.as_char(); break;
                        case type::S8: as_bool() += thatref.as_s8(); break;
                        case type::U8: as_bool() += thatref.as_u8(); break;
                        case type::S16: as_bool() += thatref.as_s16(); break;
                        case type::U16: as_bool() += thatref.as_u16(); break;
                        case type::WCHAR: as_bool() += thatref.as_wchar(); break;
                        case type::S32: as_bool() += thatref.as_s32(); break;
                        case type::U32: as_bool() += thatref.as_u32(); break;
                        case type::S64: as_bool() += thatref.as_s64(); break;
                        case type::U64: as_bool() += thatref.as_u64(); break;
                        case type::FLOAT: as_bool() += thatref.as_float(); break;
                        case type::DOUBLE: as_bool() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_bool() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::CHAR:
                    switch(thatt) {
                        case type::BOOL: as_char() += thatref.as_bool(); break;
                        case type::CHAR: as_char() += thatref.as_char(); break;
                        case type::S8: as_char() += thatref.as_s8(); break;
                        case type::U8: as_char() += thatref.as_u8(); break;
                        case type::S16: as_char() += thatref.as_s16(); break;
                        case type::U16: as_char() += thatref.as_u16(); break;
                        case type::WCHAR: as_char() += thatref.as_wchar(); break;
                        case type::S32: as_char() += thatref.as_s32(); break;
                        case type::U32: as_char() += thatref.as_u32(); break;
                        case type::S64: as_char() += thatref.as_s64(); break;
                        case type::U64: as_char() += thatref.as_u64(); break;
                        case type::FLOAT: as_char() += thatref.as_float(); break;
                        case type::DOUBLE: as_char() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_char() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::S8:
                    switch(thatt) {
                        case type::BOOL: as_s8() += thatref.as_bool(); break;
                        case type::CHAR: as_s8() += thatref.as_char(); break;
                        case type::S8: as_s8() += thatref.as_s8(); break;
                        case type::U8: as_s8() += thatref.as_u8(); break;
                        case type::S16: as_s8() += thatref.as_s16(); break;
                        case type::U16: as_s8() += thatref.as_u16(); break;
                        case type::WCHAR: as_s8() += thatref.as_wchar(); break;
                        case type::S32: as_s8() += thatref.as_s32(); break;
                        case type::U32: as_s8() += thatref.as_u32(); break;
                        case type::S64: as_s8() += thatref.as_s64(); break;
                        case type::U64: as_s8() += thatref.as_u64(); break;
                        case type::FLOAT: as_s8() += thatref.as_float(); break;
                        case type::DOUBLE: as_s8() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s8() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::U8:
                    switch(thatt) {
                        case type::BOOL: as_u8() += thatref.as_bool(); break;
                        case type::CHAR: as_u8() += thatref.as_char(); break;
                        case type::S8: as_u8() += thatref.as_s8(); break;
                        case type::U8: as_u8() += thatref.as_u8(); break;
                        case type::S16: as_u8() += thatref.as_s16(); break;
                        case type::U16: as_u8() += thatref.as_u16(); break;
                        case type::WCHAR: as_u8() += thatref.as_wchar(); break;
                        case type::S32: as_u8() += thatref.as_s32(); break;
                        case type::U32: as_u8() += thatref.as_u32(); break;
                        case type::S64: as_u8() += thatref.as_s64(); break;
                        case type::U64: as_u8() += thatref.as_u64(); break;
                        case type::FLOAT: as_u8() += thatref.as_float(); break;
                        case type::DOUBLE: as_u8() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u8() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::S16:
                    switch(thatt) {
                        case type::BOOL: as_s16() += thatref.as_bool(); break;
                        case type::CHAR: as_s16() += thatref.as_char(); break;
                        case type::S8: as_s16() += thatref.as_s8(); break;
                        case type::U8: as_s16() += thatref.as_u8(); break;
                        case type::S16: as_s16() += thatref.as_s16(); break;
                        case type::U16: as_s16() += thatref.as_u16(); break;
                        case type::WCHAR: as_s16() += thatref.as_wchar(); break;
                        case type::S32: as_s16() += thatref.as_s32(); break;
                        case type::U32: as_s16() += thatref.as_u32(); break;
                        case type::S64: as_s16() += thatref.as_s64(); break;
                        case type::U64: as_s16() += thatref.as_u64(); break;
                        case type::FLOAT: as_s16() += thatref.as_float(); break;
                        case type::DOUBLE: as_s16() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s16() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::U16:
                    switch(thatt) {
                        case type::BOOL: as_u16() += thatref.as_bool(); break;
                        case type::CHAR: as_u16() += thatref.as_char(); break;
                        case type::S8: as_u16() += thatref.as_s8(); break;
                        case type::U8: as_u16() += thatref.as_u8(); break;
                        case type::S16: as_u16() += thatref.as_s16(); break;
                        case type::U16: as_u16() += thatref.as_u16(); break;
                        case type::WCHAR: as_u16() += thatref.as_wchar(); break;
                        case type::S32: as_u16() += thatref.as_s32(); break;
                        case type::U32: as_u16() += thatref.as_u32(); break;
                        case type::S64: as_u16() += thatref.as_s64(); break;
                        case type::U64: as_u16() += thatref.as_u64(); break;
                        case type::FLOAT: as_u16() += thatref.as_float(); break;
                        case type::DOUBLE: as_u16() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u16() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::WCHAR:
                    switch(thatt) {
                        case type::BOOL: as_wchar() += thatref.as_bool(); break;
                        case type::CHAR: as_wchar() += thatref.as_char(); break;
                        case type::S8: as_wchar() += thatref.as_s8(); break;
                        case type::U8: as_wchar() += thatref.as_u8(); break;
                        case type::S16: as_wchar() += thatref.as_s16(); break;
                        case type::U16: as_wchar() += thatref.as_u16(); break;
                        case type::WCHAR: as_wchar() += thatref.as_wchar(); break;
                        case type::S32: as_wchar() += thatref.as_s32(); break;
                        case type::U32: as_wchar() += thatref.as_u32(); break;
                        case type::S64: as_wchar() += thatref.as_s64(); break;
                        case type::U64: as_wchar() += thatref.as_u64(); break;
                        case type::FLOAT: as_wchar() += thatref.as_float(); break;
                        case type::DOUBLE: as_wchar() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_wchar() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::S32:
                    switch(thatt) {
                        case type::BOOL: as_s32() += thatref.as_bool(); break;
                        case type::CHAR: as_s32() += thatref.as_char(); break;
                        case type::S8: as_s32() += thatref.as_s8(); break;
                        case type::U8: as_s32() += thatref.as_u8(); break;
                        case type::S16: as_s32() += thatref.as_s16(); break;
                        case type::U16: as_s32() += thatref.as_u16(); break;
                        case type::WCHAR: as_s32() += thatref.as_wchar(); break;
                        case type::S32: as_s32() += thatref.as_s32(); break;
                        case type::U32: as_s32() += thatref.as_u32(); break;
                        case type::S64: as_s32() += thatref.as_s64(); break;
                        case type::U64: as_s32() += thatref.as_u64(); break;
                        case type::FLOAT: as_s32() += thatref.as_float(); break;
                        case type::DOUBLE: as_s32() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s32() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::U32:
                    switch(thatt) {
                        case type::BOOL: as_u32() += thatref.as_bool(); break;
                        case type::CHAR: as_u32() += thatref.as_char(); break;
                        case type::S8: as_u32() += thatref.as_s8(); break;
                        case type::U8: as_u32() += thatref.as_u8(); break;
                        case type::S16: as_u32() += thatref.as_s16(); break;
                        case type::U16: as_u32() += thatref.as_u16(); break;
                        case type::WCHAR: as_u32() += thatref.as_wchar(); break;
                        case type::S32: as_u32() += thatref.as_s32(); break;
                        case type::U32: as_u32() += thatref.as_u32(); break;
                        case type::S64: as_u32() += thatref.as_s64(); break;
                        case type::U64: as_u32() += thatref.as_u64(); break;
                        case type::FLOAT: as_u32() += thatref.as_float(); break;
                        case type::DOUBLE: as_u32() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u32() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::S64:
                    switch(thatt) {
                        case type::BOOL: as_s64() += thatref.as_bool(); break;
                        case type::CHAR: as_s64() += thatref.as_char(); break;
                        case type::S8: as_s64() += thatref.as_s8(); break;
                        case type::U8: as_s64() += thatref.as_u8(); break;
                        case type::S16: as_s64() += thatref.as_s16(); break;
                        case type::U16: as_s64() += thatref.as_u16(); break;
                        case type::WCHAR: as_s64() += thatref.as_wchar(); break;
                        case type::S32: as_s64() += thatref.as_s32(); break;
                        case type::U32: as_s64() += thatref.as_u32(); break;
                        case type::S64: as_s64() += thatref.as_s64(); break;
                        case type::U64: as_s64() += thatref.as_u64(); break;
                        case type::FLOAT: as_s64() += thatref.as_float(); break;
                        case type::DOUBLE: as_s64() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s64() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::U64:
                    switch(thatt) {
                        case type::BOOL: as_u64() += thatref.as_bool(); break;
                        case type::CHAR: as_u64() += thatref.as_char(); break;
                        case type::S8: as_u64() += thatref.as_s8(); break;
                        case type::U8: as_u64() += thatref.as_u8(); break;
                        case type::S16: as_u64() += thatref.as_s16(); break;
                        case type::U16: as_u64() += thatref.as_u16(); break;
                        case type::WCHAR: as_u64() += thatref.as_wchar(); break;
                        case type::S32: as_u64() += thatref.as_s32(); break;
                        case type::U32: as_u64() += thatref.as_u32(); break;
                        case type::S64: as_u64() += thatref.as_s64(); break;
                        case type::U64: as_u64() += thatref.as_u64(); break;
                        case type::FLOAT: as_u64() += thatref.as_float(); break;
                        case type::DOUBLE: as_u64() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u64() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::FLOAT:
                    switch(thatt) {
                        case type::BOOL: as_float() += thatref.as_bool(); break;
                        case type::CHAR: as_float() += thatref.as_char(); break;
                        case type::S8: as_float() += thatref.as_s8(); break;
                        case type::U8: as_float() += thatref.as_u8(); break;
                        case type::S16: as_float() += thatref.as_s16(); break;
                        case type::U16: as_float() += thatref.as_u16(); break;
                        case type::WCHAR: as_float() += thatref.as_wchar(); break;
                        case type::S32: as_float() += thatref.as_s32(); break;
                        case type::U32: as_float() += thatref.as_u32(); break;
                        case type::S64: as_float() += thatref.as_s64(); break;
                        case type::U64: as_float() += thatref.as_u64(); break;
                        case type::FLOAT: as_float() += thatref.as_float(); break;
                        case type::DOUBLE: as_float() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_float() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::DOUBLE:
                    switch(thatt) {
                        case type::BOOL: as_double() += thatref.as_bool(); break;
                        case type::CHAR: as_double() += thatref.as_char(); break;
                        case type::S8: as_double() += thatref.as_s8(); break;
                        case type::U8: as_double() += thatref.as_u8(); break;
                        case type::S16: as_double() += thatref.as_s16(); break;
                        case type::U16: as_double() += thatref.as_u16(); break;
                        case type::WCHAR: as_double() += thatref.as_wchar(); break;
                        case type::S32: as_double() += thatref.as_s32(); break;
                        case type::U32: as_double() += thatref.as_u32(); break;
                        case type::S64: as_double() += thatref.as_s64(); break;
                        case type::U64: as_double() += thatref.as_u64(); break;
                        case type::FLOAT: as_double() += thatref.as_float(); break;
                        case type::DOUBLE: as_double() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_double() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(thatt) {
                        case type::BOOL: as_long_double() += thatref.as_bool(); break;
                        case type::CHAR: as_long_double() += thatref.as_char(); break;
                        case type::S8: as_long_double() += thatref.as_s8(); break;
                        case type::U8: as_long_double() += thatref.as_u8(); break;
                        case type::S16: as_long_double() += thatref.as_s16(); break;
                        case type::U16: as_long_double() += thatref.as_u16(); break;
                        case type::WCHAR: as_long_double() += thatref.as_wchar(); break;
                        case type::S32: as_long_double() += thatref.as_s32(); break;
                        case type::U32: as_long_double() += thatref.as_u32(); break;
                        case type::S64: as_long_double() += thatref.as_s64(); break;
                        case type::U64: as_long_double() += thatref.as_u64(); break;
                        case type::FLOAT: as_long_double() += thatref.as_float(); break;
                        case type::DOUBLE: as_long_double() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_long_double() += thatref.as_long_double(); break;
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::VEC4:
                    switch(thatt) {
                        case type::BOOL: as_vec4() += thatref.as_bool(); break;
                        case type::CHAR: as_vec4() += thatref.as_char(); break;
                        case type::S8: as_vec4() += thatref.as_s8(); break;
                        case type::U8: as_vec4() += thatref.as_u8(); break;
                        case type::S16: as_vec4() += thatref.as_s16(); break;
                        case type::U16: as_vec4() += thatref.as_u16(); break;
                        case type::WCHAR: as_vec4() += thatref.as_wchar(); break;
                        case type::S32: as_vec4() += thatref.as_s32(); break;
                        case type::U32: as_vec4() += thatref.as_u32(); break;
                        case type::S64: as_vec4() += thatref.as_s64(); break;
                        case type::U64: as_vec4() += thatref.as_u64(); break;
                        case type::FLOAT: as_vec4() += thatref.as_float(); break;
                        case type::DOUBLE: as_vec4() += thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_vec4() += thatref.as_long_double(); break;
                        case type::VEC4: as_vec4() += thatref.as_vec4(); break;
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::MAT4:
                    switch(thatt) {
                        case type::BOOL: throw std::runtime_error{"operation not applicable"};
                        case type::CHAR: throw std::runtime_error{"operation not applicable"};
                        case type::S8: throw std::runtime_error{"operation not applicable"};
                        case type::U8: throw std::runtime_error{"operation not applicable"};
                        case type::S16: throw std::runtime_error{"operation not applicable"};
                        case type::U16: throw std::runtime_error{"operation not applicable"};
                        case type::WCHAR: throw std::runtime_error{"operation not applicable"};
                        case type::S32: throw std::runtime_error{"operation not applicable"};
                        case type::U32: throw std::runtime_error{"operation not applicable"};
                        case type::S64: throw std::runtime_error{"operation not applicable"};
                        case type::U64: throw std::runtime_error{"operation not applicable"};
                        case type::FLOAT: throw std::runtime_error{"operation not applicable"};
                        case type::DOUBLE: throw std::runtime_error{"operation not applicable"};
                        case type::LONG_DOUBLE: throw std::runtime_error{"operation not applicable"};
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: as_mat4() += thatref.as_mat4(); break;
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::POINTER: throw std::runtime_error{"operation not applicable"};
                    break;
                case type::CLASS: throw std::runtime_error{"operation not applicable"};
                    break;
                case type::FUNC: throw std::runtime_error{"operation not applicable"};
                    break;
                case type::ARRAY:
                    switch(thatt) {
                        case type::BOOL:
                        case type::CHAR:
                        case type::S8:
                        case type::U8:
                        case type::S16:
                        case type::U16:
                        case type::WCHAR:
                        case type::S32:
                        case type::U32:
                        case type::S64:
                        case type::U64:
                        case type::FLOAT:
                        case type::DOUBLE:
                        case type::LONG_DOUBLE:
                        case type::VEC4:
                        case type::MAT4:
                        case type::POINTER:
                        case type::CLASS:
                        case type::FUNC:
                        case type::OBJECT:
                        case type::STRING:
                        case type::WSTRING:
                        case type::UNDEFINED:
                        case type::VALBOX:
                            as_array().push_back(thatref.clone());
                            break;
                        case type::ARRAY:
                            for(auto &&v: thatref.as_array()) {
                                as_array().push_back(v.clone());
                            }
                            break;
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::OBJECT:
                    switch(thatt) {
                        case type::BOOL: throw std::runtime_error{"operation not applicable"};
                        case type::CHAR: throw std::runtime_error{"operation not applicable"};
                        case type::S8: throw std::runtime_error{"operation not applicable"};
                        case type::U8: throw std::runtime_error{"operation not applicable"};
                        case type::S16: throw std::runtime_error{"operation not applicable"};
                        case type::U16: throw std::runtime_error{"operation not applicable"};
                        case type::WCHAR: throw std::runtime_error{"operation not applicable"};
                        case type::S32: throw std::runtime_error{"operation not applicable"};
                        case type::U32: throw std::runtime_error{"operation not applicable"};
                        case type::S64: throw std::runtime_error{"operation not applicable"};
                        case type::U64: throw std::runtime_error{"operation not applicable"};
                        case type::FLOAT: throw std::runtime_error{"operation not applicable"};
                        case type::DOUBLE: throw std::runtime_error{"operation not applicable"};
                        case type::LONG_DOUBLE: throw std::runtime_error{"operation not applicable"};
                        case type::VEC4: throw std::runtime_error{"operation not applicable"};
                        case type::MAT4: throw std::runtime_error{"operation not applicable"};
                        case type::POINTER: throw std::runtime_error{"operation not applicable"};
                        case type::CLASS: throw std::runtime_error{"operation not applicable"};
                        case type::FUNC: throw std::runtime_error{"operation not applicable"};
                        case type::STRING: throw std::runtime_error{"operation not applicable"};
                        case type::WSTRING: throw std::runtime_error{"operation not applicable"};
                        case type::UNDEFINED: break;
                        case type::VALBOX: throw std::runtime_error{"operation not applicable"};
                        case type::ARRAY: throw std::runtime_error{"operation not applicable"};
                        case type::OBJECT: {
                            for(auto &&p: thatref.as_object()) {
                                as_object()[p.first] = p.second.clone();
                            }
                            break;
                        }
                        default: throw std::runtime_error{"operation not applicable"};
                    }
                    break;
                case type::STRING:
                    if(thatt != type::UNDEFINED) {
                        as_string() += thatref.cast_to_string();
                    }
                    break;
                case type::WSTRING:
                    if(thatt != type::UNDEFINED) {
                        as_wstring() += thatref.cast_to_wstring();
                    }
                    break;
                case type::UNDEFINED:
                    if(thatt != type::UNDEFINED) {
                        assign(thatref);
                    }
                    break;
                case type::VALBOX:
                    throw std::runtime_error{"operation not applicable"};
                default:
                    throw std::runtime_error{"operation not applicable"};
            }
            return *this;
        }

        friend valbox operator+(valbox const &larg, valbox const &rarg) {
            valbox const &lr{larg.deref()};
            valbox const &rr{rarg.deref()};
            auto lt{lr.val_or_pointed_type()};
            auto rt{rr.val_or_pointed_type()};
            switch(lt) {
                case type::BOOL:
                    switch(rt) {
                        case type::BOOL: return (bool)(lr.as_bool() + rr.as_bool());
                        case type::CHAR: return lr.as_bool() + rr.as_char();
                        case type::S8: return lr.as_bool() + rr.as_s8();
                        case type::U8: return lr.as_bool() + rr.as_u8();
                        case type::S16: return lr.as_bool() + rr.as_s16();
                        case type::U16: return lr.as_bool() + rr.as_u16();
                        case type::WCHAR: return lr.as_bool() + rr.as_wchar();
                        case type::S32: return lr.as_bool() + rr.as_s32();
                        case type::U32: return lr.as_bool() + rr.as_u32();
                        case type::S64: return lr.as_bool() + rr.as_s64();
                        case type::U64: return lr.as_bool() + rr.as_u64();
                        case type::FLOAT: return lr.as_bool() + rr.as_float();
                        case type::DOUBLE: return lr.as_bool() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_bool() + rr.as_long_double();
                        case type::VEC4: return lr.as_bool() + rr.as_vec4();
                        case type::MAT4: return lr.as_bool() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: { auto res{rr.clone()}; res.as_array().push_front(lr.as_bool()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return (lr.as_bool() ? std::string{"true"} : std::string{"false"}) + rr.as_string();
                        case type::WSTRING: return (lr.as_bool() ? std::wstring{L"true"} : std::wstring{L"false"}) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_bool();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::CHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_char() + rr.as_bool();
                        case type::CHAR: return lr.as_char() + rr.as_char();
                        case type::S8: return lr.as_char() + rr.as_s8();
                        case type::U8: return lr.as_char() + rr.as_u8();
                        case type::S16: return lr.as_char() + rr.as_s16();
                        case type::U16: return lr.as_char() + rr.as_u16();
                        case type::WCHAR: return lr.as_char() + rr.as_wchar();
                        case type::S32: return lr.as_char() + rr.as_s32();
                        case type::U32: return lr.as_char() + rr.as_u32();
                        case type::S64: return lr.as_char() + rr.as_s64();
                        case type::U64: return lr.as_char() + rr.as_u64();
                        case type::FLOAT: return lr.as_char() + rr.as_float();
                        case type::DOUBLE: return lr.as_char() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_char() + rr.as_long_double();
                        case type::VEC4: return lr.as_char() + rr.as_vec4();
                        case type::MAT4: return lr.as_char() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_char()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return std::string{} + lr.as_char() + rr.as_string();
                        case type::WSTRING: return std::wstring{} + (wchar_t)lr.as_char() + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_char();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S8:
                    switch(rt) {
                        case type::BOOL: return lr.as_s8() + rr.as_bool();
                        case type::CHAR: return lr.as_s8() + rr.as_char();
                        case type::S8: return lr.as_s8() + rr.as_s8();
                        case type::U8: return lr.as_s8() + rr.as_u8();
                        case type::S16: return lr.as_s8() + rr.as_s16();
                        case type::U16: return lr.as_s8() + rr.as_u16();
                        case type::WCHAR: return lr.as_s8() + rr.as_wchar();
                        case type::S32: return lr.as_s8() + rr.as_s32();
                        case type::U32: return lr.as_s8() + rr.as_u32();
                        case type::S64: return lr.as_s8() + rr.as_s64();
                        case type::U64: return lr.as_s8() + rr.as_u64();
                        case type::FLOAT: return lr.as_s8() + rr.as_float();
                        case type::DOUBLE: return lr.as_s8() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s8() + rr.as_long_double();
                        case type::VEC4: return lr.as_s8() + rr.as_vec4();
                        case type::MAT4: return lr.as_s8() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_s8()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::itoa<std::string>(lr.as_s8()) + rr.as_string();
                        case type::WSTRING: return str_util::itoa<std::wstring>(lr.as_s8()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_s8();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U8:
                    switch(rt) {
                        case type::BOOL: return lr.as_u8() + rr.as_bool();
                        case type::CHAR: return lr.as_u8() + rr.as_char();
                        case type::S8: return lr.as_u8() + rr.as_s8();
                        case type::U8: return lr.as_u8() + rr.as_u8();
                        case type::S16: return lr.as_u8() + rr.as_s16();
                        case type::U16: return lr.as_u8() + rr.as_u16();
                        case type::WCHAR: return lr.as_u8() + rr.as_wchar();
                        case type::S32: return lr.as_u8() + rr.as_s32();
                        case type::U32: return lr.as_u8() + rr.as_u32();
                        case type::S64: return lr.as_u8() + rr.as_s64();
                        case type::U64: return lr.as_u8() + rr.as_u64();
                        case type::FLOAT: return lr.as_u8() + rr.as_float();
                        case type::DOUBLE: return lr.as_u8() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u8() + rr.as_long_double();
                        case type::VEC4: return lr.as_u8() + rr.as_vec4();
                        case type::MAT4: return lr.as_u8() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_u8()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::utoa<std::string>(lr.as_u8()) + rr.as_string();
                        case type::WSTRING: return str_util::utoa<std::wstring>(lr.as_u8()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_u8();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S16:
                    switch(rt) {
                        case type::BOOL: return lr.as_s16() + rr.as_bool();
                        case type::CHAR: return lr.as_s16() + rr.as_char();
                        case type::S8: return lr.as_s16() + rr.as_s8();
                        case type::U8: return lr.as_s16() + rr.as_u8();
                        case type::S16: return lr.as_s16() + rr.as_s16();
                        case type::U16: return lr.as_s16() + rr.as_u16();
                        case type::WCHAR: return lr.as_s16() + rr.as_wchar();
                        case type::S32: return lr.as_s16() + rr.as_s32();
                        case type::U32: return lr.as_s16() + rr.as_u32();
                        case type::S64: return lr.as_s16() + rr.as_s64();
                        case type::U64: return lr.as_s16() + rr.as_u64();
                        case type::FLOAT: return lr.as_s16() + rr.as_float();
                        case type::DOUBLE: return lr.as_s16() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s16() + rr.as_long_double();
                        case type::VEC4: return lr.as_s16() + rr.as_vec4();
                        case type::MAT4: return lr.as_s16() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_s16()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::itoa<std::string>(lr.as_s16()) + rr.as_string();
                        case type::WSTRING: return str_util::itoa<std::wstring>(lr.as_s16()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_s16();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(rt) {
                        case type::BOOL: return lr.as_u16() + rr.as_bool();
                        case type::CHAR: return lr.as_u16() + rr.as_char();
                        case type::S8: return lr.as_u16() + rr.as_s8();
                        case type::U8: return lr.as_u16() + rr.as_u8();
                        case type::S16: return lr.as_u16() + rr.as_s16();
                        case type::U16: return lr.as_u16() + rr.as_u16();
                        case type::WCHAR: return lr.as_u16() + rr.as_wchar();
                        case type::S32: return lr.as_u16() + rr.as_s32();
                        case type::U32: return lr.as_u16() + rr.as_u32();
                        case type::S64: return lr.as_u16() + rr.as_s64();
                        case type::U64: return lr.as_u16() + rr.as_u64();
                        case type::FLOAT: return lr.as_u16() + rr.as_float();
                        case type::DOUBLE: return lr.as_u16() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u16() + rr.as_long_double();
                        case type::VEC4: return lr.as_u16() + rr.as_vec4();
                        case type::MAT4: return lr.as_u16() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_u16()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::utoa<std::string>(lr.as_u16()) + rr.as_string();
                        case type::WSTRING: return str_util::utoa<std::wstring>(lr.as_u16()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_u16();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_wchar() + rr.as_bool();
                        case type::CHAR: return lr.as_wchar() + rr.as_char();
                        case type::S8: return lr.as_wchar() + rr.as_s8();
                        case type::U8: return lr.as_wchar() + rr.as_u8();
                        case type::S16: return lr.as_wchar() + rr.as_s16();
                        case type::U16: return lr.as_wchar() + rr.as_u16();
                        case type::WCHAR: return lr.as_wchar() + rr.as_wchar();
                        case type::S32: return lr.as_wchar() + rr.as_s32();
                        case type::U32: return lr.as_wchar() + rr.as_u32();
                        case type::S64: return lr.as_wchar() + rr.as_s64();
                        case type::U64: return lr.as_wchar() + rr.as_u64();
                        case type::FLOAT: return lr.as_wchar() + rr.as_float();
                        case type::DOUBLE: return lr.as_wchar() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_wchar() + rr.as_long_double();
                        case type::VEC4: return lr.as_wchar() + rr.as_vec4();
                        case type::MAT4: return lr.as_wchar() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_char()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return std::string{} + str_util::ucs_to_utf8(lr.as_wchar()) + rr.as_string();
                        case type::WSTRING: return std::wstring{} + lr.as_wchar() + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_wchar();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(rt) {
                        case type::BOOL: return lr.as_s32() + rr.as_bool();
                        case type::CHAR: return lr.as_s32() + rr.as_char();
                        case type::S8: return lr.as_s32() + rr.as_s8();
                        case type::U8: return lr.as_s32() + rr.as_u8();
                        case type::S16: return lr.as_s32() + rr.as_s16();
                        case type::U16: return lr.as_s32() + rr.as_u16();
                        case type::WCHAR: return lr.as_s32() + rr.as_wchar();
                        case type::S32: return lr.as_s32() + rr.as_s32();
                        case type::U32: return lr.as_s32() + rr.as_u32();
                        case type::S64: return lr.as_s32() + rr.as_s64();
                        case type::U64: return lr.as_s32() + rr.as_u64();
                        case type::FLOAT: return lr.as_s32() + rr.as_float();
                        case type::DOUBLE: return lr.as_s32() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s32() + rr.as_long_double();
                        case type::VEC4: return lr.as_s32() + rr.as_vec4();
                        case type::MAT4: return lr.as_s32() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_s32()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::itoa<std::string>(lr.as_s32()) + rr.as_string();
                        case type::WSTRING: return str_util::itoa<std::wstring>(lr.as_s32()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_s32();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(rt) {
                        case type::BOOL: return lr.as_u32() + rr.as_bool();
                        case type::CHAR: return lr.as_u32() + rr.as_char();
                        case type::S8: return lr.as_u32() + rr.as_s8();
                        case type::U8: return lr.as_u32() + rr.as_u8();
                        case type::S16: return lr.as_u32() + rr.as_s16();
                        case type::U16: return lr.as_u32() + rr.as_u16();
                        case type::WCHAR: return lr.as_u32() + rr.as_wchar();
                        case type::S32: return lr.as_u32() + rr.as_s32();
                        case type::U32: return lr.as_u32() + rr.as_u32();
                        case type::S64: return lr.as_u32() + rr.as_s64();
                        case type::U64: return lr.as_u32() + rr.as_u64();
                        case type::FLOAT: return lr.as_u32() + rr.as_float();
                        case type::DOUBLE: return lr.as_u32() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u32() + rr.as_long_double();
                        case type::VEC4: return lr.as_u32() + rr.as_vec4();
                        case type::MAT4: return lr.as_u32() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_u32()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::utoa<std::string>(lr.as_u32()) + rr.as_string();
                        case type::WSTRING: return str_util::utoa<std::wstring>(lr.as_u32()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_u32();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(rt) {
                        case type::BOOL: return lr.as_s64() + rr.as_bool();
                        case type::CHAR: return lr.as_s64() + rr.as_char();
                        case type::S8: return lr.as_s64() + rr.as_s8();
                        case type::U8: return lr.as_s64() + rr.as_u8();
                        case type::S16: return lr.as_s64() + rr.as_s16();
                        case type::U16: return lr.as_s64() + rr.as_u16();
                        case type::WCHAR: return lr.as_s64() + rr.as_wchar();
                        case type::S32: return lr.as_s64() + rr.as_s32();
                        case type::U32: return lr.as_s64() + rr.as_u32();
                        case type::S64: return lr.as_s64() + rr.as_s64();
                        case type::U64: return lr.as_s64() + rr.as_u64();
                        case type::FLOAT: return lr.as_s64() + rr.as_float();
                        case type::DOUBLE: return lr.as_s64() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s64() + rr.as_long_double();
                        case type::VEC4: return lr.as_s64() + rr.as_vec4();
                        case type::MAT4: return lr.as_s64() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_s64()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::itoa<std::string>(lr.as_s64()) + rr.as_string();
                        case type::WSTRING: return str_util::itoa<std::wstring>(lr.as_s64()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_s64();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(rt) {
                        case type::BOOL: return lr.as_u64() + rr.as_bool();
                        case type::CHAR: return lr.as_u64() + rr.as_char();
                        case type::S8: return lr.as_u64() + rr.as_s8();
                        case type::U8: return lr.as_u64() + rr.as_u8();
                        case type::S16: return lr.as_u64() + rr.as_s16();
                        case type::U16: return lr.as_u64() + rr.as_u16();
                        case type::WCHAR: return lr.as_u64() + rr.as_wchar();
                        case type::S32: return lr.as_u64() + rr.as_s32();
                        case type::U32: return lr.as_u64() + rr.as_u32();
                        case type::S64: return lr.as_u64() + rr.as_s64();
                        case type::U64: return lr.as_u64() + rr.as_u64();
                        case type::FLOAT: return lr.as_u64() + rr.as_float();
                        case type::DOUBLE: return lr.as_u64() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u64() + rr.as_long_double();
                        case type::VEC4: return lr.as_u64() + rr.as_vec4();
                        case type::MAT4: return lr.as_u64() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_u64()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::utoa<std::string>(lr.as_u64()) + rr.as_string();
                        case type::WSTRING: return str_util::utoa<std::wstring>(lr.as_u64()) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_u64();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(rt) {
                        case type::BOOL: return lr.as_float() + rr.as_bool();
                        case type::CHAR: return lr.as_float() + rr.as_char();
                        case type::S8: return lr.as_float() + rr.as_s8();
                        case type::U8: return lr.as_float() + rr.as_u8();
                        case type::S16: return lr.as_float() + rr.as_s16();
                        case type::U16: return lr.as_float() + rr.as_u16();
                        case type::WCHAR: return lr.as_float() + rr.as_wchar();
                        case type::S32: return lr.as_float() + rr.as_s32();
                        case type::U32: return lr.as_float() + rr.as_u32();
                        case type::S64: return lr.as_float() + rr.as_s64();
                        case type::U64: return lr.as_float() + rr.as_u64();
                        case type::FLOAT: return lr.as_float() + rr.as_float();
                        case type::DOUBLE: return lr.as_float() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_float() + rr.as_long_double();
                        case type::VEC4: return lr.as_float() + rr.as_vec4();
                        case type::MAT4: return lr.as_float() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_float()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::ftoa(lr.as_float()) + rr.as_string();
                        case type::WSTRING: return str_util::from_utf8(str_util::ftoa(lr.as_float())) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_float();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_double() + rr.as_bool();
                        case type::CHAR: return lr.as_double() + rr.as_char();
                        case type::S8: return lr.as_double() + rr.as_s8();
                        case type::U8: return lr.as_double() + rr.as_u8();
                        case type::S16: return lr.as_double() + rr.as_s16();
                        case type::U16: return lr.as_double() + rr.as_u16();
                        case type::WCHAR: return lr.as_double() + rr.as_wchar();
                        case type::S32: return lr.as_double() + rr.as_s32();
                        case type::U32: return lr.as_double() + rr.as_u32();
                        case type::S64: return lr.as_double() + rr.as_s64();
                        case type::U64: return lr.as_double() + rr.as_u64();
                        case type::FLOAT: return lr.as_double() + rr.as_float();
                        case type::DOUBLE: return lr.as_double() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_double() + rr.as_long_double();
                        case type::VEC4: return lr.as_double() + rr.as_vec4();
                        case type::MAT4: return lr.as_double() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_double()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::ftoa(lr.as_double()) + rr.as_string();
                        case type::WSTRING: return str_util::from_utf8(str_util::ftoa(lr.as_double())) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_double();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_long_double() + rr.as_bool();
                        case type::CHAR: return lr.as_long_double() + rr.as_char();
                        case type::S8: return lr.as_long_double() + rr.as_s8();
                        case type::U8: return lr.as_long_double() + rr.as_u8();
                        case type::S16: return lr.as_long_double() + rr.as_s16();
                        case type::U16: return lr.as_long_double() + rr.as_u16();
                        case type::WCHAR: return lr.as_long_double() + rr.as_wchar();
                        case type::S32: return lr.as_long_double() + rr.as_s32();
                        case type::U32: return lr.as_long_double() + rr.as_u32();
                        case type::S64: return lr.as_long_double() + rr.as_s64();
                        case type::U64: return lr.as_long_double() + rr.as_u64();
                        case type::FLOAT: return lr.as_long_double() + rr.as_float();
                        case type::DOUBLE: return lr.as_long_double() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_long_double() + rr.as_long_double();
                        case type::VEC4: return lr.as_long_double() + rr.as_vec4();
                        case type::MAT4: return lr.as_long_double() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.as_long_double()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return str_util::ftoa(lr.as_long_double()) + rr.as_string();
                        case type::WSTRING: return str_util::from_utf8(str_util::ftoa(lr.as_long_double())) + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_long_double();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(rt) {
                        case type::BOOL: return lr.as_vec4() + rr.as_bool();
                        case type::CHAR: return lr.as_vec4() + rr.as_char();
                        case type::S8: return lr.as_vec4() + rr.as_s8();
                        case type::U8: return lr.as_vec4() + rr.as_u8();
                        case type::S16: return lr.as_vec4() + rr.as_s16();
                        case type::U16: return lr.as_vec4() + rr.as_u16();
                        case type::WCHAR: return lr.as_vec4() + rr.as_wchar();
                        case type::S32: return lr.as_vec4() + rr.as_s32();
                        case type::U32: return lr.as_vec4() + rr.as_u32();
                        case type::S64: return lr.as_vec4() + rr.as_s64();
                        case type::U64: return lr.as_vec4() + rr.as_u64();
                        case type::FLOAT: return lr.as_vec4() + rr.as_float();
                        case type::DOUBLE: return lr.as_vec4() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_vec4() + rr.as_long_double();
                        case type::VEC4: return lr.as_vec4() + rr.as_vec4();
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.clone()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return lr.cast_to_string() + rr.as_string();
                        case type::WSTRING: return lr.cast_to_wstring() + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_vec4();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(rt) {
                        case type::BOOL: return lr.as_mat4() + rr.as_bool();
                        case type::CHAR: return lr.as_mat4() + rr.as_char();
                        case type::S8: return lr.as_mat4() + rr.as_s8();
                        case type::U8: return lr.as_mat4() + rr.as_u8();
                        case type::S16: return lr.as_mat4() + rr.as_s16();
                        case type::U16: return lr.as_mat4() + rr.as_u16();
                        case type::WCHAR: return lr.as_mat4() + rr.as_wchar();
                        case type::S32: return lr.as_mat4() + rr.as_s32();
                        case type::U32: return lr.as_mat4() + rr.as_u32();
                        case type::S64: return lr.as_mat4() + rr.as_s64();
                        case type::U64: return lr.as_mat4() + rr.as_u64();
                        case type::FLOAT: return lr.as_mat4() + rr.as_float();
                        case type::DOUBLE: return lr.as_mat4() + rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_mat4() + rr.as_long_double();
                        case type::VEC4: break;
                        case type::MAT4: return lr.as_mat4() + rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY:  { auto res{rr.clone()}; res.as_array().push_front(lr.clone()); return res; }
                        case type::OBJECT: break;
                        case type::STRING: return lr.cast_to_string() + rr.as_string();
                        case type::WSTRING: return lr.cast_to_wstring() + rr.as_wstring();
                        case type::UNDEFINED: return lr.as_mat4();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::POINTER:
                    break;
                case type::CLASS:
                    break;
                case type::FUNC:
                    break;
                case type::ARRAY:
                    switch(rt) {
                        case type::BOOL:
                        case type::CHAR:
                        case type::S8:
                        case type::U8:
                        case type::S16:
                        case type::U16:
                        case type::WCHAR:
                        case type::S32:
                        case type::U32:
                        case type::S64:
                        case type::U64:
                        case type::FLOAT:
                        case type::DOUBLE:
                        case type::LONG_DOUBLE:
                        case type::VEC4:
                        case type::MAT4:
                        case type::POINTER:
                        case type::CLASS:
                        case type::FUNC:
                        case type::OBJECT:
                        case type::STRING:
                        case type::WSTRING:
                        case type::UNDEFINED:
                        case type::VALBOX: { auto res{lr.clone()}; res.as_array().push_back(rr.clone()); return res; }
                        case type::ARRAY: { auto res{lr.clone()}; for(auto &&v: rr.as_array()) { res.as_array().push_back(v.clone()); } return res; }
                        default: break;
                    }
                    break;
                case type::OBJECT:
                    switch(rt) {
                        case type::BOOL: break;
                        case type::CHAR: break;
                        case type::S8: break;
                        case type::U8: break;
                        case type::S16: break;
                        case type::U16: break;
                        case type::WCHAR: break;
                        case type::S32: break;
                        case type::U32: break;
                        case type::S64: break;
                        case type::U64: break;
                        case type::FLOAT: break;
                        case type::DOUBLE: break;
                        case type::LONG_DOUBLE: break;
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: break;
                        case type::VALBOX: break;
                        case type::ARRAY: break;
                        case type::OBJECT: {
                            auto res{lr.clone()};
                            for(auto &&p: rr.as_object()) {
                                res.as_object()[p.first] = p.second.clone();
                            }
                            return res;
                        }
                        default: break;
                    }
                    break;
                case type::STRING:
                    return lr.as_string() + rr.cast_to_string();
                    break;
                case type::WSTRING:
                    return lr.as_wstring() + rr.cast_to_wstring();
                    break;
                case type::UNDEFINED:
                    return rr.clone();
                case type::VALBOX:
                    break;
                default:
                    break;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend bool operator==(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lr.box_.get() == rr.box_.get() || (lt == rt && lt == type::UNDEFINED)) {
                return true;
            }
            if(lt == rt) {
                switch(lt) {
                    case type::S64: return lr.as_s64() == rr.as_s64();
                    case type::U64: return lr.as_u64() == rr.as_u64();
                    case type::CHAR: return lr.as_char() == rr.as_char();
                    case type::U8: return lr.as_u8() == rr.as_u8();
                    case type::S8: return lr.as_s8() == rr.as_s8();
                    case type::U16: return lr.as_u16() == rr.as_u16();
                    case type::S16: return lr.as_s16() == rr.as_s16();
                    case type::U32: return lr.as_u32() == rr.as_u32();
                    case type::S32: return lr.as_s32() == rr.as_s32();
                    case type::FLOAT: return lr.as_float() == rr.as_float();
                    case type::DOUBLE: return lr.as_double() == rr.as_double();
                    case type::LONG_DOUBLE: return lr.as_long_double() == rr.as_long_double();
                    case type::BOOL: return lr.as_bool() == rr.as_bool();
                    case type::WCHAR: return lr.as_wchar() == rr.as_wchar();
                    case type::STRING: return lr.as_string() == rr.as_string();
                    case type::WSTRING: return lr.as_wstring() == rr.as_wstring();
                    case type::UNDEFINED: return true;
                    case type::VEC4: return lr.as_vec4() == rr.as_vec4();
                    case type::MAT4: return lr.as_mat4() == rr.as_mat4();
                    case type::POINTER: return lr.as_ptr() == rr.as_ptr();
                    case type::CLASS: break;
                    case type::FUNC: {
                        return lr.as_func().target_type().hash_code() == rr.as_func().target_type().hash_code();
                    }
                    case type::ARRAY: return lr.as_array() == rr.as_array();
                    case type::OBJECT: return lr.as_object() == rr.as_object();
                    case type::VALBOX: break;
                    default: break;
                }
            } else {
                if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                    return false;
                }
                if(is_numeric_type(lt) && is_numeric_type(rt)) {
                    if(static_cast<int>(lt) > static_cast<int>(rt)) {
                        switch(lt) {
                            case type::CHAR: return lr.as_char() == rr.cast_num_to_num<char>();
                            case type::WCHAR: return lr.as_wchar() == rr.cast_num_to_num<wchar_t>();
                            case type::U8: return lr.as_u8() == rr.cast_num_to_num<std::uint8_t>();
                            case type::S8: return lr.as_s8() == rr.cast_num_to_num<std::int8_t>();
                            case type::U16: return lr.as_u16() == rr.cast_num_to_num<std::uint16_t>();
                            case type::S16: return lr.as_s16() == rr.cast_num_to_num<std::int16_t>();
                            case type::U32: return lr.as_u32() == rr.cast_num_to_num<std::uint32_t>();
                            case type::S32: return lr.as_s32() == rr.cast_num_to_num<std::int32_t>();
                            case type::U64: return lr.as_u64() == rr.cast_num_to_num<std::uint64_t>();
                            case type::S64: return lr.as_s64() == rr.cast_num_to_num<std::int64_t>();
                            case type::FLOAT: return lr.as_float() == rr.cast_num_to_num<float>();
                            case type::DOUBLE: return lr.as_double() == rr.cast_num_to_num<double>();
                            case type::LONG_DOUBLE: return lr.as_long_double() == rr.cast_num_to_num<long double>();
                            default: break;
                        }
                    } else {
                        switch(rt) {
                            case type::CHAR: return lr.cast_num_to_num<char>() == rr.as_char();
                            case type::WCHAR: return lr.cast_num_to_num<wchar_t>() == rr.as_wchar();
                            case type::U8: return lr.cast_num_to_num<std::uint8_t>() == rr.as_u8();
                            case type::S8: return lr.cast_num_to_num<std::int8_t>() == rr.as_s8();
                            case type::U16: return lr.cast_num_to_num<std::uint16_t>() == rr.as_u16();
                            case type::S16: return lr.cast_num_to_num<std::int16_t>() == rr.as_s16();
                            case type::U32: return lr.cast_num_to_num<std::uint32_t>() == rr.as_u32();
                            case type::S32: return lr.cast_num_to_num<std::int32_t>() == rr.as_s32();
                            case type::U64: return lr.cast_num_to_num<std::uint64_t>() == rr.as_u64();
                            case type::S64: return lr.cast_num_to_num<std::int64_t>() == rr.as_s64();
                            case type::FLOAT: return lr.cast_num_to_num<float>() == rr.as_float();
                            case type::DOUBLE: return lr.cast_num_to_num<double>() == rr.as_double();
                            case type::LONG_DOUBLE: return lr.cast_num_to_num<long double>() == rr.as_long_double();
                            default: break;
                        }
                    }
                } else {
                    if(lt == type::STRING) {
                        return lr.as_string() == rr.cast_to_string();
                    } else if(lt == type::WSTRING) {
                        return lr.as_wstring() == rr.cast_to_wstring();
                    } else if(rt == type::STRING) {
                        return lr.cast_to_string() == rr.as_string();
                    } else if(rt == type::WSTRING) {
                        return lr.cast_to_wstring() == rr.as_wstring();
                    } else {
                        return false;
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend bool operator<=(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == rt) {
                switch(lt) {
                    case type::S64: return lr.as_s64() <= rr.as_s64();
                    case type::U64: return lr.as_u64() <= rr.as_u64();
                    case type::CHAR: return lr.as_char() <= rr.as_char();
                    case type::U8: return lr.as_u8() <= rr.as_u8();
                    case type::S8: return lr.as_s8() <= rr.as_s8();
                    case type::U16: return lr.as_u16() <= rr.as_u16();
                    case type::S16: return lr.as_s16() <= rr.as_s16();
                    case type::U32: return lr.as_u32() <= rr.as_u32();
                    case type::S32: return lr.as_s32() <= rr.as_s32();
                    case type::FLOAT: return lr.as_float() <= rr.as_float();
                    case type::DOUBLE: return lr.as_double() <= rr.as_double();
                    case type::LONG_DOUBLE: return lr.as_long_double() <= rr.as_long_double();
                    case type::BOOL: return lr.as_bool() <= rr.as_bool();
                    case type::WCHAR: return lr.as_wchar() <= rr.as_wchar();
                    case type::STRING: return lr.as_string() <= rr.as_string();
                    case type::WSTRING: return lr.as_wstring() <= rr.as_wstring();
                    case type::UNDEFINED: return true;
                    default: break;
                }
            } else {
                if(lt == type::UNDEFINED) {
                    return true;
                } else if(rt == type::UNDEFINED) {
                    return false;
                }
                if(is_numeric_type(lt) && is_numeric_type(rt)) {
                    if(static_cast<int>(lt) > static_cast<int>(rt)) {
                        switch(lt) {
                            case type::CHAR: return lr.as_char() <= rr.cast_num_to_num<char>();
                            case type::WCHAR: return lr.as_wchar() <= rr.cast_num_to_num<wchar_t>();
                            case type::U8: return lr.as_u8() <= rr.cast_num_to_num<std::uint8_t>();
                            case type::S8: return lr.as_s8() <= rr.cast_num_to_num<std::int8_t>();
                            case type::U16: return lr.as_u16() <= rr.cast_num_to_num<std::uint16_t>();
                            case type::S16: return lr.as_s16() <= rr.cast_num_to_num<std::int16_t>();
                            case type::U32: return lr.as_u32() <= rr.cast_num_to_num<std::uint32_t>();
                            case type::S32: return lr.as_s32() <= rr.cast_num_to_num<std::int32_t>();
                            case type::U64: return lr.as_u64() <= rr.cast_num_to_num<std::uint64_t>();
                            case type::S64: return lr.as_s64() <= rr.cast_num_to_num<std::int64_t>();
                            case type::FLOAT: return lr.as_float() <= rr.cast_num_to_num<float>();
                            case type::DOUBLE: return lr.as_double() <= rr.cast_num_to_num<double>();
                            case type::LONG_DOUBLE: return lr.as_long_double() <= rr.cast_num_to_num<long double>();
                            default: break;
                        }
                    } else {
                        switch(rt) {
                            case type::CHAR: return lr.cast_num_to_num<char>() <= rr.as_char();
                            case type::WCHAR: return lr.cast_num_to_num<wchar_t>() <= rr.as_wchar();
                            case type::U8: return lr.cast_num_to_num<std::uint8_t>() <= rr.as_u8();
                            case type::S8: return lr.cast_num_to_num<std::int8_t>() <= rr.as_s8();
                            case type::U16: return lr.cast_num_to_num<std::uint16_t>() <= rr.as_u16();
                            case type::S16: return lr.cast_num_to_num<std::int16_t>() <= rr.as_s16();
                            case type::U32: return lr.cast_num_to_num<std::uint32_t>() <= rr.as_u32();
                            case type::S32: return lr.cast_num_to_num<std::int32_t>() <= rr.as_s32();
                            case type::U64: return lr.cast_num_to_num<std::uint64_t>() <= rr.as_u64();
                            case type::S64: return lr.cast_num_to_num<std::int64_t>() <= rr.as_s64();
                            case type::FLOAT: return lr.cast_num_to_num<float>() <= rr.as_float();
                            case type::DOUBLE: return lr.cast_num_to_num<double>() <= rr.as_double();
                            case type::LONG_DOUBLE: return lr.cast_num_to_num<long double>() <= rr.as_long_double();
                            default: break;
                        }
                    }
                } else {
                    if(lt == type::STRING) {
                        return lr.as_string() <= rr.cast_to_string();
                    } else if(lt == type::WSTRING) {
                        return lr.as_wstring() <= rr.cast_to_wstring();
                    } else if(rt == type::STRING) {
                        return lr.cast_to_string() <= rr.as_string();
                    } else if(rt == type::WSTRING) {
                        return lr.cast_to_wstring() <= rr.as_wstring();
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend bool operator<(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == rt) {
                switch(lt) {
                    case type::S64: return lr.as_s64() < rr.as_s64();
                    case type::U64: return lr.as_u64() < rr.as_u64();
                    case type::CHAR: return lr.as_char() < rr.as_char();
                    case type::U8: return lr.as_u8() < rr.as_u8();
                    case type::S8: return lr.as_s8() < rr.as_s8();
                    case type::U16: return lr.as_u16() < rr.as_u16();
                    case type::S16: return lr.as_s16() < rr.as_s16();
                    case type::U32: return lr.as_u32() < rr.as_u32();
                    case type::S32: return lr.as_s32() < rr.as_s32();
                    case type::FLOAT: return lr.as_float() < rr.as_float();
                    case type::DOUBLE: return lr.as_double() < rr.as_double();
                    case type::LONG_DOUBLE: return lr.as_long_double() < rr.as_long_double();
                    case type::BOOL: return lr.as_bool() < rr.as_bool();
                    case type::WCHAR: return lr.as_wchar() < rr.as_wchar();
                    case type::STRING: return lr.as_string() < rr.as_string();
                    case type::WSTRING: return lr.as_wstring() < rr.as_wstring();
                    case type::UNDEFINED: return false;
                    default: break;
                }
            } else {
                if(lt == type::UNDEFINED) {
                    return true;
                } else if(rt == type::UNDEFINED) {
                    return false;
                }
                if(is_numeric_type(lt) && is_numeric_type(rt)) {
                    if(static_cast<int>(lt) > static_cast<int>(rt)) {
                        switch(lt) {
                            case type::CHAR: return lr.as_char() < rr.cast_num_to_num<char>();
                            case type::WCHAR: return lr.as_wchar() < rr.cast_num_to_num<wchar_t>();
                            case type::U8: return lr.as_u8() < rr.cast_num_to_num<std::uint8_t>();
                            case type::S8: return lr.as_s8() < rr.cast_num_to_num<std::int8_t>();
                            case type::U16: return lr.as_u16() < rr.cast_num_to_num<std::uint16_t>();
                            case type::S16: return lr.as_s16() < rr.cast_num_to_num<std::int16_t>();
                            case type::U32: return lr.as_u32() < rr.cast_num_to_num<std::uint32_t>();
                            case type::S32: return lr.as_s32() < rr.cast_num_to_num<std::int32_t>();
                            case type::U64: return lr.as_u64() < rr.cast_num_to_num<std::uint64_t>();
                            case type::S64: return lr.as_s64() < rr.cast_num_to_num<std::int64_t>();
                            case type::FLOAT: return lr.as_float() < rr.cast_num_to_num<float>();
                            case type::DOUBLE: return lr.as_double() < rr.cast_num_to_num<double>();
                            case type::LONG_DOUBLE: return lr.as_long_double() < rr.cast_num_to_num<long double>();
                            default: break;
                        }
                    } else {
                        switch(rt) {
                            case type::CHAR: return lr.cast_num_to_num<char>() < rr.as_char();
                            case type::WCHAR: return lr.cast_num_to_num<wchar_t>() < rr.as_wchar();
                            case type::U8: return lr.cast_num_to_num<std::uint8_t>() < rr.as_u8();
                            case type::S8: return lr.cast_num_to_num<std::int8_t>() < rr.as_s8();
                            case type::U16: return lr.cast_num_to_num<std::uint16_t>() < rr.as_u16();
                            case type::S16: return lr.cast_num_to_num<std::int16_t>() < rr.as_s16();
                            case type::U32: return lr.cast_num_to_num<std::uint32_t>() < rr.as_u32();
                            case type::S32: return lr.cast_num_to_num<std::int32_t>() < rr.as_s32();
                            case type::U64: return lr.cast_num_to_num<std::uint64_t>() < rr.as_u64();
                            case type::S64: return lr.cast_num_to_num<std::int64_t>() < rr.as_s64();
                            case type::FLOAT: return lr.cast_num_to_num<float>() < rr.as_float();
                            case type::DOUBLE: return lr.cast_num_to_num<double>() < rr.as_double();
                            case type::LONG_DOUBLE: return lr.cast_num_to_num<long double>() < rr.as_long_double();
                            default: break;
                        }
                    }
                } else {
                    if(lt == type::STRING) {
                        return lr.as_string() < rr.cast_to_string();
                    } else if(lt == type::WSTRING) {
                        return lr.as_wstring() < rr.cast_to_wstring();
                    } else if(rt == type::STRING) {
                        return lr.cast_to_string() < rr.as_string();
                    } else if(rt == type::WSTRING) {
                        return lr.cast_to_wstring() < rr.as_wstring();
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend bool operator>(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED && rt == type::UNDEFINED) {
                return false;
            }
            return !(lr <= rr);
        }

        friend bool operator>=(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED && rt == type::UNDEFINED) {
                return true;
            }
            return !(lr < rr);
        }

#if (__cplusplus >= 202000L)
        friend int operator<=>(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == rt) {
                switch(lt) {
                    case type::S64: {
                        auto ar{lr.as_s64() <=> rr.as_s64()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::U64: {
                        auto ar{lr.as_u64() <=> rr.as_u64()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::CHAR: {
                        auto ar{lr.as_char() <=> rr.as_char()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::U8: {
                        auto ar{lr.as_u8() <=> rr.as_u8()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::S8: {
                        auto ar{lr.as_s8() <=> rr.as_s8()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::U16: {
                        auto ar{lr.as_u16() <=> rr.as_u16()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::S16: {
                        auto ar{lr.as_s16() <=> rr.as_s16()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::U32: {
                        auto ar{lr.as_u32() <=> rr.as_u32()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::S32: {
                        auto ar{lr.as_s32() <=> rr.as_s32()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::FLOAT: {
                        auto ar{lr.as_float() <=> rr.as_float()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::DOUBLE: {
                        auto ar{lr.as_double() <=> rr.as_double()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::LONG_DOUBLE: {
                        auto ar{lr.as_long_double() <=> rr.as_long_double()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::BOOL: {
                        auto ar{lr.as_bool() <=> rr.as_bool()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::WCHAR: {
                        auto ar{lr.as_wchar() <=> rr.as_wchar()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::STRING: {
                        auto ar{lr.as_string() <=> rr.as_string()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::WSTRING: {
                        auto ar{lr.as_wstring() <=> rr.as_wstring()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                    case type::UNDEFINED:
                        return 0;
                    default: break;
                }
            } else {
                if(lt == type::UNDEFINED) {
                    return -1;
                } else if(rt == type::UNDEFINED) {
                    return 1;
                }
                if(is_numeric_type(lt) && is_numeric_type(rt)) {
                    if(static_cast<int>(lt) > static_cast<int>(rt)) {
                        switch(lt) {
                            case type::CHAR: {
                                auto ar{lr.as_char() <=> rr.cast_num_to_num<char>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::WCHAR: {
                                auto ar{lr.as_wchar() <=> rr.cast_num_to_num<wchar_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U8: {
                                auto ar{lr.as_u8() <=> rr.cast_num_to_num<std::uint8_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S8: {
                                auto ar{lr.as_s8() <=> rr.cast_num_to_num<std::int8_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U16: {
                                auto ar{lr.as_u16() <=> rr.cast_num_to_num<std::uint16_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S16: {
                                auto ar{lr.as_s16() <=> rr.cast_num_to_num<std::int16_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U32: {
                                auto ar{lr.as_u32() <=> rr.cast_num_to_num<std::uint32_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S32: {
                                auto ar{lr.as_s32() <=> rr.cast_num_to_num<std::int32_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U64: {
                                auto ar{lr.as_u64() <=> rr.cast_num_to_num<std::uint64_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S64: {
                                auto ar{lr.as_s64() <=> rr.cast_num_to_num<std::int64_t>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::FLOAT: {
                                auto ar{lr.as_float() <=> rr.cast_num_to_num<float>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::DOUBLE: {
                                auto ar{lr.as_double() <=> rr.cast_num_to_num<double>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::LONG_DOUBLE: {
                                auto ar{lr.as_long_double() <=> rr.cast_num_to_num<long double>()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            default: break;
                        }
                    } else {
                        switch(rt) {
                            case type::CHAR: {
                                auto ar{lr.cast_num_to_num<char>() <=> rr.as_char()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::WCHAR: {
                                auto ar{lr.cast_num_to_num<wchar_t>() <=> rr.as_wchar()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U8: {
                                auto ar{lr.cast_num_to_num<std::uint8_t>() <=> rr.as_u8()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S8: {
                                auto ar{lr.cast_num_to_num<std::int8_t>() <=> rr.as_s8()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U16: {
                                auto ar{lr.cast_num_to_num<std::uint16_t>() <=> rr.as_u16()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S16: {
                                auto ar{lr.cast_num_to_num<std::int16_t>() <=> rr.as_s16()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U32: {
                                auto ar{lr.cast_num_to_num<std::uint32_t>() <=> rr.as_u32()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S32: {
                                auto ar{lr.cast_num_to_num<std::int32_t>() <=> rr.as_s32()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::U64: {
                                auto ar{lr.cast_num_to_num<std::uint64_t>() <=> rr.as_u64()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::S64: {
                                auto ar{lr.cast_num_to_num<std::int64_t>() <=> rr.as_s64()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::FLOAT: {
                                auto ar{lr.cast_num_to_num<float>() <=> rr.as_float()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::DOUBLE: {
                                auto ar{lr.cast_num_to_num<double>() <=> rr.as_double()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            case type::LONG_DOUBLE: {
                                auto ar{lr.cast_num_to_num<long double>() <=> rr.as_long_double()};
                                return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                            }
                            default: break;
                        }
                    }
                } else {
                    if(lt == type::STRING) {
                        auto ar{lr.as_string() <=> rr.cast_to_string()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    } else if(lt == type::WSTRING) {
                        auto ar{lr.as_wstring() <=> rr.cast_to_wstring()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    } else if(rt == type::STRING) {
                        auto ar{lr.cast_to_string() <=> rr.as_string()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    } else if(rt == type::WSTRING) {
                        auto ar{lr.cast_to_wstring() <=> rr.as_wstring()};
                        return std::strong_ordering::less == ar ? -1 : (std::strong_ordering::greater == ar ? 1 : 0);
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }
#else
        static int operator_spaceship(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == rt) {
                switch(lt) {
                    case type::S64: { auto lv{lr.as_s64()}; auto rv{rr.as_s64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::U64: { auto lv{lr.as_u64()}; auto rv{rr.as_u64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::CHAR: { auto lv{lr.as_char()}; auto rv{rr.as_char()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::U8: { auto lv{lr.as_u8()}; auto rv{rr.as_u8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::S8: { auto lv{lr.as_s8()}; auto rv{rr.as_s8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::U16: { auto lv{lr.as_u16()}; auto rv{rr.as_u16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::S16: { auto lv{lr.as_s16()}; auto rv{rr.as_s16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::U32: { auto lv{lr.as_u32()}; auto rv{rr.as_u32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::S32: { auto lv{lr.as_s32()}; auto rv{rr.as_s32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::FLOAT: { auto lv{lr.as_float()}; auto rv{rr.as_float()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::DOUBLE: { auto lv{lr.as_double()}; auto rv{rr.as_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::LONG_DOUBLE: { auto lv{lr.as_long_double()}; auto rv{rr.as_long_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::BOOL: { auto lv{(int)lr.as_bool()}; auto rv{(int)rr.as_bool()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::WCHAR: { auto lv{lr.as_wchar()}; auto rv{rr.as_wchar()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::STRING: { auto lv{lr.as_string()}; auto rv{rr.as_string()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::WSTRING: { auto lv{lr.as_wstring()}; auto rv{rr.as_wstring()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                    case type::UNDEFINED: return 0;
                    default: break;
                }
            } else {
                if(lt == type::UNDEFINED) {
                    return -1;
                } else if(rt == type::UNDEFINED) {
                    return 1;
                }
                if(is_numeric_type(lt) && is_numeric_type(rt)) {
                    if(static_cast<int>(lt) > static_cast<int>(rt)) {
                        switch(lt) {
                            case type::CHAR: { auto lv{lr.as_char()}; auto rv{rr.cast_to_char()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::WCHAR: { auto lv{lr.as_wchar()}; auto rv{rr.cast_to_wchar()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U8: { auto lv{lr.as_u8()}; auto rv{rr.cast_to_u8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S8: { auto lv{lr.as_s8()}; auto rv{rr.cast_to_s8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U16: { auto lv{lr.as_u16()}; auto rv{rr.cast_to_u16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S16: { auto lv{lr.as_s16()}; auto rv{rr.cast_to_s16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U32: { auto lv{lr.as_u32()}; auto rv{rr.cast_to_u32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S32: { auto lv{lr.as_s32()}; auto rv{rr.cast_to_s32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U64: { auto lv{lr.as_u64()}; auto rv{rr.cast_to_u64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S64: { auto lv{lr.as_s64()}; auto rv{rr.cast_to_s64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::FLOAT: { auto lv{lr.as_float()}; auto rv{rr.cast_to_float()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::DOUBLE: { auto lv{lr.as_double()}; auto rv{rr.cast_to_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::LONG_DOUBLE: { auto lv{lr.as_long_double()}; auto rv{rr.cast_to_long_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            default: break;
                        }
                    } else {
                        switch(rt) {
                            case type::CHAR: { auto lv{lr.cast_to_char()}; auto rv{rr.as_char()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::WCHAR: { auto lv{lr.cast_to_wchar()}; auto rv{rr.as_wchar()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U8: { auto lv{lr.cast_to_u8()}; auto rv{rr.as_u8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S8: { auto lv{lr.cast_to_s8()}; auto rv{rr.as_s8()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U16: { auto lv{lr.cast_to_u16()}; auto rv{rr.as_u16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S16: { auto lv{lr.cast_to_s16()}; auto rv{rr.as_s16()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U32: { auto lv{lr.cast_to_u32()}; auto rv{rr.as_u32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S32: { auto lv{lr.cast_to_s32()}; auto rv{rr.as_s32()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::U64: { auto lv{lr.cast_to_u64()}; auto rv{rr.as_u64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::S64: { auto lv{lr.cast_to_s64()}; auto rv{rr.as_s64()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::FLOAT: { auto lv{lr.cast_to_float()}; auto rv{rr.as_float()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::DOUBLE: { auto lv{lr.cast_to_double()}; auto rv{rr.as_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            case type::LONG_DOUBLE: { auto lv{lr.cast_to_long_double()}; auto rv{rr.as_long_double()}; return lv < rv ? -1 : (rv < lv ? 1 : 0); }
                            default: break;
                        }
                    }
                } else {
                    if(lt == type::STRING) {
                        auto lv{lr.as_string()};
                        auto rv{rr.cast_to_string()};
                        return lv < rv ? -1 : (rv < lv ? 1 : 0);
                    } else if(lt == type::WSTRING) {
                        auto lv{lr.as_wstring()};
                        auto rv{rr.cast_to_wstring()};
                        return lv < rv ? -1 : (rv < lv ? 1 : 0);
                    } else if(rt == type::STRING) {
                        auto lv{lr.cast_to_string()};
                        auto rv{rr.as_string()};
                        return lv < rv ? -1 : (rv < lv ? 1 : 0);
                    } else if(rt == type::WSTRING) {
                        auto lv{lr.cast_to_wstring()};
                        auto rv{rr.as_wstring()};
                        return lv < rv ? -1 : (rv < lv ? 1 : 0);
                    }
                }
            }
            throw std::runtime_error{"operation not applicable"};
        }
#endif

        friend bool operator!=(valbox const &l, valbox const &r) {
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED && rt == type::UNDEFINED) {
                return false;
            }
            return !(lr == rr);
        }

        friend valbox operator-(valbox const &lr, valbox const &rr) {
            auto lt{lr.val_or_pointed_type()};
            auto rt{rr.val_or_pointed_type()};
            switch(lt) {
                case type::BOOL:
                    switch(rt) {
                        case type::BOOL: return lr.as_bool() - rr.as_bool();
                        case type::CHAR: return lr.as_bool() - rr.as_char();
                        case type::S8: return lr.as_bool() - rr.as_s8();
                        case type::U8: return lr.as_bool() - rr.as_u8();
                        case type::S16: return lr.as_bool() - rr.as_s16();
                        case type::U16: return lr.as_bool() - rr.as_u16();
                        case type::WCHAR: return lr.as_bool() - rr.as_wchar();
                        case type::S32: return lr.as_bool() - rr.as_s32();
                        case type::U32: return lr.as_bool() - rr.as_u32();
                        case type::S64: return lr.as_bool() - rr.as_s64();
                        case type::U64: return lr.as_bool() - rr.as_u64();
                        case type::FLOAT: return lr.as_bool() - rr.as_float();
                        case type::DOUBLE: return lr.as_bool() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_bool() - rr.as_long_double();
                        case type::VEC4: return lr.as_bool() - rr.as_vec4();
                        case type::MAT4: return lr.as_bool() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_bool();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::CHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_char() - rr.as_bool();
                        case type::CHAR: return lr.as_char() - rr.as_char();
                        case type::S8: return lr.as_char() - rr.as_s8();
                        case type::U8: return lr.as_char() - rr.as_u8();
                        case type::S16: return lr.as_char() - rr.as_s16();
                        case type::U16: return lr.as_char() - rr.as_u16();
                        case type::WCHAR: return lr.as_char() - rr.as_wchar();
                        case type::S32: return lr.as_char() - rr.as_s32();
                        case type::U32: return lr.as_char() - rr.as_u32();
                        case type::S64: return lr.as_char() - rr.as_s64();
                        case type::U64: return lr.as_char() - rr.as_u64();
                        case type::FLOAT: return lr.as_char() - rr.as_float();
                        case type::DOUBLE: return lr.as_char() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_char() - rr.as_long_double();
                        case type::VEC4: return lr.as_char() - rr.as_vec4();
                        case type::MAT4: return lr.as_char() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_char();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S8:
                    switch(rt) {
                        case type::BOOL: return lr.as_s8() - rr.as_bool();
                        case type::CHAR: return lr.as_s8() - rr.as_char();
                        case type::S8: return lr.as_s8() - rr.as_s8();
                        case type::U8: return lr.as_s8() - rr.as_u8();
                        case type::S16: return lr.as_s8() - rr.as_s16();
                        case type::U16: return lr.as_s8() - rr.as_u16();
                        case type::WCHAR: return lr.as_s8() - rr.as_wchar();
                        case type::S32: return lr.as_s8() - rr.as_s32();
                        case type::U32: return lr.as_s8() - rr.as_u32();
                        case type::S64: return lr.as_s8() - rr.as_s64();
                        case type::U64: return lr.as_s8() - rr.as_u64();
                        case type::FLOAT: return lr.as_s8() - rr.as_float();
                        case type::DOUBLE: return lr.as_s8() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s8() - rr.as_long_double();
                        case type::VEC4: return lr.as_s8() - rr.as_vec4();
                        case type::MAT4: return lr.as_s8() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_s8();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U8:
                    switch(rt) {
                        case type::BOOL: return lr.as_u8() - rr.as_bool();
                        case type::CHAR: return lr.as_u8() - rr.as_char();
                        case type::S8: return lr.as_u8() - rr.as_s8();
                        case type::U8: return lr.as_u8() - rr.as_u8();
                        case type::S16: return lr.as_u8() - rr.as_s16();
                        case type::U16: return lr.as_u8() - rr.as_u16();
                        case type::WCHAR: return lr.as_u8() - rr.as_wchar();
                        case type::S32: return lr.as_u8() - rr.as_s32();
                        case type::U32: return lr.as_u8() - rr.as_u32();
                        case type::S64: return lr.as_u8() - rr.as_s64();
                        case type::U64: return lr.as_u8() - rr.as_u64();
                        case type::FLOAT: return lr.as_u8() - rr.as_float();
                        case type::DOUBLE: return lr.as_u8() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u8() - rr.as_long_double();
                        case type::VEC4: return lr.as_u8() - rr.as_vec4();
                        case type::MAT4: return lr.as_u8() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_u8();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S16:
                    switch(rt) {
                        case type::BOOL: return lr.as_s16() - rr.as_bool();
                        case type::CHAR: return lr.as_s16() - rr.as_char();
                        case type::S8: return lr.as_s16() - rr.as_s8();
                        case type::U8: return lr.as_s16() - rr.as_u8();
                        case type::S16: return lr.as_s16() - rr.as_s16();
                        case type::U16: return lr.as_s16() - rr.as_u16();
                        case type::WCHAR: return lr.as_s16() - rr.as_wchar();
                        case type::S32: return lr.as_s16() - rr.as_s32();
                        case type::U32: return lr.as_s16() - rr.as_u32();
                        case type::S64: return lr.as_s16() - rr.as_s64();
                        case type::U64: return lr.as_s16() - rr.as_u64();
                        case type::FLOAT: return lr.as_s16() - rr.as_float();
                        case type::DOUBLE: return lr.as_s16() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s16() - rr.as_long_double();
                        case type::VEC4: return lr.as_s16() - rr.as_vec4();
                        case type::MAT4: return lr.as_s16() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_s16();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(rt) {
                        case type::BOOL: return lr.as_u16() - rr.as_bool();
                        case type::CHAR: return lr.as_u16() - rr.as_char();
                        case type::S8: return lr.as_u16() - rr.as_s8();
                        case type::U8: return lr.as_u16() - rr.as_u8();
                        case type::S16: return lr.as_u16() - rr.as_s16();
                        case type::U16: return lr.as_u16() - rr.as_u16();
                        case type::WCHAR: return lr.as_u16() - rr.as_wchar();
                        case type::S32: return lr.as_u16() - rr.as_s32();
                        case type::U32: return lr.as_u16() - rr.as_u32();
                        case type::S64: return lr.as_u16() - rr.as_s64();
                        case type::U64: return lr.as_u16() - rr.as_u64();
                        case type::FLOAT: return lr.as_u16() - rr.as_float();
                        case type::DOUBLE: return lr.as_u16() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u16() - rr.as_long_double();
                        case type::VEC4: return lr.as_u16() - rr.as_vec4();
                        case type::MAT4: return lr.as_u16() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_u16();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_wchar() - rr.as_bool();
                        case type::CHAR: return lr.as_wchar() - rr.as_char();
                        case type::S8: return lr.as_wchar() - rr.as_s8();
                        case type::U8: return lr.as_wchar() - rr.as_u8();
                        case type::S16: return lr.as_wchar() - rr.as_s16();
                        case type::U16: return lr.as_wchar() - rr.as_u16();
                        case type::WCHAR: return lr.as_wchar() - rr.as_wchar();
                        case type::S32: return lr.as_wchar() - rr.as_s32();
                        case type::U32: return lr.as_wchar() - rr.as_u32();
                        case type::S64: return lr.as_wchar() - rr.as_s64();
                        case type::U64: return lr.as_wchar() - rr.as_u64();
                        case type::FLOAT: return lr.as_wchar() - rr.as_float();
                        case type::DOUBLE: return lr.as_wchar() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_wchar() - rr.as_long_double();
                        case type::VEC4: return lr.as_wchar() - rr.as_vec4();
                        case type::MAT4: return lr.as_wchar() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_wchar();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(rt) {
                        case type::BOOL: return lr.as_s32() - rr.as_bool();
                        case type::CHAR: return lr.as_s32() - rr.as_char();
                        case type::S8: return lr.as_s32() - rr.as_s8();
                        case type::U8: return lr.as_s32() - rr.as_u8();
                        case type::S16: return lr.as_s32() - rr.as_s16();
                        case type::U16: return lr.as_s32() - rr.as_u16();
                        case type::WCHAR: return lr.as_s32() - rr.as_wchar();
                        case type::S32: return lr.as_s32() - rr.as_s32();
                        case type::U32: return lr.as_s32() - rr.as_u32();
                        case type::S64: return lr.as_s32() - rr.as_s64();
                        case type::U64: return lr.as_s32() - rr.as_u64();
                        case type::FLOAT: return lr.as_s32() - rr.as_float();
                        case type::DOUBLE: return lr.as_s32() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s32() - rr.as_long_double();
                        case type::VEC4: return lr.as_s32() - rr.as_vec4();
                        case type::MAT4: return lr.as_s32() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_s32();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(rt) {
                        case type::BOOL: return lr.as_u32() - rr.as_bool();
                        case type::CHAR: return lr.as_u32() - rr.as_char();
                        case type::S8: return lr.as_u32() - rr.as_s8();
                        case type::U8: return lr.as_u32() - rr.as_u8();
                        case type::S16: return lr.as_u32() - rr.as_s16();
                        case type::U16: return lr.as_u32() - rr.as_u16();
                        case type::WCHAR: return lr.as_u32() - rr.as_wchar();
                        case type::S32: return lr.as_u32() - rr.as_s32();
                        case type::U32: return lr.as_u32() - rr.as_u32();
                        case type::S64: return lr.as_u32() - rr.as_s64();
                        case type::U64: return lr.as_u32() - rr.as_u64();
                        case type::FLOAT: return lr.as_u32() - rr.as_float();
                        case type::DOUBLE: return lr.as_u32() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u32() - rr.as_long_double();
                        case type::VEC4: return lr.as_u32() - rr.as_vec4();
                        case type::MAT4: return lr.as_u32() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_u32();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(rt) {
                        case type::BOOL: return lr.as_s64() - rr.as_bool();
                        case type::CHAR: return lr.as_s64() - rr.as_char();
                        case type::S8: return lr.as_s64() - rr.as_s8();
                        case type::U8: return lr.as_s64() - rr.as_u8();
                        case type::S16: return lr.as_s64() - rr.as_s16();
                        case type::U16: return lr.as_s64() - rr.as_u16();
                        case type::WCHAR: return lr.as_s64() - rr.as_wchar();
                        case type::S32: return lr.as_s64() - rr.as_s32();
                        case type::U32: return lr.as_s64() - rr.as_u32();
                        case type::S64: return lr.as_s64() - rr.as_s64();
                        case type::U64: return lr.as_s64() - rr.as_u64();
                        case type::FLOAT: return lr.as_s64() - rr.as_float();
                        case type::DOUBLE: return lr.as_s64() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s64() - rr.as_long_double();
                        case type::VEC4: return lr.as_s64() - rr.as_vec4();
                        case type::MAT4: return lr.as_s64() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_s64();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(rt) {
                        case type::BOOL: return lr.as_u64() - rr.as_bool();
                        case type::CHAR: return lr.as_u64() - rr.as_char();
                        case type::S8: return lr.as_u64() - rr.as_s8();
                        case type::U8: return lr.as_u64() - rr.as_u8();
                        case type::S16: return lr.as_u64() - rr.as_s16();
                        case type::U16: return lr.as_u64() - rr.as_u16();
                        case type::WCHAR: return lr.as_u64() - rr.as_wchar();
                        case type::S32: return lr.as_u64() - rr.as_s32();
                        case type::U32: return lr.as_u64() - rr.as_u32();
                        case type::S64: return lr.as_u64() - rr.as_s64();
                        case type::U64: return lr.as_u64() - rr.as_u64();
                        case type::FLOAT: return lr.as_u64() - rr.as_float();
                        case type::DOUBLE: return lr.as_u64() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u64() - rr.as_long_double();
                        case type::VEC4: return lr.as_u64() - rr.as_vec4();
                        case type::MAT4: return lr.as_u64() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_u64();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(rt) {
                        case type::BOOL: return lr.as_float() - rr.as_bool();
                        case type::CHAR: return lr.as_float() - rr.as_char();
                        case type::S8: return lr.as_float() - rr.as_s8();
                        case type::U8: return lr.as_float() - rr.as_u8();
                        case type::S16: return lr.as_float() - rr.as_s16();
                        case type::U16: return lr.as_float() - rr.as_u16();
                        case type::WCHAR: return lr.as_float() - rr.as_wchar();
                        case type::S32: return lr.as_float() - rr.as_s32();
                        case type::U32: return lr.as_float() - rr.as_u32();
                        case type::S64: return lr.as_float() - rr.as_s64();
                        case type::U64: return lr.as_float() - rr.as_u64();
                        case type::FLOAT: return lr.as_float() - rr.as_float();
                        case type::DOUBLE: return lr.as_float() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_float() - rr.as_long_double();
                        case type::VEC4: return lr.as_float() - rr.as_vec4();
                        case type::MAT4: return lr.as_float() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_float();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_double() - rr.as_bool();
                        case type::CHAR: return lr.as_double() - rr.as_char();
                        case type::S8: return lr.as_double() - rr.as_s8();
                        case type::U8: return lr.as_double() - rr.as_u8();
                        case type::S16: return lr.as_double() - rr.as_s16();
                        case type::U16: return lr.as_double() - rr.as_u16();
                        case type::WCHAR: return lr.as_double() - rr.as_wchar();
                        case type::S32: return lr.as_double() - rr.as_s32();
                        case type::U32: return lr.as_double() - rr.as_u32();
                        case type::S64: return lr.as_double() - rr.as_s64();
                        case type::U64: return lr.as_double() - rr.as_u64();
                        case type::FLOAT: return lr.as_double() - rr.as_float();
                        case type::DOUBLE: return lr.as_double() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_double() - rr.as_long_double();
                        case type::VEC4: return lr.as_double() - rr.as_vec4();
                        case type::MAT4: return lr.as_double() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_double();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_long_double() - rr.as_bool();
                        case type::CHAR: return lr.as_long_double() - rr.as_char();
                        case type::S8: return lr.as_long_double() - rr.as_s8();
                        case type::U8: return lr.as_long_double() - rr.as_u8();
                        case type::S16: return lr.as_long_double() - rr.as_s16();
                        case type::U16: return lr.as_long_double() - rr.as_u16();
                        case type::WCHAR: return lr.as_long_double() - rr.as_wchar();
                        case type::S32: return lr.as_long_double() - rr.as_s32();
                        case type::U32: return lr.as_long_double() - rr.as_u32();
                        case type::S64: return lr.as_long_double() - rr.as_s64();
                        case type::U64: return lr.as_long_double() - rr.as_u64();
                        case type::FLOAT: return lr.as_long_double() - rr.as_float();
                        case type::DOUBLE: return lr.as_long_double() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_long_double() - rr.as_long_double();
                        case type::VEC4: return lr.as_long_double() - rr.as_vec4();
                        case type::MAT4: return lr.as_long_double() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_long_double();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(rt) {
                        case type::BOOL: return lr.as_vec4() - rr.as_bool();
                        case type::CHAR: return lr.as_vec4() - rr.as_char();
                        case type::S8: return lr.as_vec4() - rr.as_s8();
                        case type::U8: return lr.as_vec4() - rr.as_u8();
                        case type::S16: return lr.as_vec4() - rr.as_s16();
                        case type::U16: return lr.as_vec4() - rr.as_u16();
                        case type::WCHAR: return lr.as_vec4() - rr.as_wchar();
                        case type::S32: return lr.as_vec4() - rr.as_s32();
                        case type::U32: return lr.as_vec4() - rr.as_u32();
                        case type::S64: return lr.as_vec4() - rr.as_s64();
                        case type::U64: return lr.as_vec4() - rr.as_u64();
                        case type::FLOAT: return lr.as_vec4() - rr.as_float();
                        case type::DOUBLE: return lr.as_vec4() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_vec4() - rr.as_long_double();
                        case type::VEC4: return lr.as_vec4() - rr.as_vec4();
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_vec4();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(rt) {
                        case type::BOOL: return lr.as_mat4() - rr.as_bool();
                        case type::CHAR: return lr.as_mat4() - rr.as_char();
                        case type::S8: return lr.as_mat4() - rr.as_s8();
                        case type::U8: return lr.as_mat4() - rr.as_u8();
                        case type::S16: return lr.as_mat4() - rr.as_s16();
                        case type::U16: return lr.as_mat4() - rr.as_u16();
                        case type::WCHAR: return lr.as_mat4() - rr.as_wchar();
                        case type::S32: return lr.as_mat4() - rr.as_s32();
                        case type::U32: return lr.as_mat4() - rr.as_u32();
                        case type::S64: return lr.as_mat4() - rr.as_s64();
                        case type::U64: return lr.as_mat4() - rr.as_u64();
                        case type::FLOAT: return lr.as_mat4() - rr.as_float();
                        case type::DOUBLE: return lr.as_mat4() - rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_mat4() - rr.as_long_double();
                        case type::VEC4: break;
                        case type::MAT4: return lr.as_mat4() - rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return lr.as_mat4();
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::POINTER:
                    break;
                case type::CLASS:
                    break;
                case type::FUNC:
                    break;
                case type::ARRAY:
                    break;
                case type::OBJECT:
                    break;
                case type::STRING:
                    break;
                case type::WSTRING:
                    break;
                case type::UNDEFINED:
                    switch(rt) {
                        case type::BOOL: return -rr.as_bool();
                        case type::CHAR: return -rr.as_char();
                        case type::S8: return -rr.as_s8();
                        case type::U8: return -rr.as_u8();
                        case type::S16: return -rr.as_s16();
                        case type::U16: return -rr.as_u16();
                        case type::WCHAR: return -rr.as_wchar();
                        case type::S32: return -rr.as_s32();
                        case type::U32: return -rr.as_u32();
                        case type::S64: return -rr.as_s64();
                        case type::U64: return -rr.as_u64();
                        case type::FLOAT: return -rr.as_float();
                        case type::DOUBLE: return -rr.as_double();
                        case type::LONG_DOUBLE: return -rr.as_long_double();
                        case type::VEC4: return -rr.as_vec4();
                        case type::MAT4: return -rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VALBOX:
                    break;
                default:
                    break;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator*(valbox const &lr, valbox const &rr) {
            auto lt{lr.val_or_pointed_type()};
            auto rt{rr.val_or_pointed_type()};
            switch(lt) {
                case type::BOOL:
                    switch(rt) {
                        case type::BOOL: return lr.as_bool() * rr.as_bool();
                        case type::CHAR: return lr.as_bool() * rr.as_char();
                        case type::S8: return lr.as_bool() * rr.as_s8();
                        case type::U8: return lr.as_bool() * rr.as_u8();
                        case type::S16: return lr.as_bool() * rr.as_s16();
                        case type::U16: return lr.as_bool() * rr.as_u16();
                        case type::WCHAR: return lr.as_bool() * rr.as_wchar();
                        case type::S32: return lr.as_bool() * rr.as_s32();
                        case type::U32: return lr.as_bool() * rr.as_u32();
                        case type::S64: return lr.as_bool() * rr.as_s64();
                        case type::U64: return lr.as_bool() * rr.as_u64();
                        case type::FLOAT: return lr.as_bool() * rr.as_float();
                        case type::DOUBLE: return lr.as_bool() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_bool() * rr.as_long_double();
                        case type::VEC4: return lr.as_bool() * rr.as_vec4();
                        case type::MAT4: return lr.as_bool() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return false;
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::CHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_char() * rr.as_bool();
                        case type::CHAR: return lr.as_char() * rr.as_char();
                        case type::S8: return lr.as_char() * rr.as_s8();
                        case type::U8: return lr.as_char() * rr.as_u8();
                        case type::S16: return lr.as_char() * rr.as_s16();
                        case type::U16: return lr.as_char() * rr.as_u16();
                        case type::WCHAR: return lr.as_char() * rr.as_wchar();
                        case type::S32: return lr.as_char() * rr.as_s32();
                        case type::U32: return lr.as_char() * rr.as_u32();
                        case type::S64: return lr.as_char() * rr.as_s64();
                        case type::U64: return lr.as_char() * rr.as_u64();
                        case type::FLOAT: return lr.as_char() * rr.as_float();
                        case type::DOUBLE: return lr.as_char() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_char() * rr.as_long_double();
                        case type::VEC4: return lr.as_char() * rr.as_vec4();
                        case type::MAT4: return lr.as_char() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return char{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S8:
                    switch(rt) {
                        case type::BOOL: return lr.as_s8() * rr.as_bool();
                        case type::CHAR: return lr.as_s8() * rr.as_char();
                        case type::S8: return lr.as_s8() * rr.as_s8();
                        case type::U8: return lr.as_s8() * rr.as_u8();
                        case type::S16: return lr.as_s8() * rr.as_s16();
                        case type::U16: return lr.as_s8() * rr.as_u16();
                        case type::WCHAR: return lr.as_s8() * rr.as_wchar();
                        case type::S32: return lr.as_s8() * rr.as_s32();
                        case type::U32: return lr.as_s8() * rr.as_u32();
                        case type::S64: return lr.as_s8() * rr.as_s64();
                        case type::U64: return lr.as_s8() * rr.as_u64();
                        case type::FLOAT: return lr.as_s8() * rr.as_float();
                        case type::DOUBLE: return lr.as_s8() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s8() * rr.as_long_double();
                        case type::VEC4: return lr.as_s8() * rr.as_vec4();
                        case type::MAT4: return lr.as_s8() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::int8_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U8:
                    switch(rt) {
                        case type::BOOL: return lr.as_u8() * rr.as_bool();
                        case type::CHAR: return lr.as_u8() * rr.as_char();
                        case type::S8: return lr.as_u8() * rr.as_s8();
                        case type::U8: return lr.as_u8() * rr.as_u8();
                        case type::S16: return lr.as_u8() * rr.as_s16();
                        case type::U16: return lr.as_u8() * rr.as_u16();
                        case type::WCHAR: return lr.as_u8() * rr.as_wchar();
                        case type::S32: return lr.as_u8() * rr.as_s32();
                        case type::U32: return lr.as_u8() * rr.as_u32();
                        case type::S64: return lr.as_u8() * rr.as_s64();
                        case type::U64: return lr.as_u8() * rr.as_u64();
                        case type::FLOAT: return lr.as_u8() * rr.as_float();
                        case type::DOUBLE: return lr.as_u8() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u8() * rr.as_long_double();
                        case type::VEC4: return lr.as_u8() * rr.as_vec4();
                        case type::MAT4: return lr.as_u8() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::uint8_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S16:
                    switch(rt) {
                        case type::BOOL: return lr.as_s16() * rr.as_bool();
                        case type::CHAR: return lr.as_s16() * rr.as_char();
                        case type::S8: return lr.as_s16() * rr.as_s8();
                        case type::U8: return lr.as_s16() * rr.as_u8();
                        case type::S16: return lr.as_s16() * rr.as_s16();
                        case type::U16: return lr.as_s16() * rr.as_u16();
                        case type::WCHAR: return lr.as_s16() * rr.as_wchar();
                        case type::S32: return lr.as_s16() * rr.as_s32();
                        case type::U32: return lr.as_s16() * rr.as_u32();
                        case type::S64: return lr.as_s16() * rr.as_s64();
                        case type::U64: return lr.as_s16() * rr.as_u64();
                        case type::FLOAT: return lr.as_s16() * rr.as_float();
                        case type::DOUBLE: return lr.as_s16() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s16() * rr.as_long_double();
                        case type::VEC4: return lr.as_s16() * rr.as_vec4();
                        case type::MAT4: return lr.as_s16() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::int16_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(rt) {
                        case type::BOOL: return lr.as_u16() * rr.as_bool();
                        case type::CHAR: return lr.as_u16() * rr.as_char();
                        case type::S8: return lr.as_u16() * rr.as_s8();
                        case type::U8: return lr.as_u16() * rr.as_u8();
                        case type::S16: return lr.as_u16() * rr.as_s16();
                        case type::U16: return lr.as_u16() * rr.as_u16();
                        case type::WCHAR: return lr.as_u16() * rr.as_wchar();
                        case type::S32: return lr.as_u16() * rr.as_s32();
                        case type::U32: return lr.as_u16() * rr.as_u32();
                        case type::S64: return lr.as_u16() * rr.as_s64();
                        case type::U64: return lr.as_u16() * rr.as_u64();
                        case type::FLOAT: return lr.as_u16() * rr.as_float();
                        case type::DOUBLE: return lr.as_u16() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u16() * rr.as_long_double();
                        case type::VEC4: return lr.as_u16() * rr.as_vec4();
                        case type::MAT4: return lr.as_u16() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::uint16_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(rt) {
                        case type::BOOL: return lr.as_wchar() * rr.as_bool();
                        case type::CHAR: return lr.as_wchar() * rr.as_char();
                        case type::S8: return lr.as_wchar() * rr.as_s8();
                        case type::U8: return lr.as_wchar() * rr.as_u8();
                        case type::S16: return lr.as_wchar() * rr.as_s16();
                        case type::U16: return lr.as_wchar() * rr.as_u16();
                        case type::WCHAR: return lr.as_wchar() * rr.as_wchar();
                        case type::S32: return lr.as_wchar() * rr.as_s32();
                        case type::U32: return lr.as_wchar() * rr.as_u32();
                        case type::S64: return lr.as_wchar() * rr.as_s64();
                        case type::U64: return lr.as_wchar() * rr.as_u64();
                        case type::FLOAT: return lr.as_wchar() * rr.as_float();
                        case type::DOUBLE: return lr.as_wchar() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_wchar() * rr.as_long_double();
                        case type::VEC4: return lr.as_wchar() * rr.as_vec4();
                        case type::MAT4: return lr.as_wchar() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return char{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(rt) {
                        case type::BOOL: return lr.as_s32() * rr.as_bool();
                        case type::CHAR: return lr.as_s32() * rr.as_char();
                        case type::S8: return lr.as_s32() * rr.as_s8();
                        case type::U8: return lr.as_s32() * rr.as_u8();
                        case type::S16: return lr.as_s32() * rr.as_s16();
                        case type::U16: return lr.as_s32() * rr.as_u16();
                        case type::WCHAR: return lr.as_s32() * rr.as_wchar();
                        case type::S32: return lr.as_s32() * rr.as_s32();
                        case type::U32: return lr.as_s32() * rr.as_u32();
                        case type::S64: return lr.as_s32() * rr.as_s64();
                        case type::U64: return lr.as_s32() * rr.as_u64();
                        case type::FLOAT: return lr.as_s32() * rr.as_float();
                        case type::DOUBLE: return lr.as_s32() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s32() * rr.as_long_double();
                        case type::VEC4: return lr.as_s32() * rr.as_vec4();
                        case type::MAT4: return lr.as_s32() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::int32_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(rt) {
                        case type::BOOL: return lr.as_u32() * rr.as_bool();
                        case type::CHAR: return lr.as_u32() * rr.as_char();
                        case type::S8: return lr.as_u32() * rr.as_s8();
                        case type::U8: return lr.as_u32() * rr.as_u8();
                        case type::S16: return lr.as_u32() * rr.as_s16();
                        case type::U16: return lr.as_u32() * rr.as_u16();
                        case type::WCHAR: return lr.as_u32() * rr.as_wchar();
                        case type::S32: return lr.as_u32() * rr.as_s32();
                        case type::U32: return lr.as_u32() * rr.as_u32();
                        case type::S64: return lr.as_u32() * rr.as_s64();
                        case type::U64: return lr.as_u32() * rr.as_u64();
                        case type::FLOAT: return lr.as_u32() * rr.as_float();
                        case type::DOUBLE: return lr.as_u32() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u32() * rr.as_long_double();
                        case type::VEC4: return lr.as_u32() * rr.as_vec4();
                        case type::MAT4: return lr.as_u32() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::uint32_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(rt) {
                        case type::BOOL: return lr.as_s64() * rr.as_bool();
                        case type::CHAR: return lr.as_s64() * rr.as_char();
                        case type::S8: return lr.as_s64() * rr.as_s8();
                        case type::U8: return lr.as_s64() * rr.as_u8();
                        case type::S16: return lr.as_s64() * rr.as_s16();
                        case type::U16: return lr.as_s64() * rr.as_u16();
                        case type::WCHAR: return lr.as_s64() * rr.as_wchar();
                        case type::S32: return lr.as_s64() * rr.as_s32();
                        case type::U32: return lr.as_s64() * rr.as_u32();
                        case type::S64: return lr.as_s64() * rr.as_s64();
                        case type::U64: return lr.as_s64() * rr.as_u64();
                        case type::FLOAT: return lr.as_s64() * rr.as_float();
                        case type::DOUBLE: return lr.as_s64() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_s64() * rr.as_long_double();
                        case type::VEC4: return lr.as_s64() * rr.as_vec4();
                        case type::MAT4: return lr.as_s64() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::int64_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(rt) {
                        case type::BOOL: return lr.as_u64() * rr.as_bool();
                        case type::CHAR: return lr.as_u64() * rr.as_char();
                        case type::S8: return lr.as_u64() * rr.as_s8();
                        case type::U8: return lr.as_u64() * rr.as_u8();
                        case type::S16: return lr.as_u64() * rr.as_s16();
                        case type::U16: return lr.as_u64() * rr.as_u16();
                        case type::WCHAR: return lr.as_u64() * rr.as_wchar();
                        case type::S32: return lr.as_u64() * rr.as_s32();
                        case type::U32: return lr.as_u64() * rr.as_u32();
                        case type::S64: return lr.as_u64() * rr.as_s64();
                        case type::U64: return lr.as_u64() * rr.as_u64();
                        case type::FLOAT: return lr.as_u64() * rr.as_float();
                        case type::DOUBLE: return lr.as_u64() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_u64() * rr.as_long_double();
                        case type::VEC4: return lr.as_u64() * rr.as_vec4();
                        case type::MAT4: return lr.as_u64() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return std::uint64_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(rt) {
                        case type::BOOL: return lr.as_float() * rr.as_bool();
                        case type::CHAR: return lr.as_float() * rr.as_char();
                        case type::S8: return lr.as_float() * rr.as_s8();
                        case type::U8: return lr.as_float() * rr.as_u8();
                        case type::S16: return lr.as_float() * rr.as_s16();
                        case type::U16: return lr.as_float() * rr.as_u16();
                        case type::WCHAR: return lr.as_float() * rr.as_wchar();
                        case type::S32: return lr.as_float() * rr.as_s32();
                        case type::U32: return lr.as_float() * rr.as_u32();
                        case type::S64: return lr.as_float() * rr.as_s64();
                        case type::U64: return lr.as_float() * rr.as_u64();
                        case type::FLOAT: return lr.as_float() * rr.as_float();
                        case type::DOUBLE: return lr.as_float() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_float() * rr.as_long_double();
                        case type::VEC4: return lr.as_float() * rr.as_vec4();
                        case type::MAT4: return lr.as_float() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return float{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_double() * rr.as_bool();
                        case type::CHAR: return lr.as_double() * rr.as_char();
                        case type::S8: return lr.as_double() * rr.as_s8();
                        case type::U8: return lr.as_double() * rr.as_u8();
                        case type::S16: return lr.as_double() * rr.as_s16();
                        case type::U16: return lr.as_double() * rr.as_u16();
                        case type::WCHAR: return lr.as_double() * rr.as_wchar();
                        case type::S32: return lr.as_double() * rr.as_s32();
                        case type::U32: return lr.as_double() * rr.as_u32();
                        case type::S64: return lr.as_double() * rr.as_s64();
                        case type::U64: return lr.as_double() * rr.as_u64();
                        case type::FLOAT: return lr.as_double() * rr.as_float();
                        case type::DOUBLE: return lr.as_double() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_double() * rr.as_long_double();
                        case type::VEC4: return lr.as_double() * rr.as_vec4();
                        case type::MAT4: return lr.as_double() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return double{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(rt) {
                        case type::BOOL: return lr.as_long_double() * rr.as_bool();
                        case type::CHAR: return lr.as_long_double() * rr.as_char();
                        case type::S8: return lr.as_long_double() * rr.as_s8();
                        case type::U8: return lr.as_long_double() * rr.as_u8();
                        case type::S16: return lr.as_long_double() * rr.as_s16();
                        case type::U16: return lr.as_long_double() * rr.as_u16();
                        case type::WCHAR: return lr.as_long_double() * rr.as_wchar();
                        case type::S32: return lr.as_long_double() * rr.as_s32();
                        case type::U32: return lr.as_long_double() * rr.as_u32();
                        case type::S64: return lr.as_long_double() * rr.as_s64();
                        case type::U64: return lr.as_long_double() * rr.as_u64();
                        case type::FLOAT: return lr.as_long_double() * rr.as_float();
                        case type::DOUBLE: return lr.as_long_double() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_long_double() * rr.as_long_double();
                        case type::VEC4: return lr.as_long_double() * rr.as_vec4();
                        case type::MAT4: return lr.as_long_double() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return static_cast<long double>(0);
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(rt) {
                        case type::BOOL: return lr.as_vec4() * rr.as_bool();
                        case type::CHAR: return lr.as_vec4() * rr.as_char();
                        case type::S8: return lr.as_vec4() * rr.as_s8();
                        case type::U8: return lr.as_vec4() * rr.as_u8();
                        case type::S16: return lr.as_vec4() * rr.as_s16();
                        case type::U16: return lr.as_vec4() * rr.as_u16();
                        case type::WCHAR: return lr.as_vec4() * rr.as_wchar();
                        case type::S32: return lr.as_vec4() * rr.as_s32();
                        case type::U32: return lr.as_vec4() * rr.as_u32();
                        case type::S64: return lr.as_vec4() * rr.as_s64();
                        case type::U64: return lr.as_vec4() * rr.as_u64();
                        case type::FLOAT: return lr.as_vec4() * rr.as_float();
                        case type::DOUBLE: return lr.as_vec4() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_vec4() * rr.as_long_double();
                        case type::VEC4: return lr.as_vec4() * rr.as_vec4();
                        case type::MAT4: return lr.as_vec4() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return vec4_t{};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(rt) {
                        case type::BOOL: return lr.as_mat4() * rr.as_bool();
                        case type::CHAR: return lr.as_mat4() * rr.as_char();
                        case type::S8: return lr.as_mat4() * rr.as_s8();
                        case type::U8: return lr.as_mat4() * rr.as_u8();
                        case type::S16: return lr.as_mat4() * rr.as_s16();
                        case type::U16: return lr.as_mat4() * rr.as_u16();
                        case type::WCHAR: return lr.as_mat4() * rr.as_wchar();
                        case type::S32: return lr.as_mat4() * rr.as_s32();
                        case type::U32: return lr.as_mat4() * rr.as_u32();
                        case type::S64: return lr.as_mat4() * rr.as_s64();
                        case type::U64: return lr.as_mat4() * rr.as_u64();
                        case type::FLOAT: return lr.as_mat4() * rr.as_float();
                        case type::DOUBLE: return lr.as_mat4() * rr.as_double();
                        case type::LONG_DOUBLE: return lr.as_mat4() * rr.as_long_double();
                        case type::VEC4: return lr.as_mat4() * rr.as_vec4();
                        case type::MAT4: return lr.as_mat4() * rr.as_mat4();
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return mat4_t{0};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::POINTER:
                    break;
                case type::CLASS:
                    break;
                case type::FUNC:
                    break;
                case type::ARRAY:
                    break;
                case type::OBJECT:
                    break;
                case type::STRING:
                    break;
                case type::WSTRING:
                    break;
                case type::UNDEFINED:
                    switch(rt) {
                        case type::BOOL: return false;
                        case type::CHAR: return char{0};
                        case type::S8: return std::int8_t{0};
                        case type::U8: return std::uint8_t{0};
                        case type::S16: return std::int16_t{0};
                        case type::U16: return std::uint16_t{0};
                        case type::WCHAR: return wchar_t{0};
                        case type::S32: return std::int32_t{0};
                        case type::U32: return std::uint32_t{0};
                        case type::S64: return std::int64_t{0};
                        case type::U64: return std::uint64_t{0};
                        case type::FLOAT: return float{0};
                        case type::DOUBLE: return double{0};
                        case type::LONG_DOUBLE: return static_cast<long double>(0);
                        case type::VEC4: return vec4_t{};
                        case type::MAT4: return mat4_t{};
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VALBOX:
                    break;
                default:
                    break;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator/(valbox const &lr, valbox const &rr) {
            auto lt{lr.val_or_pointed_type()};
            auto rt{rr.val_or_pointed_type()};
            switch(lt) {
                case type::BOOL:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_bool() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_bool() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_bool() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::CHAR:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_char() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_char() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_char() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S8:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_s8() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_s8() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_s8() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U8:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_u8() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_u8() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_u8() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S16:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_s16() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_s16() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_s16() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_u16() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_u16() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_u16() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_wchar() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_wchar() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_wchar() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_s32() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_s32() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_s32() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_u32() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_u32() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_u32() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_s64() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_s64() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_s64() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_u64() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_u64() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_u64() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_float() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_float() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_float() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_float() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_double() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_double() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_double() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_double() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_long_double() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_long_double() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_long_double() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_long_double() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_vec4() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_vec4() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_vec4() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_vec4() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_mat4() / dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return lr.as_mat4() / dvzr; }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return lr.as_mat4() / dvzr; }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return lr.as_mat4() / dvzr; }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::POINTER:
                    break;
                case type::CLASS:
                    break;
                case type::FUNC:
                    break;
                case type::ARRAY:
                    break;
                case type::OBJECT:
                    break;
                case type::STRING:
                    break;
                case type::WSTRING:
                    break;
                case type::UNDEFINED:
                    return valbox{valbox_no_initialize::dont_do_it};
                    break;
                case type::VALBOX:
                    break;
                default:
                    break;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator%(valbox const &lr, valbox const &rr) {
            auto lt{lr.val_or_pointed_type()};
            auto rt{rr.val_or_pointed_type()};
            switch(lt) {
                case type::BOOL:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_bool() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_bool(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_bool(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_bool(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::CHAR:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_char() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_char(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_char(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_char(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S8:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s8() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_s8(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_s8(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_s8(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U8:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u8() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_u8(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_u8(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_u8(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S16:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s16() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_s16(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_s16(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_s16(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u16() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_u16(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_u16(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_u16(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_wchar() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_wchar(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_wchar(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_wchar(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s32() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_s32(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_s32(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_s32(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u32() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_u32(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_u32(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_u32(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::S64: {
                            auto dvzr{rr.as_s64()};
                            if(dvzr == 0) {
                                throw std::runtime_error{"integer divizion by zero"};
                            }
                            return lr.as_s64() % dvzr;
                        }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_s64() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_s64(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_s64(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_s64(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(rt) {
                        case type::BOOL: { auto dvzr{rr.as_bool()}; if(!dvzr) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::CHAR: { auto dvzr{rr.as_char()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::S8: { auto dvzr{rr.as_s8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::U8: { auto dvzr{rr.as_u8()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::S16: { auto dvzr{rr.as_s16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::U16: { auto dvzr{rr.as_u16()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::WCHAR: { auto dvzr{rr.as_wchar()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::S32: { auto dvzr{rr.as_s32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::U32: { auto dvzr{rr.as_u32()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::S64: { auto dvzr{rr.as_s64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::U64: { auto dvzr{rr.as_u64()}; if(dvzr == 0) { throw std::runtime_error{"integer divizion by zero"}; } return lr.as_u64() % dvzr; }
                        case type::FLOAT: { auto dvzr{rr.as_float()}; return std::fmod(lr.as_u64(), dvzr); }
                        case type::DOUBLE: { auto dvzr{rr.as_double()}; return std::fmod(lr.as_u64(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_u64(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(rt) {
                        case type::BOOL:        { auto dvzr{rr.as_bool()};        return std::fmod(lr.as_float(), dvzr); }
                        case type::CHAR:        { auto dvzr{rr.as_char()};        return std::fmod(lr.as_float(), dvzr); }
                        case type::S8:          { auto dvzr{rr.as_s8()};          return std::fmod(lr.as_float(), dvzr); }
                        case type::U8:          { auto dvzr{rr.as_u8()};          return std::fmod(lr.as_float(), dvzr); }
                        case type::S16:         { auto dvzr{rr.as_s16()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::U16:         { auto dvzr{rr.as_u16()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::WCHAR:       { auto dvzr{rr.as_wchar()};       return std::fmod(lr.as_float(), dvzr); }
                        case type::S32:         { auto dvzr{rr.as_s32()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::U32:         { auto dvzr{rr.as_u32()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::S64:         { auto dvzr{rr.as_s64()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::U64:         { auto dvzr{rr.as_u64()};         return std::fmod(lr.as_float(), dvzr); }
                        case type::FLOAT:       { auto dvzr{rr.as_float()};       return std::fmod(lr.as_float(), dvzr); }
                        case type::DOUBLE:      { auto dvzr{rr.as_double()};      return std::fmod(lr.as_float(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_float(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(rt) {
                        case type::BOOL:        { auto dvzr{rr.as_bool()};        return std::fmod(lr.as_double(), dvzr); }
                        case type::CHAR:        { auto dvzr{rr.as_char()};        return std::fmod(lr.as_double(), dvzr); }
                        case type::S8:          { auto dvzr{rr.as_s8()};          return std::fmod(lr.as_double(), dvzr); }
                        case type::U8:          { auto dvzr{rr.as_u8()};          return std::fmod(lr.as_double(), dvzr); }
                        case type::S16:         { auto dvzr{rr.as_s16()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U16:         { auto dvzr{rr.as_u16()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::WCHAR:       { auto dvzr{rr.as_wchar()};       return std::fmod(lr.as_double(), dvzr); }
                        case type::S32:         { auto dvzr{rr.as_s32()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U32:         { auto dvzr{rr.as_u32()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::S64:         { auto dvzr{rr.as_s64()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U64:         { auto dvzr{rr.as_u64()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::FLOAT:       { auto dvzr{rr.as_float()};       return std::fmod(lr.as_double(), dvzr); }
                        case type::DOUBLE:      { auto dvzr{rr.as_double()};      return std::fmod(lr.as_double(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_double(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(rt) {
                        case type::BOOL:        { auto dvzr{rr.as_bool()};        return std::fmod(lr.as_double(), dvzr); }
                        case type::CHAR:        { auto dvzr{rr.as_char()};        return std::fmod(lr.as_double(), dvzr); }
                        case type::S8:          { auto dvzr{rr.as_s8()};          return std::fmod(lr.as_double(), dvzr); }
                        case type::U8:          { auto dvzr{rr.as_u8()};          return std::fmod(lr.as_double(), dvzr); }
                        case type::S16:         { auto dvzr{rr.as_s16()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U16:         { auto dvzr{rr.as_u16()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::WCHAR:       { auto dvzr{rr.as_wchar()};       return std::fmod(lr.as_double(), dvzr); }
                        case type::S32:         { auto dvzr{rr.as_s32()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U32:         { auto dvzr{rr.as_u32()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::S64:         { auto dvzr{rr.as_s64()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::U64:         { auto dvzr{rr.as_u64()};         return std::fmod(lr.as_double(), dvzr); }
                        case type::FLOAT:       { auto dvzr{rr.as_float()};       return std::fmod(lr.as_double(), dvzr); }
                        case type::DOUBLE:      { auto dvzr{rr.as_double()};      return std::fmod(lr.as_double(), dvzr); }
                        case type::LONG_DOUBLE: { auto dvzr{rr.as_long_double()}; return std::fmod(lr.as_double(), dvzr); }
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: return valbox{valbox_no_initialize::dont_do_it};
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(rt) {
                        case type::BOOL: break;
                        case type::CHAR: break;
                        case type::S8: break;
                        case type::U8: break;
                        case type::S16: break;
                        case type::U16: break;
                        case type::WCHAR: break;
                        case type::S32: break;
                        case type::U32: break;
                        case type::S64: break;
                        case type::U64: break;
                        case type::FLOAT: break;
                        case type::DOUBLE: break;
                        case type::LONG_DOUBLE: break;
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: break;
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(rt) {
                        case type::BOOL: break;
                        case type::CHAR: break;
                        case type::S8: break;
                        case type::U8: break;
                        case type::S16: break;
                        case type::U16: break;
                        case type::WCHAR: break;
                        case type::S32: break;
                        case type::U32: break;
                        case type::S64: break;
                        case type::U64: break;
                        case type::FLOAT: break;
                        case type::DOUBLE: break;
                        case type::LONG_DOUBLE: break;
                        case type::VEC4: break;
                        case type::MAT4: break;
                        case type::POINTER: break;
                        case type::CLASS: break;
                        case type::FUNC: break;
                        case type::ARRAY: break;
                        case type::OBJECT: break;
                        case type::STRING: break;
                        case type::WSTRING: break;
                        case type::UNDEFINED: break;
                        case type::VALBOX: break;
                        default: break;
                    }
                    break;
                case type::POINTER:
                    break;
                case type::CLASS:
                    break;
                case type::FUNC:
                    break;
                case type::ARRAY:
                    break;
                case type::OBJECT:
                    break;
                case type::STRING:
                    break;
                case type::WSTRING:
                    break;
                case type::UNDEFINED:
                    return valbox{valbox_no_initialize::dont_do_it};
                    break;
                case type::VALBOX:
                    break;
                default:
                    break;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator<<(valbox const &l, valbox const &r) {
            valbox res{valbox_no_initialize::dont_do_it};
            valbox const &lr{l.deref()};
            valbox const &rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                return res;
            }
            if(lr.is_numeric() && rr.is_numeric()) {
                res.become_same_type_as(lr);
                switch(lt) {
                    case type::BOOL: res.assign_preserving_type(lr.as_bool() && !rr.cast_to_bool()); break;
                    case type::CHAR: res.assign_preserving_type(lr.as_char() << rr.cast_to_u64()); break;
                    case type::WCHAR: res.assign_preserving_type(lr.as_wchar() << rr.cast_to_u64()); break;
                    case type::U8: res.assign_preserving_type(lr.as_u8() << rr.cast_to_u64()); break;
                    case type::S8: res.assign_preserving_type(lr.as_s8() << rr.cast_to_u64()); break;
                    case type::U16: res.assign_preserving_type(lr.as_u16() << rr.cast_to_u64()); break;
                    case type::S16: res.assign_preserving_type(lr.as_s16() << rr.cast_to_u64()); break;
                    case type::U32: res.assign_preserving_type(lr.as_u32() << rr.cast_to_u64()); break;
                    case type::S32: res.assign_preserving_type(lr.as_s32() << rr.cast_to_u64()); break;
                    case type::U64: res.assign_preserving_type(lr.as_u64() << rr.cast_to_u64()); break;
                    case type::S64: res.assign_preserving_type(lr.as_s64() << rr.cast_to_u64()); break;
                    default: throw std::runtime_error{"operation not applicable"};
                }
                return res;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator>>(valbox const &l, valbox const &r) {
            valbox res{valbox_no_initialize::dont_do_it};
            valbox const &lr{l.deref()};
            valbox const &rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                return res;
            }
            if(lr.is_numeric() && rr.is_numeric()) {
                res.become_same_type_as(lr);
                switch(lt) {
                    case type::BOOL: res.assign_preserving_type(lr.as_bool() && !rr.cast_to_bool()); break;
                    case type::CHAR: res.assign_preserving_type(lr.as_char() >> rr.cast_to_u64()); break;
                    case type::WCHAR: res.assign_preserving_type(lr.as_wchar() >> rr.cast_to_u64()); break;
                    case type::U8: res.assign_preserving_type(lr.as_u8() >> rr.cast_to_u64()); break;
                    case type::S8: res.assign_preserving_type(lr.as_s8() >> rr.cast_to_u64()); break;
                    case type::U16: res.assign_preserving_type(lr.as_u16() >> rr.cast_to_u64()); break;
                    case type::S16: res.assign_preserving_type(lr.as_s16() >> rr.cast_to_u64()); break;
                    case type::U32: res.assign_preserving_type(lr.as_u32() >> rr.cast_to_u64()); break;
                    case type::S32: res.assign_preserving_type(lr.as_s32() >> rr.cast_to_u64()); break;
                    case type::U64: res.assign_preserving_type(lr.as_u64() >> rr.cast_to_u64()); break;
                    case type::S64: res.assign_preserving_type(lr.as_s64() >> rr.cast_to_u64()); break;
                    default: throw std::runtime_error{"operation not applicable"};
                }
                return res;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator&(valbox const &l, valbox const &r) {
            valbox res{valbox_no_initialize::dont_do_it};
            type st;
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                return res;
            }
            bool applicable{stronger_numeric_type(lt, rt, st)};
            if(applicable) {
                if(st == lt) {
                    res.become_same_type_as(lr);
                } else {
                    res.become_same_type_as(rr);
                }
                switch(st) {
                    case type::CHAR: res.assign_preserving_type(lr.cast_num_to_num<char>() & rr.cast_num_to_num<char>()); break;
                    case type::WCHAR:res.assign_preserving_type(lr.cast_num_to_num<wchar_t>() & rr.cast_num_to_num<wchar_t>()); break;
                    case type::U8:   res.assign_preserving_type(lr.cast_num_to_num<std::uint8_t>() & rr.cast_num_to_num<std::uint8_t>()); break;
                    case type::S8:   res.assign_preserving_type(lr.cast_num_to_num<std::int8_t>() & rr.cast_num_to_num<std::int8_t>()); break;
                    case type::U16:  res.assign_preserving_type(lr.cast_num_to_num<std::uint16_t>() & rr.cast_num_to_num<std::uint16_t>()); break;
                    case type::S16:  res.assign_preserving_type(lr.cast_num_to_num<std::int16_t>() & rr.cast_num_to_num<std::int16_t>()); break;
                    case type::U32:  res.assign_preserving_type(lr.cast_num_to_num<std::uint32_t>() & rr.cast_num_to_num<std::uint32_t>()); break;
                    case type::S32:  res.assign_preserving_type(lr.cast_num_to_num<std::int32_t>() & rr.cast_num_to_num<std::int32_t>()); break;
                    case type::U64:  res.assign_preserving_type(lr.cast_num_to_num<std::uint64_t>() & rr.cast_num_to_num<std::uint64_t>()); break;
                    case type::S64:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() & rr.cast_num_to_num<std::int64_t>()); break;
                    case type::BOOL:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() & rr.cast_num_to_num<std::int64_t>()); break;
                    default: throw std::runtime_error{"operation not applicable"};
                }
                return res;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator|(valbox const &l, valbox const &r) {
            valbox res{valbox_no_initialize::dont_do_it};
            type st;
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                return res;
            }
            bool applicable{stronger_numeric_type(lt, rt, st)};
            if(applicable) {
                if(st == lt) {
                    res.become_same_type_as(lr);
                } else {
                    res.become_same_type_as(rr);
                }
                switch(st) {
                    case type::CHAR: res.assign_preserving_type(lr.cast_num_to_num<char>() | rr.cast_num_to_num<char>()); break;
                    case type::WCHAR:res.assign_preserving_type(lr.cast_num_to_num<wchar_t>() | rr.cast_num_to_num<wchar_t>()); break;
                    case type::U8:   res.assign_preserving_type(lr.cast_num_to_num<std::uint8_t>() | rr.cast_num_to_num<std::uint8_t>()); break;
                    case type::S8:   res.assign_preserving_type(lr.cast_num_to_num<std::int8_t>() | rr.cast_num_to_num<std::int8_t>()); break;
                    case type::U16:  res.assign_preserving_type(lr.cast_num_to_num<std::uint16_t>() | rr.cast_num_to_num<std::uint16_t>()); break;
                    case type::S16:  res.assign_preserving_type(lr.cast_num_to_num<std::int16_t>() | rr.cast_num_to_num<std::int16_t>()); break;
                    case type::U32:  res.assign_preserving_type(lr.cast_num_to_num<std::uint32_t>() | rr.cast_num_to_num<std::uint32_t>()); break;
                    case type::S32:  res.assign_preserving_type(lr.cast_num_to_num<std::int32_t>() | rr.cast_num_to_num<std::int32_t>()); break;
                    case type::U64:  res.assign_preserving_type(lr.cast_num_to_num<std::uint64_t>() | rr.cast_num_to_num<std::uint64_t>()); break;
                    case type::S64:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() | rr.cast_num_to_num<std::int64_t>()); break;
                    case type::BOOL:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() | rr.cast_num_to_num<std::int64_t>()); break;
                    default: throw std::runtime_error{"operation not applicable"};
                }
                return res;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        friend valbox operator^(valbox const &l, valbox const &r) {
            valbox res{valbox_no_initialize::dont_do_it};
            type st;
            auto lr{l.deref()};
            auto rr{r.deref()};
            type lt{lr.val_or_pointed_type()};
            type rt{rr.val_or_pointed_type()};
            if(lt == type::UNDEFINED || rt == type::UNDEFINED) {
                return res;
            }
            bool applicable{stronger_numeric_type(lt, rt, st)};
            if(applicable) {
                if(st == lt) {
                    res.become_same_type_as(lr);
                } else {
                    res.become_same_type_as(rr);
                }
                switch(st) {
                    case type::CHAR: res.assign_preserving_type(lr.cast_num_to_num<char>() ^ rr.cast_num_to_num<char>()); break;
                    case type::WCHAR:res.assign_preserving_type(lr.cast_num_to_num<wchar_t>() ^ rr.cast_num_to_num<wchar_t>()); break;
                    case type::U8:   res.assign_preserving_type(lr.cast_num_to_num<std::uint8_t>() ^ rr.cast_num_to_num<std::uint8_t>()); break;
                    case type::S8:   res.assign_preserving_type(lr.cast_num_to_num<std::int8_t>() ^ rr.cast_num_to_num<std::int8_t>()); break;
                    case type::U16:  res.assign_preserving_type(lr.cast_num_to_num<std::uint16_t>() ^ rr.cast_num_to_num<std::uint16_t>()); break;
                    case type::S16:  res.assign_preserving_type(lr.cast_num_to_num<std::int16_t>() ^ rr.cast_num_to_num<std::int16_t>()); break;
                    case type::U32:  res.assign_preserving_type(lr.cast_num_to_num<std::uint32_t>() ^ rr.cast_num_to_num<std::uint32_t>()); break;
                    case type::S32:  res.assign_preserving_type(lr.cast_num_to_num<std::int32_t>() ^ rr.cast_num_to_num<std::int32_t>()); break;
                    case type::U64:  res.assign_preserving_type(lr.cast_num_to_num<std::uint64_t>() ^ rr.cast_num_to_num<std::uint64_t>()); break;
                    case type::S64:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() ^ rr.cast_num_to_num<std::int64_t>()); break;
                    case type::BOOL:  res.assign_preserving_type(lr.cast_num_to_num<std::int64_t>() ^ rr.cast_num_to_num<std::int64_t>()); break;
                    default: throw std::runtime_error{"operation not applicable"};
                }
                return res;
            }
            throw std::runtime_error{"operation not applicable"};
        }

        valbox &assign_preserving_type(valbox const &that) {
            valbox &thisref{deref()};
            valbox const &thatref{that.deref()};
            auto thist{thisref.val_or_pointed_type()};
            auto thatt{thatref.val_or_pointed_type()};
            switch(thist) {
                case type::BOOL:
                    switch(thatt) {
                        case type::BOOL: as_bool() = thatref.as_bool(); break;
                        case type::CHAR: as_bool() = thatref.as_char(); break;
                        case type::S8: as_bool() = thatref.as_s8(); break;
                        case type::U8: as_bool() = thatref.as_u8(); break;
                        case type::S16: as_bool() = thatref.as_s16(); break;
                        case type::U16: as_bool() = thatref.as_u16(); break;
                        case type::WCHAR: as_bool() = thatref.as_wchar(); break;
                        case type::S32: as_bool() = thatref.as_s32(); break;
                        case type::U32: as_bool() = thatref.as_u32(); break;
                        case type::S64: as_bool() = thatref.as_s64(); break;
                        case type::U64: as_bool() = thatref.as_u64(); break;
                        case type::FLOAT: as_bool() = thatref.as_float(); break;
                        case type::DOUBLE: as_bool() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_bool() = thatref.as_long_double(); break;
                        case type::VEC4: as_bool() = !thatref.as_vec4().is_zero(); break;
                        case type::MAT4: as_bool() = !thatref.as_mat4().is_zero(); break;
                        case type::POINTER: as_bool() = thatref.as_ptr() != nullptr; break;
                        case type::CLASS: throw std::runtime_error{"inappropriate value"};
                        case type::FUNC: throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY: as_bool() = !thatref.as_array().empty(); break;
                        case type::OBJECT: as_bool() = !thatref.as_object().empty(); break;
                        case type::STRING: as_bool() = !thatref.as_string().empty(); break;
                        case type::WSTRING: as_bool() = !thatref.as_wstring().empty(); break;
                        case type::UNDEFINED: as_bool() = false; break;
                        case type::VALBOX: throw std::runtime_error{"inappropriate value"};
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::CHAR:
                    switch(thatt) {
                        case type::BOOL: as_char() = thatref.as_bool(); break;
                        case type::CHAR: as_char() = thatref.as_char(); break;
                        case type::S8: as_char() = thatref.as_s8(); break;
                        case type::U8: as_char() = thatref.as_u8(); break;
                        case type::S16: as_char() = thatref.as_s16(); break;
                        case type::U16: as_char() = thatref.as_u16(); break;
                        case type::WCHAR: as_char() = thatref.as_wchar(); break;
                        case type::S32: as_char() = thatref.as_s32(); break;
                        case type::U32: as_char() = thatref.as_u32(); break;
                        case type::S64: as_char() = thatref.as_s64(); break;
                        case type::U64: as_char() = thatref.as_u64(); break;
                        case type::FLOAT: as_char() = thatref.as_float(); break;
                        case type::DOUBLE: as_char() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_char() = thatref.as_long_double(); break;
                        case type::VEC4: as_char() = thatref.as_vec4().x(); break;
                        case type::MAT4: as_char() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER: as_char() = static_cast<char>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS: throw std::runtime_error{"inappropriate value"};
                        case type::FUNC: throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY: as_char() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_char(); break;
                        case type::OBJECT: as_char() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_char(); break;
                        case type::STRING: as_char() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING: as_char() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED: as_char() = 0; break;
                        case type::VALBOX: as_char() = static_cast<char>(type::VALBOX); break;
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::S8:
                    switch(thatt) {
                        case type::BOOL: as_s8() = thatref.as_bool(); break;
                        case type::CHAR: as_s8() = thatref.as_char(); break;
                        case type::S8: as_s8() = thatref.as_s8(); break;
                        case type::U8: as_s8() = thatref.as_u8(); break;
                        case type::S16: as_s8() = thatref.as_s16(); break;
                        case type::U16: as_s8() = thatref.as_u16(); break;
                        case type::WCHAR: as_s8() = thatref.as_wchar(); break;
                        case type::S32: as_s8() = thatref.as_s32(); break;
                        case type::U32: as_s8() = thatref.as_u32(); break;
                        case type::S64: as_s8() = thatref.as_s64(); break;
                        case type::U64: as_s8() = thatref.as_u64(); break;
                        case type::FLOAT: as_s8() = thatref.as_float(); break;
                        case type::DOUBLE: as_s8() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s8() = thatref.as_long_double(); break;
                        case type::VEC4: as_s8() = thatref.as_vec4().x(); break;
                        case type::MAT4: as_s8() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER: as_s8() = static_cast<int8_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS: throw std::runtime_error{"inappropriate value"};
                        case type::FUNC: throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY: as_s8() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_s8(); break;
                        case type::OBJECT: as_s8() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_s8(); break;
                        case type::STRING: as_s8() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING: as_s8() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED: as_s8() = 0; break;
                        case type::VALBOX: throw std::runtime_error{"inappropriate value"};
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::U8:
                    switch(thatt) {
                        case type::BOOL: as_u8() = thatref.as_bool(); break;
                        case type::CHAR: as_u8() = thatref.as_char(); break;
                        case type::S8: as_u8() = thatref.as_s8(); break;
                        case type::U8: as_u8() = thatref.as_u8(); break;
                        case type::S16: as_u8() = thatref.as_s16(); break;
                        case type::U16: as_u8() = thatref.as_u16(); break;
                        case type::WCHAR: as_u8() = thatref.as_wchar(); break;
                        case type::S32: as_u8() = thatref.as_s32(); break;
                        case type::U32: as_u8() = thatref.as_u32(); break;
                        case type::S64: as_u8() = thatref.as_s64(); break;
                        case type::U64: as_u8() = thatref.as_u64(); break;
                        case type::FLOAT: as_u8() = thatref.as_float(); break;
                        case type::DOUBLE: as_u8() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u8() = thatref.as_long_double(); break;
                        case type::VEC4: as_u8() = thatref.as_vec4().x(); break;
                        case type::MAT4: as_u8() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER: as_u8() = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS: throw std::runtime_error{"inappropriate value"};
                        case type::FUNC: throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY: as_u8() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_u8(); break;
                        case type::OBJECT: as_u8() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_u8(); break;
                        case type::STRING: as_u8() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING: as_u8() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED: as_u8() = 0; break;
                        case type::VALBOX: throw std::runtime_error{"inappropriate value"};
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::S16:
                    switch(thatt) {
                        case type::BOOL: as_s16() = thatref.as_bool(); break;
                        case type::CHAR: as_s16() = thatref.as_char(); break;
                        case type::S8: as_s16() = thatref.as_s8(); break;
                        case type::U8: as_s16() = thatref.as_u8(); break;
                        case type::S16: as_s16() = thatref.as_s16(); break;
                        case type::U16: as_s16() = thatref.as_u16(); break;
                        case type::WCHAR: as_s16() = thatref.as_wchar(); break;
                        case type::S32: as_s16() = thatref.as_s32(); break;
                        case type::U32: as_s16() = thatref.as_u32(); break;
                        case type::S64: as_s16() = thatref.as_s64(); break;
                        case type::U64: as_s16() = thatref.as_u64(); break;
                        case type::FLOAT: as_s16() = thatref.as_float(); break;
                        case type::DOUBLE: as_s16() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s16() = thatref.as_long_double(); break;
                        case type::VEC4: as_s16() = thatref.as_vec4().x(); break;
                        case type::MAT4: as_s16() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER: as_s16() = static_cast<int16_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS: as_s16() = static_cast<int16_t>(type::CLASS); break;
                        case type::FUNC: as_s16() = static_cast<int16_t>(type::FUNC); break;
                        case type::ARRAY: as_s16() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_s16(); break;
                        case type::OBJECT: as_s16() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_s16(); break;
                        case type::STRING: as_s16() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING: as_s16() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED: as_s16() = 0; break;
                        case type::VALBOX: as_s16() = static_cast<int16_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::U16:
                    switch(thatt) {
                        case type::BOOL: as_u16() = thatref.as_bool(); break;
                        case type::CHAR: as_u16() = thatref.as_char(); break;
                        case type::S8: as_u16() = thatref.as_s8(); break;
                        case type::U8: as_u16() = thatref.as_u8(); break;
                        case type::S16: as_u16() = thatref.as_s16(); break;
                        case type::U16: as_u16() = thatref.as_u16(); break;
                        case type::WCHAR: as_u16() = thatref.as_wchar(); break;
                        case type::S32: as_u16() = thatref.as_s32(); break;
                        case type::U32: as_u16() = thatref.as_u32(); break;
                        case type::S64: as_u16() = thatref.as_s64(); break;
                        case type::U64: as_u16() = thatref.as_u64(); break;
                        case type::FLOAT: as_u16() = thatref.as_float(); break;
                        case type::DOUBLE: as_u16() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u16() = thatref.as_long_double(); break;
                        case type::VEC4: as_u16() = thatref.as_vec4().x(); break;
                        case type::MAT4: as_u16() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER: as_u16() = static_cast<uint16_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS: as_u16() = static_cast<uint16_t>(type::CLASS); break;
                        case type::FUNC: as_u16() = static_cast<uint16_t>(type::FUNC); break;
                        case type::ARRAY: as_u16() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_u16(); break;
                        case type::OBJECT: as_u16() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_u16(); break;
                        case type::STRING: as_u16() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING: as_u16() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED: as_u16() = 0; break;
                        case type::VALBOX: as_u16() = static_cast<uint16_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::WCHAR:
                    switch(thatt) {
                        case type::BOOL:        as_wchar() = thatref.as_bool(); break;
                        case type::CHAR:        as_wchar() = thatref.as_char(); break;
                        case type::S8:          as_wchar() = thatref.as_s8(); break;
                        case type::U8:          as_wchar() = thatref.as_u8(); break;
                        case type::S16:         as_wchar() = thatref.as_s16(); break;
                        case type::U16:         as_wchar() = thatref.as_u16(); break;
                        case type::WCHAR:       as_wchar() = thatref.as_wchar(); break;
                        case type::S32:         as_wchar() = thatref.as_s32(); break;
                        case type::U32:         as_wchar() = thatref.as_u32(); break;
                        case type::S64:         as_wchar() = thatref.as_s64(); break;
                        case type::U64:         as_wchar() = thatref.as_u64(); break;
                        case type::FLOAT:       as_wchar() = thatref.as_float(); break;
                        case type::DOUBLE:      as_wchar() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_wchar() = thatref.as_long_double(); break;
                        case type::VEC4:        as_wchar() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_wchar() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_wchar() = static_cast<wchar_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS:       as_wchar() = static_cast<wchar_t>(type::CLASS); break;
                        case type::FUNC:        as_wchar() = static_cast<wchar_t>(type::FUNC); break;
                        case type::ARRAY:       as_wchar() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_wchar(); break;
                        case type::OBJECT:      as_wchar() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_wchar(); break;
                        case type::STRING:      as_wchar() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_wchar() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_wchar() = 0; break;
                        case type::VALBOX:      as_wchar() = static_cast<wchar_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::S32:
                    switch(thatt) {
                        case type::BOOL:        as_s32() = thatref.as_bool(); break;
                        case type::CHAR:        as_s32() = thatref.as_char(); break;
                        case type::S8:          as_s32() = thatref.as_s8(); break;
                        case type::U8:          as_s32() = thatref.as_u8(); break;
                        case type::S16:         as_s32() = thatref.as_s16(); break;
                        case type::U16:         as_s32() = thatref.as_u16(); break;
                        case type::WCHAR:       as_s32() = thatref.as_wchar(); break;
                        case type::S32:         as_s32() = thatref.as_s32(); break;
                        case type::U32:         as_s32() = thatref.as_u32(); break;
                        case type::S64:         as_s32() = thatref.as_s64(); break;
                        case type::U64:         as_s32() = thatref.as_u64(); break;
                        case type::FLOAT:       as_s32() = thatref.as_float(); break;
                        case type::DOUBLE:      as_s32() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s32() = thatref.as_long_double(); break;
                        case type::VEC4:        as_s32() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_s32() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_s32() = static_cast<int32_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS:       as_s32() = static_cast<int32_t>(type::CLASS); break;
                        case type::FUNC:        as_s32() = static_cast<int32_t>(type::FUNC); break;
                        case type::ARRAY:       as_s32() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_s32(); break;
                        case type::OBJECT:      as_s32() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_s32(); break;
                        case type::STRING:      as_s32() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_s32() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_s32() = 0; break;
                        case type::VALBOX:      as_s32() = static_cast<int32_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::U32:
                    switch(thatt) {
                        case type::BOOL:        as_u32() = thatref.as_bool(); break;
                        case type::CHAR:        as_u32() = thatref.as_char(); break;
                        case type::S8:          as_u32() = thatref.as_s8(); break;
                        case type::U8:          as_u32() = thatref.as_u8(); break;
                        case type::S16:         as_u32() = thatref.as_s16(); break;
                        case type::U16:         as_u32() = thatref.as_u16(); break;
                        case type::WCHAR:       as_u32() = thatref.as_wchar(); break;
                        case type::S32:         as_u32() = thatref.as_s32(); break;
                        case type::U32:         as_u32() = thatref.as_u32(); break;
                        case type::S64:         as_u32() = thatref.as_s64(); break;
                        case type::U64:         as_u32() = thatref.as_u64(); break;
                        case type::FLOAT:       as_u32() = thatref.as_float(); break;
                        case type::DOUBLE:      as_u32() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u32() = thatref.as_long_double(); break;
                        case type::VEC4:        as_u32() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_u32() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_u32() = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS:       as_u32() = static_cast<uint32_t>(type::CLASS); break;
                        case type::FUNC:        as_u32() = static_cast<uint32_t>(type::FUNC); break;
                        case type::ARRAY:       as_u32() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_u32(); break;
                        case type::OBJECT:      as_u32() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_u32(); break;
                        case type::STRING:      as_u32() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_u32() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_u32() = 0; break;
                        case type::VALBOX:      as_u32() = static_cast<uint32_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::S64:
                    switch(thatt) {
                        case type::BOOL:        as_s64() = thatref.as_bool(); break;
                        case type::CHAR:        as_s64() = thatref.as_char(); break;
                        case type::S8:          as_s64() = thatref.as_s8(); break;
                        case type::U8:          as_s64() = thatref.as_u8(); break;
                        case type::S16:         as_s64() = thatref.as_s16(); break;
                        case type::U16:         as_s64() = thatref.as_u16(); break;
                        case type::WCHAR:       as_s64() = thatref.as_wchar(); break;
                        case type::S32:         as_s64() = thatref.as_s32(); break;
                        case type::U32:         as_s64() = thatref.as_u32(); break;
                        case type::S64:         as_s64() = thatref.as_s64(); break;
                        case type::U64:         as_s64() = thatref.as_u64(); break;
                        case type::FLOAT:       as_s64() = thatref.as_float(); break;
                        case type::DOUBLE:      as_s64() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_s64() = thatref.as_long_double(); break;
                        case type::VEC4:        as_s64() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_s64() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_s64() = static_cast<int64_t>(reinterpret_cast<uintptr_t>(thatref.as_ptr())); break;
                        case type::CLASS:       as_s64() = static_cast<int64_t>(type::CLASS); break;
                        case type::FUNC:        as_s64() = static_cast<int64_t>(type::FUNC); break;
                        case type::ARRAY:       as_s64() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_s64(); break;
                        case type::OBJECT:      as_s64() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_s64(); break;
                        case type::STRING:      as_s64() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_s64() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_s64() = 0; break;
                        case type::VALBOX:      as_s64() = static_cast<int64_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::U64:
                    switch(thatt) {
                        case type::BOOL:        as_u64() = thatref.as_bool(); break;
                        case type::CHAR:        as_u64() = thatref.as_char(); break;
                        case type::S8:          as_u64() = thatref.as_s8(); break;
                        case type::U8:          as_u64() = thatref.as_u8(); break;
                        case type::S16:         as_u64() = thatref.as_s16(); break;
                        case type::U16:         as_u64() = thatref.as_u16(); break;
                        case type::WCHAR:       as_u64() = thatref.as_wchar(); break;
                        case type::S32:         as_u64() = thatref.as_s32(); break;
                        case type::U32:         as_u64() = thatref.as_u32(); break;
                        case type::S64:         as_u64() = thatref.as_s64(); break;
                        case type::U64:         as_u64() = thatref.as_u64(); break;
                        case type::FLOAT:       as_u64() = thatref.as_float(); break;
                        case type::DOUBLE:      as_u64() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_u64() = thatref.as_long_double(); break;
                        case type::VEC4:        as_u64() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_u64() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_u64() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_u64() = static_cast<uint64_t>(type::CLASS); break;
                        case type::FUNC:        as_u64() = static_cast<uint64_t>(type::FUNC); break;
                        case type::ARRAY:       as_u64() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_u64(); break;
                        case type::OBJECT:      as_u64() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_u64(); break;
                        case type::STRING:      as_u64() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_u64() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_u64() = 0; break;
                        case type::VALBOX:      as_u64() = static_cast<uint64_t>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::FLOAT:
                    switch(thatt) {
                        case type::BOOL:        as_float() = thatref.as_bool(); break;
                        case type::CHAR:        as_float() = thatref.as_char(); break;
                        case type::S8:          as_float() = thatref.as_s8(); break;
                        case type::U8:          as_float() = thatref.as_u8(); break;
                        case type::S16:         as_float() = thatref.as_s16(); break;
                        case type::U16:         as_float() = thatref.as_u16(); break;
                        case type::WCHAR:       as_float() = thatref.as_wchar(); break;
                        case type::S32:         as_float() = thatref.as_s32(); break;
                        case type::U32:         as_float() = thatref.as_u32(); break;
                        case type::S64:         as_float() = thatref.as_s64(); break;
                        case type::U64:         as_float() = thatref.as_u64(); break;
                        case type::FLOAT:       as_float() = thatref.as_float(); break;
                        case type::DOUBLE:      as_float() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_float() = thatref.as_long_double(); break;
                        case type::VEC4:        as_float() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_float() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_float() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_float() = static_cast<float>(type::CLASS); break;
                        case type::FUNC:        as_float() = static_cast<float>(type::FUNC); break;
                        case type::ARRAY:       as_float() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_float(); break;
                        case type::OBJECT:      as_float() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_float(); break;
                        case type::STRING:      as_float() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_float() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_float() = 0; break;
                        case type::VALBOX:      as_float() = static_cast<float>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::DOUBLE:
                    switch(thatt) {
                        case type::BOOL:        as_double() = thatref.as_bool(); break;
                        case type::CHAR:        as_double() = thatref.as_char(); break;
                        case type::S8:          as_double() = thatref.as_s8(); break;
                        case type::U8:          as_double() = thatref.as_u8(); break;
                        case type::S16:         as_double() = thatref.as_s16(); break;
                        case type::U16:         as_double() = thatref.as_u16(); break;
                        case type::WCHAR:       as_double() = thatref.as_wchar(); break;
                        case type::S32:         as_double() = thatref.as_s32(); break;
                        case type::U32:         as_double() = thatref.as_u32(); break;
                        case type::S64:         as_double() = thatref.as_s64(); break;
                        case type::U64:         as_double() = thatref.as_u64(); break;
                        case type::FLOAT:       as_double() = thatref.as_float(); break;
                        case type::DOUBLE:      as_double() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_double() = thatref.as_long_double(); break;
                        case type::VEC4:        as_double() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_double() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_double() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_double() = static_cast<double>(type::CLASS); break;
                        case type::FUNC:        as_double() = static_cast<double>(type::FUNC); break;
                        case type::ARRAY:       as_double() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_double(); break;
                        case type::OBJECT:      as_double() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_double(); break;
                        case type::STRING:      as_double() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_double() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_double() = 0; break;
                        case type::VALBOX:      as_double() = static_cast<double>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::LONG_DOUBLE:
                    switch(thatt) {
                        case type::BOOL:        as_long_double() = thatref.as_bool(); break;
                        case type::CHAR:        as_long_double() = thatref.as_char(); break;
                        case type::S8:          as_long_double() = thatref.as_s8(); break;
                        case type::U8:          as_long_double() = thatref.as_u8(); break;
                        case type::S16:         as_long_double() = thatref.as_s16(); break;
                        case type::U16:         as_long_double() = thatref.as_u16(); break;
                        case type::WCHAR:       as_long_double() = thatref.as_wchar(); break;
                        case type::S32:         as_long_double() = thatref.as_s32(); break;
                        case type::U32:         as_long_double() = thatref.as_u32(); break;
                        case type::S64:         as_long_double() = thatref.as_s64(); break;
                        case type::U64:         as_long_double() = thatref.as_u64(); break;
                        case type::FLOAT:       as_long_double() = thatref.as_float(); break;
                        case type::DOUBLE:      as_long_double() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_long_double() = thatref.as_long_double(); break;
                        case type::VEC4:        as_long_double() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_long_double() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_long_double() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_long_double() = static_cast<long double>(type::CLASS); break;
                        case type::FUNC:        as_long_double() = static_cast<long double>(type::FUNC); break;
                        case type::ARRAY:       as_long_double() = thatref.as_array().empty() ? 0 : thatref.as_array()[0].cast_to_long_double(); break;
                        case type::OBJECT:      as_long_double() = thatref.as_object().empty() ? 0 : thatref.as_object().begin()->second.cast_to_long_double(); break;
                        case type::STRING:      as_long_double() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_long_double() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_long_double() = 0; break;
                        case type::VALBOX:      as_long_double() = static_cast<long double>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::VEC4:
                    switch(thatt) {
                        case type::BOOL:        as_vec4() = thatref.as_bool(); break;
                        case type::CHAR:        as_vec4() = thatref.as_char(); break;
                        case type::S8:          as_vec4() = thatref.as_s8(); break;
                        case type::U8:          as_vec4() = thatref.as_u8(); break;
                        case type::S16:         as_vec4() = thatref.as_s16(); break;
                        case type::U16:         as_vec4() = thatref.as_u16(); break;
                        case type::WCHAR:       as_vec4() = thatref.as_wchar(); break;
                        case type::S32:         as_vec4() = thatref.as_s32(); break;
                        case type::U32:         as_vec4() = thatref.as_u32(); break;
                        case type::S64:         as_vec4() = thatref.as_s64(); break;
                        case type::U64:         as_vec4() = thatref.as_u64(); break;
                        case type::FLOAT:       as_vec4() = thatref.as_float(); break;
                        case type::DOUBLE:      as_vec4() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_vec4() = thatref.as_long_double(); break;
                        case type::VEC4:        as_vec4() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_vec4() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_vec4() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_vec4() = static_cast<long double>(type::CLASS); break;
                        case type::FUNC:        as_vec4() = static_cast<long double>(type::FUNC); break;
                        case type::ARRAY:
                            if(thatref.as_array().size() == 4) {
                                thisref.as_vec4().x() = thatref.as_array().at(0).cast_to_long_double();
                                thisref.as_vec4().y() = thatref.as_array().at(1).cast_to_long_double();
                                thisref.as_vec4().z() = thatref.as_array().at(2).cast_to_long_double();
                                thisref.as_vec4().w() = thatref.as_array().at(3).cast_to_long_double();
                            }
                            break;
                        case type::OBJECT:
                            thisref.as_vec4().x() = thatref.as_object().at("x").cast_to_long_double();
                            thisref.as_vec4().y() = thatref.as_object().at("y").cast_to_long_double();
                            thisref.as_vec4().z() = thatref.as_object().at("z").cast_to_long_double();
                            thisref.as_vec4().w() = thatref.as_object().at("w").cast_to_long_double();
                            break;
                        case type::STRING:      as_vec4() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_vec4() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_vec4() = vec4_t{}; break;
                        case type::VALBOX:      as_vec4() = static_cast<long double>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::MAT4:
                    switch(thatt) {
                        case type::BOOL:        as_mat4() = thatref.as_bool(); break;
                        case type::CHAR:        as_mat4() = thatref.as_char(); break;
                        case type::S8:          as_mat4() = thatref.as_s8(); break;
                        case type::U8:          as_mat4() = thatref.as_u8(); break;
                        case type::S16:         as_mat4() = thatref.as_s16(); break;
                        case type::U16:         as_mat4() = thatref.as_u16(); break;
                        case type::WCHAR:       as_mat4() = thatref.as_wchar(); break;
                        case type::S32:         as_mat4() = thatref.as_s32(); break;
                        case type::U32:         as_mat4() = thatref.as_u32(); break;
                        case type::S64:         as_mat4() = thatref.as_s64(); break;
                        case type::U64:         as_mat4() = thatref.as_u64(); break;
                        case type::FLOAT:       as_mat4() = thatref.as_float(); break;
                        case type::DOUBLE:      as_mat4() = thatref.as_double(); break;
                        case type::LONG_DOUBLE: as_mat4() = thatref.as_long_double(); break;
                        case type::VEC4:        as_mat4() = thatref.as_vec4().x(); break;
                        case type::MAT4:        as_mat4() = thatref.as_mat4().at(0, 0); break;
                        case type::POINTER:     as_mat4() = reinterpret_cast<uintptr_t>(thatref.as_ptr()); break;
                        case type::CLASS:       as_mat4() = static_cast<long double>(type::CLASS); break;
                        case type::FUNC:        as_mat4() = static_cast<long double>(type::FUNC); break;
                        case type::ARRAY:
                            if(thatref.as_array().size() == 16) {
                                for(int i{}; i < 16; ++i) {
                                    thisref.as_mat4().at_flat_index(i) = thatref.as_array().at(i).cast_to_long_double();
                                }
                            }
                            break;
                        case type::OBJECT: {
                                for(int i{}; i < 16; ++i) {
                                    thisref.as_mat4().at_flat_index(i) = thatref.as_object().at("mat4").as_array().at(i).cast_to_long_double();
                                }
                            }
                            break;
                        case type::STRING:      as_mat4() = thatref.as_string().empty() ? 0 : thatref.as_string().at(0); break;
                        case type::WSTRING:     as_mat4() = thatref.as_wstring().empty() ? 0 : thatref.as_wstring().at(0); break;
                        case type::UNDEFINED:   as_mat4() = mat4_t{}; break;
                        case type::VALBOX:      as_mat4() = static_cast<long double>(type::VALBOX); break;
                        default: break;
                    }
                    break;
                case type::POINTER: throw std::runtime_error{"inappropriate value"};
                    break;
                case type::CLASS:
                    switch(thatt) {
                        case type::BOOL:        throw std::runtime_error{"inappropriate value"};
                        case type::CHAR:        throw std::runtime_error{"inappropriate value"};
                        case type::S8:          throw std::runtime_error{"inappropriate value"};
                        case type::U8:          throw std::runtime_error{"inappropriate value"};
                        case type::S16:         throw std::runtime_error{"inappropriate value"};
                        case type::U16:         throw std::runtime_error{"inappropriate value"};
                        case type::WCHAR:       throw std::runtime_error{"inappropriate value"};
                        case type::S32:         throw std::runtime_error{"inappropriate value"};
                        case type::U32:         throw std::runtime_error{"inappropriate value"};
                        case type::S64:         throw std::runtime_error{"inappropriate value"};
                        case type::U64:         throw std::runtime_error{"inappropriate value"};
                        case type::FLOAT:       throw std::runtime_error{"inappropriate value"};
                        case type::DOUBLE:      throw std::runtime_error{"inappropriate value"};
                        case type::LONG_DOUBLE: throw std::runtime_error{"inappropriate value"};
                        case type::VEC4:        throw std::runtime_error{"inappropriate value"};
                        case type::MAT4:        throw std::runtime_error{"inappropriate value"};
                        case type::POINTER:     throw std::runtime_error{"inappropriate value"};
                        case type::CLASS:
                            if(thisref.box_->class_ == thatref.box_->class_) {
                                thisref.box_->value_ = thatref.box_->value_;
                            }
                            break;
                        case type::FUNC:        throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY:       throw std::runtime_error{"inappropriate value"};
                        case type::OBJECT:      throw std::runtime_error{"inappropriate value"};
                        case type::STRING:      throw std::runtime_error{"inappropriate value"};
                        case type::WSTRING:     throw std::runtime_error{"inappropriate value"};
                        case type::UNDEFINED:   throw std::runtime_error{"inappropriate value"};
                        case type::VALBOX:      throw std::runtime_error{"inappropriate value"};
                        default: break;
                    }
                    break;
                case type::FUNC: throw std::runtime_error{"inappropriate value"};
                case type::ARRAY:
                    switch(thatt) {
                        case type::BOOL:
                        case type::CHAR:
                        case type::S8:
                        case type::U8:
                        case type::S16:
                        case type::U16:
                        case type::WCHAR:
                        case type::S32:
                        case type::U32:
                        case type::S64:
                        case type::U64:
                        case type::FLOAT:
                        case type::DOUBLE:
                        case type::LONG_DOUBLE:
                        case type::POINTER:
                        case type::CLASS:
                        case type::FUNC:
                        case type::VALBOX:
                        case type::OBJECT:
                            thisref.as_array().clear();
                            thisref.as_array().push_back(thatref.clone());
                            break;
                        case type::VEC4: {
                                array_t &thisarr{thisref.as_array()};
                                thisarr.clear();
                                thisarr.push_back(thatref.as_vec4().x());
                                thisarr.push_back(thatref.as_vec4().y());
                                thisarr.push_back(thatref.as_vec4().z());
                                thisarr.push_back(thatref.as_vec4().w());
                            }
                            break;
                        case type::MAT4:  {
                                array_t &thisarr{thisref.as_array()};
                                thisarr.clear();
                                for(int i{}; i < 16; ++i) {
                                    thisarr.push_back(thatref.as_mat4().at_flat_index(i));
                                }
                            }
                            break;
                        case type::ARRAY:
                            thisref.box_->value_ = thatref.box_->value_;
                            thisref.pointed_box_ = thatref.pointed_box_;
                            break;
                        case type::STRING: {
                                array_t &thisarr{thisref.as_array()};
                                thisarr.clear();
                                for(auto &&c: thatref.as_string()) {
                                    thisarr.push_back(c);
                                }
                            }
                            break;
                        case type::WSTRING: {
                                array_t &thisarr{thisref.as_array()};
                                thisarr.clear();
                                for(auto &&c: thatref.as_wstring()) {
                                    thisarr.push_back(c);
                                }
                            }
                            break;
                        case type::UNDEFINED: thisref.as_array().clear(); break;
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::OBJECT:
                    switch(thatt) {
                        case type::BOOL: throw std::runtime_error{"inappropriate value"};
                        case type::CHAR: throw std::runtime_error{"inappropriate value"};
                        case type::S8: throw std::runtime_error{"inappropriate value"};
                        case type::U8: throw std::runtime_error{"inappropriate value"};
                        case type::S16: throw std::runtime_error{"inappropriate value"};
                        case type::U16: throw std::runtime_error{"inappropriate value"};
                        case type::WCHAR: throw std::runtime_error{"inappropriate value"};
                        case type::S32: throw std::runtime_error{"inappropriate value"};
                        case type::U32: throw std::runtime_error{"inappropriate value"};
                        case type::S64: throw std::runtime_error{"inappropriate value"};
                        case type::U64: throw std::runtime_error{"inappropriate value"};
                        case type::FLOAT: throw std::runtime_error{"inappropriate value"};
                        case type::DOUBLE: throw std::runtime_error{"inappropriate value"};
                        case type::LONG_DOUBLE: throw std::runtime_error{"inappropriate value"};
                        case type::VEC4:
                            thisref.as_object().clear();
                            thisref.as_object()["x"] = thatref.as_vec4().x();
                            thisref.as_object()["y"] = thatref.as_vec4().y();
                            thisref.as_object()["z"] = thatref.as_vec4().z();
                            thisref.as_object()["w"] = thatref.as_vec4().w();
                            break;
                        case type::MAT4:
                            thisref.as_object().clear();
                            for(int i{}; i < 16; ++i) {
                                thisref["mat4"][i].assign(thatref.as_mat4().at_flat_index(i));
                            }
                            break;
                        case type::POINTER: throw std::runtime_error{"inappropriate value"};
                        case type::CLASS: throw std::runtime_error{"inappropriate value"};
                        case type::FUNC: throw std::runtime_error{"inappropriate value"};
                        case type::STRING:
                            thisref.from_json(json::deserialize(thatref.as_string()));
                            break;
                        case type::WSTRING:
                            thisref.from_json(json::deserialize(thatref.as_wstring()));
                            break;
                        case type::UNDEFINED: thisref.as_object().clear(); break;
                        case type::VALBOX: throw std::runtime_error{"inappropriate value"};
                        case type::ARRAY: throw std::runtime_error{"inappropriate value"};
                        case type::OBJECT: {
                            for(auto &&p: thatref.as_object()) {
                                thisref.as_object()[p.first] = p.second.clone();
                            }
                            break;
                        }
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::STRING:
                    switch(thatt) {
                        case type::BOOL: thisref.as_string() = thatref.as_bool() ? "true" : "false"; break;
                        case type::CHAR: thisref.as_string().resize(1); thisref.as_string()[0] = thatref.as_char(); break;
                        case type::S8: thisref.as_string() = str_util::itoa<std::string>(thatref.as_s8()); break;
                        case type::U8: thisref.as_string() = str_util::utoa<std::string>(thatref.as_u8()); break;
                        case type::S16: thisref.as_string() = str_util::itoa<std::string>(thatref.as_s16()); break;
                        case type::U16: thisref.as_string() = str_util::utoa<std::string>(thatref.as_u16()); break;
                        case type::WCHAR: thisref.as_string() = str_util::ucs_to_utf8(thatref.cast_to_u64()); break; // throw std::runtime_error{"inappropriate value"};
                        case type::S32: thisref.as_string() = str_util::itoa<std::string>(thatref.as_s32()); break;
                        case type::U32: thisref.as_string() = str_util::utoa<std::string>(thatref.as_u32()); break;
                        case type::S64: thisref.as_string() = str_util::itoa<std::string>(thatref.as_s64()); break;
                        case type::U64: thisref.as_string() = str_util::utoa<std::string>(thatref.as_u64()); break;
                        case type::FLOAT: thisref.as_string() = str_util::ftoa(thatref.as_float()); break;
                        case type::DOUBLE: thisref.as_string() = str_util::ftoa(thatref.as_double()); break;
                        case type::LONG_DOUBLE: thisref.as_string() = str_util::ftoa(thatref.as_long_double()); break;
                        case type::VEC4: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::MAT4: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::POINTER: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::CLASS: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::FUNC: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::STRING: thisref.as_string() = thatref.as_string(); break;
                        case type::WSTRING: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::UNDEFINED: thisref.as_string().clear(); break;
                        case type::VALBOX: thisref.as_string() = thatref.cast_to_string(); break;
                        case type::ARRAY: {
                                std::string &thisstr{thisref.as_string()};
                                thisstr.clear();
                                for(auto &&itm: thatref.as_array()) {
                                    thisstr.push_back(itm.cast_to_char());
                                }
                            }
                            break;
                        case type::OBJECT: thisref.as_string() = thatref.cast_to_string(); break;
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::WSTRING:
                    switch(thatt) {
                        case type::BOOL: thisref.as_wstring() = thatref.as_bool() ? L"true" : L"false"; break;
                        case type::CHAR: thisref.as_wstring().resize(1); thisref.as_string()[0] = thatref.as_char(); break;
                        case type::S8: thisref.as_wstring() = str_util::itoa<std::wstring>(thatref.as_s8()); break;
                        case type::U8: thisref.as_wstring() = str_util::utoa<std::wstring>(thatref.as_u8()); break;
                        case type::S16: thisref.as_wstring() = str_util::itoa<std::wstring>(thatref.as_s16()); break;
                        case type::U16: thisref.as_wstring() = str_util::utoa<std::wstring>(thatref.as_u16()); break;
                        case type::WCHAR: thisref.as_wstring().clear(); thisref.as_wstring() += thatref.as_wchar(); break;
                        case type::S32: thisref.as_wstring() = str_util::itoa<std::wstring>(thatref.as_s32()); break;
                        case type::U32: thisref.as_wstring() = str_util::utoa<std::wstring>(thatref.as_u32()); break;
                        case type::S64: thisref.as_wstring() = str_util::itoa<std::wstring>(thatref.as_s64()); break;
                        case type::U64: thisref.as_wstring() = str_util::utoa<std::wstring>(thatref.as_u64()); break;
                        case type::FLOAT: thisref.as_wstring() = str_util::from_utf8(str_util::ftoa(thatref.as_float())); break;
                        case type::DOUBLE: thisref.as_wstring() = str_util::from_utf8(str_util::ftoa(thatref.as_double())); break;
                        case type::LONG_DOUBLE: thisref.as_wstring() = str_util::from_utf8(str_util::ftoa(thatref.as_long_double())); break;
                        case type::VEC4: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::MAT4: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::POINTER: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::CLASS: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::FUNC: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::STRING: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::WSTRING: thisref.as_wstring() = thatref.as_wstring(); break;
                        case type::UNDEFINED: thisref.as_wstring().clear(); break;
                        case type::VALBOX: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        case type::ARRAY: {
                                std::wstring &thisstr{thisref.as_wstring()};
                                thisstr.clear();
                                for(auto &&itm: thatref.as_array()) {
                                    thisstr.push_back(itm.cast_to_wchar());
                                }
                            }
                            break;
                        case type::OBJECT: thisref.as_wstring() = thatref.cast_to_wstring(); break;
                        default: throw std::runtime_error{"inappropriate value"};
                    }
                    break;
                case type::UNDEFINED:
                    if(thatt != type::UNDEFINED) {
                        *this = thatref.clone();
                    }
                    break;
                case type::VALBOX:
                    throw std::runtime_error{"inappropriate value"};
                default:
                    throw std::runtime_error{"inappropriate value"};
            }
            return *this;
        }

        valbox &assign(valbox const &that) {
            valbox const &that_ref{that.deref()};
            valbox &ref{deref()};
            if(ref.box_.get() == that_ref.box_.get()) {
                return *this;
            }
            if(that_ref.is_undefined()) {
                if(ref.box_) {
                    ref.box_->value_ = value_t{};
                    ref.box_->type_ = type::UNDEFINED;
                    ref.box_->pointed_type_ = type::UNDEFINED;
                    ref.box_->class_.clear();
                    ref.box_->func_name_.clear();
                    ref.box_->user_func_ = false;
                }
                ref.pointed_box_.reset();
            } else {
                if(!ref.box_) {
                    ref.box_ = std::make_shared<box_data>(
                        that_ref.box_->value_,
                        that_ref.box_->type_,
                        that_ref.box_->pointed_type_,
                        that_ref.box_->class_,
                        that_ref.box_->func_name_,
                        that_ref.box_->user_func_
                    );
                } else {
                    ref.box_->value_ = that_ref.box_->value_;
                    ref.box_->type_ = that_ref.box_->type_;
                    ref.box_->pointed_type_ = that_ref.box_->pointed_type_;
                    ref.box_->class_ = that_ref.box_->class_;
                    ref.box_->func_name_ = that_ref.box_->func_name_;
                    ref.box_->user_func_ = that_ref.box_->user_func_;
                }
                ref.pointed_box_ = that_ref.pointed_box_;
            }
            return *this;
        }

        valbox &fetch_from(valbox &&that) {
            valbox &that_ref{that.deref()};
            valbox &ref{deref()};
            if(ref.box_.get() == that_ref.box_.get()) {
                return *this;
            }
            ref.box_ = std::move(that_ref.box_);
            ref.pointed_box_ = std::move(that_ref.pointed_box_);
            return *this;
        }

        char cast_to_char() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) { return as_string().empty() ? 0 : as_string()[0]; }
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) { return as_wstring().empty() ? 0 : as_wstring()[0]; }
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::uint8_t cast_to_u8() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::int8_t cast_to_s8() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoi(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoi(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::uint16_t cast_to_u16() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::int16_t cast_to_s16() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoi(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoi(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::uint32_t cast_to_u32() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::int32_t cast_to_s32() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoi(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoi(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        int cast_to_int() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoi(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoi(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::uint64_t cast_to_u64() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::int64_t cast_to_s64() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoi(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoi(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::size_t cast_to_size_t() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atoui(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atoui(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        float cast_to_float() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atof(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atof(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        double cast_to_double() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atof(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atof(cast_to_string()); } catch (...) {} break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        long double cast_to_long_double() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        try { return str_util::atof(as_string()); } catch (...) {} break;
                case type::WSTRING:       try { return str_util::atof(cast_to_string()); } catch (...) { } break;
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        bool cast_to_bool() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool();
                case type::WCHAR:         return as_wchar();
                case type::STRING:
                    if(str_util::fltr<std::string>{}.strtolower(as_string()) == "false") {
                        return false;
                    } else {
                        try {
                            return str_util::atof(as_string());
                        } catch (...) {
                        }
                        return true;
                    }
                case type::WSTRING:
                    if(str_util::fltr<std::string>{}.strtolower(cast_to_string()) == "false") {
                        return false;
                    } else {
                        try {
                            return str_util::atof(as_string());
                        } catch (...) {
                        }
                        return true;
                    }
                case type::OBJECT:        return !as_object().empty();
                case type::ARRAY:         return !as_array().empty();
                case type::POINTER:       return deref().as_ptr() != nullptr;
                default: break;
            }
            return false;
        }

        wchar_t cast_to_wchar() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          return as_char();
                case type::U8:            return as_u8();
                case type::S8:            return as_s8();
                case type::U16:           return as_u16();
                case type::S16:           return as_s16();
                case type::U32:           return as_u32();
                case type::S32:           return as_s32();
                case type::U64:           return as_u64();
                case type::S64:           return as_s64();
                case type::FLOAT:         return as_float();
                case type::DOUBLE:        return as_double();
                case type::LONG_DOUBLE:   return as_long_double();
                case type::BOOL:          return as_bool() ? 1 : 0;
                case type::WCHAR:         return as_wchar();
                case type::STRING:        return as_string().empty() ? 0 : as_string()[0];
                case type::WSTRING:       return as_wstring().empty() ? 0 : as_wstring()[0];
                case type::POINTER:       return (uintptr_t)deref().as_ptr();
                default: break;
            }
            return 0;
        }

        std::string cast_to_string() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          { std::stringstream ss{}; ss << as_char(); return ss.str(); }
                case type::U8:            { std::stringstream ss{}; ss << as_u8(); return ss.str(); }
                case type::S8:            { std::stringstream ss{}; ss << as_s8(); return ss.str(); }
                case type::U16:           { std::stringstream ss{}; ss << as_u16(); return ss.str(); }
                case type::S16:           { std::stringstream ss{}; ss << as_s16(); return ss.str(); }
                case type::U32:           { std::stringstream ss{}; ss << as_u32(); return ss.str(); }
                case type::S32:           { std::stringstream ss{}; ss << as_s32(); return ss.str(); }
                case type::U64:           { std::stringstream ss{}; ss << as_u64(); return ss.str(); }
                case type::S64:           { std::stringstream ss{}; ss << as_s64(); return ss.str(); }
                case type::FLOAT:         { std::stringstream ss{}; ss << as_float(); return ss.str(); }
                case type::DOUBLE:        { std::stringstream ss{}; ss << as_double(); return ss.str(); }
                case type::LONG_DOUBLE:   { std::stringstream ss{}; ss << as_long_double(); return ss.str(); }
                case type::BOOL:          { return as_bool() ? "true" : "false"; }
                case type::WCHAR:         { std::wstring s{}; s += as_wchar(); return teal::str_util::to_utf8(s); }
                case type::STRING:        return as_string();
                case type::WSTRING:       return teal::str_util::to_utf8(as_wstring());
                case type::OBJECT:        return to_json().serialize5(4);
                case type::ARRAY: {
                    std::string res{};
                    for(auto &&v: as_array()) {
                        res += v.cast_to_char();
                    }
                    return res;
                }
                default: break;
            }
            return {};
        }

        std::wstring cast_to_wstring() const {
            switch(val_or_pointed_type()) {
                case type::CHAR:          { std::wstringstream ss{}; ss << as_char(); return ss.str(); }
                case type::U8:            { std::wstringstream ss{}; ss << as_u8(); return ss.str(); }
                case type::S8:            { std::wstringstream ss{}; ss << as_s8(); return ss.str(); }
                case type::U16:           { std::wstringstream ss{}; ss << as_u16(); return ss.str(); }
                case type::S16:           { std::wstringstream ss{}; ss << as_s16(); return ss.str(); }
                case type::U32:           { std::wstringstream ss{}; ss << as_u32(); return ss.str(); }
                case type::S32:           { std::wstringstream ss{}; ss << as_s32(); return ss.str(); }
                case type::U64:           { std::wstringstream ss{}; ss << as_u64(); return ss.str(); }
                case type::S64:           { std::wstringstream ss{}; ss << as_s64(); return ss.str(); }
                case type::FLOAT:         { std::wstringstream ss{}; ss << as_float(); return ss.str(); }
                case type::DOUBLE:        { std::wstringstream ss{}; ss << as_double(); return ss.str(); }
                case type::LONG_DOUBLE:   { std::wstringstream ss{}; ss << as_long_double(); return ss.str(); }
                case type::BOOL:          { return as_bool() ? L"true" : L"false"; }
                case type::WCHAR:         { std::wstring s{}; s += as_wchar(); return s; }
                case type::STRING:        return teal::str_util::from_utf8(as_string());
                case type::WSTRING:       return as_wstring();
                case type::OBJECT:        return teal::str_util::from_utf8(to_json().serialize5(4));
                case type::ARRAY: {
                    std::wstring res{};
                    for(auto &&v: as_array()) {
                        res += v.cast_to_wchar();
                    }
                    return res;
                }
                default: break;
            }
            return {};
        }

        array_t cast_to_array() const {
            array_t res{};
            switch(val_or_pointed_type()) {
                case type::STRING:      for(auto &&c: as_string()) { res.push_back(c); } break;
                case type::WSTRING:     for(auto &&c: as_wstring()) { res.push_back(c); } break;
                case type::ARRAY:       res = as_array(); break;
                default:                res.push_back(*this); break;
            }
            return res;
        }

        std::vector<std::uint8_t> cast_to_byte_array() const {
            std::vector<std::uint8_t> res{};
            switch(val_or_pointed_type()) {
                case type::BOOL:         res.push_back(cast_to_u8());                 break;
                case type::CHAR:         res.push_back(cast_to_u8());                 break;
                case type::S8:           res.push_back(cast_to_u8());                 break;
                case type::U8:           res.push_back(as_u8());                      break;
                case type::S16: {
                        auto v{as_s16()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::U16: {
                        auto v{as_u16()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::WCHAR: {
                        auto v{as_wchar()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::S32: {
                        auto v{as_s32()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::U32: {
                        auto v{as_u32()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::S64: {
                        auto v{as_s64()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::U64: {
                        auto v{as_u64()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::FLOAT: {
                        auto v{as_float()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::DOUBLE: {
                        auto v{as_double()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::LONG_DOUBLE: {
                        auto v{as_long_double()};
                        std::uint8_t *vptr{(std::uint8_t *)&v};
                        for(std::size_t i = 0; i < sizeof(v); ++i) {
                            res.push_back(vptr[i]);
                        }
                    }
                    break;
                case type::STRING:
                    return std::vector<std::uint8_t>{as_string().begin(), as_string().end()};
                case type::WSTRING: {
                        res.assign((uint8_t *)as_wstring().data(), (uint8_t *)as_wstring().data() + as_wstring().size() * sizeof(std::wstring::value_type));
                    }
                    break;
                case type::ARRAY:
                    res.reserve(as_array().size());
                    for(auto &&v: as_array()) {
                        auto itm_bytes{v.cast_to_byte_array()};
                        res.insert(res.end(), itm_bytes.begin(), itm_bytes.end());
                    }
                    break;
                case type::OBJECT: {
                        auto js{to_json().serialize5()};
                        res = std::vector<std::uint8_t>{js.begin(), js.end()};
                    }
                    break;
                default:
                    break;
            }
            return res;
        }

        object_t cast_to_object() const {
            object_t res{};
            switch(val_or_pointed_type()) {
                case type::OBJECT:
                    res = as_object();
                    break;
                case type::STRING: {
                    try {
                            res = valbox{json::deserialize(as_string())}.as_object();
                        } catch(...) {
                        }
                    }
                    break;
                case type::WSTRING: {
                        try {
                            res = valbox{json::deserialize(cast_to_string())}.as_object();
                        } catch(...) {
                        }
                    }
                    break;
                default:
                    break;
            }
            return res;
        }

        json to_json() const {
            if(is_undefined_ref()) {
                return json{};
            } else if(is_string_ref()) {
                return json{as_string()};
            } else if(is_bool_ref()) {
                return json{as_bool()};
            } else if(is_char_ref()) {
                std::string s{}; s += as_char();
                return json{s};
            } else if(is_wchar_ref()) {
                std::wstring s{}; s += as_wchar();
                return json{s};
            } else if(is_wstring_ref()) {
                return json{as_wstring()};
            } else if(is_any_int_number()) {
                return json{cast_num_to_num<std::int64_t>()};
            } else if(is_any_fp_number()) {
                return json{cast_num_to_num<long double>()};
            } else if(is_array_ref()) {
                json res{};
                res.become_array();
                for(auto &&v: as_array()) {
                    res.push_back(v.to_json());
                }
                return res;
            } else if(is_object_ref()) {
                json res{};
                res.become_object();
                object_t const &o{as_object()};
                for(auto &&p: o) {
                    res[p.first] = p.second.to_json();
                }
                return res;
            } else if(as_valbox_ptr() != nullptr) {
                return deref().to_json();
            }
            return {};
        }

        valbox &from_json(json const &v) {
            valbox &vr{deref()};
            vr.pointed_box_.reset();
            if(v.is_float()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(v.as_longdouble(), type::LONG_DOUBLE);
                } else {
                    vr.box_->value_ = v.as_longdouble();
                    vr.box_->type_ = type::LONG_DOUBLE;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
            } else if(v.is_number()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(v.as_number(), type::S64);
                } else {
                    vr.box_->value_ = v.as_number();
                    vr.box_->type_ = type::S64;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
            } else if(v.is_string()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(v.as_string(), type::STRING);
                } else {
                    vr.box_->value_ = v.as_string();
                    vr.box_->type_ = type::STRING;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
            } else if(v.is_bool()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(v.as_boolean(), type::BOOL);
                } else {
                    vr.box_->value_ = v.as_boolean();
                    vr.box_->type_ = type::BOOL;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
            } else if(v.is_array()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(array_t{}, type::ARRAY);
                } else {
                    vr.box_->value_ = array_t{};
                    vr.box_->type_ = type::ARRAY;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
                for(std::size_t i{0}; i < v.size(); ++i) {
                    valbox item{};
                    item.from_json(v[i]);
                    vr.as_array().push_back(std::move(item));
                }
            } else if(v.is_object()) {
                if(!vr.box_) {
                    vr.box_ = std::make_shared<box_data>(object_t{}, type::OBJECT);
                } else {
                    vr.box_->value_ = object_t{};
                    vr.box_->type_ = type::OBJECT;
                    vr.box_->pointed_type_ = type::UNDEFINED;
                    vr.box_->class_.clear();
                    vr.box_->func_name_.clear();
                    vr.box_->user_func_ = false;
                }
                vr.pointed_box_.reset();
                v.traverse_object([&](std::string const &key, json const &val) {
                    vr.as_object()[key].from_json(val);
                });
            } else if(v.is_null()) {
                vr.become_undefined();
            }
            return *this;
        }

        void set_pointed(valbox &vbp) {
            pointed_box_ = vbp.box_;
        }

        std::size_t num_refs() const {
            return box_.use_count();
        }

        std::string dump() const {
            std::stringstream os{};
            switch(val_or_pointed_type()) {
                case type::CHAR: os << as_char(); break;
                case type::U8: os << as_u8(); break;
                case type::S8: os << as_s8(); break;
                case type::U16: os << as_u16(); break;
                case type::S16: os << as_s16(); break;
                case type::U32: os << as_u32(); break;
                case type::S32: os << as_s32(); break;
                case type::U64: os << as_u64(); break;
                case type::S64: os << as_s64(); break;
                case type::FLOAT: os << as_float(); break;
                case type::DOUBLE: os << as_double(); break;
                case type::LONG_DOUBLE: os << as_long_double(); break;
                case type::BOOL: os << (as_bool() ? "true" : "false"); break;
                case type::WCHAR: { std::wstring ws{}; ws += as_wchar(); os << teal::str_util::to_utf8(ws); } break;
                case type::STRING: os << as_string(); break;
                case type::WSTRING: os << teal::str_util::to_utf8(as_wstring()); break;
                case type::CLASS: os << "<" << deref().class_name() << ">"; break;
                case type::POINTER: os << deref().dump(); break;
                case type::UNDEFINED: os << "<undefined>"; break;
                case type::FUNC: os << "<function>"; break;
                case type::ARRAY: {
                        auto const &a{as_array()};
                        os << "[";
                        std::string sep{};
                        for(auto &&v: a) {
                            os << sep << v.dump(); sep = ",";
                        }
                        os << "]";
                    }
                    break;
                case type::VEC4: {
                        std::stringstream ss{};
                        ss << "vec4{";
                        std::string sep{};
                        for(std::size_t i{0}; i < 4; ++i) {
                            ss << sep << std::fixed << as_vec4()[i];
                            if(sep.empty()) {
                                sep = ",";
                            }
                        }
                        ss << "}";
                        os << ss.str();
                    }
                    break;
                case type::MAT4: {
                        std::stringstream ss{};
                        ss << "mat4{";
                        std::string sep1{};
                        for(std::size_t r{0}; r < 4; ++r) {
                            ss << sep1 << "{";
                            std::string sep2{};
                            for(std::size_t c{0}; c < 4; ++c) {
                                ss << sep2 << std::fixed << as_mat4().at(r, c);
                                sep2 = ", ";
                            }
                            ss << "}";
                            if(sep1.empty()) {
                                sep1 = ",";
                            }
                        }
                        ss << "}";
                        os << ss.str();
                    }
                    break;
                case type::OBJECT: os << to_json().serialize5(); break;
                default: os << "<corrupted>"; break;
            }
            return os.str();
        }

        std::wstring wdump() const {
            std::wstringstream os{};
            switch(val_or_pointed_type()) {
                case type::CHAR: os << cast_to_wchar(); break;
                case type::U8: os << as_u8(); break;
                case type::S8: os << as_s8(); break;
                case type::U16: os << as_u16(); break;
                case type::S16: os << as_s16(); break;
                case type::U32: os << as_u32(); break;
                case type::S32: os << as_s32(); break;
                case type::U64: os << as_u64(); break;
                case type::S64: os << as_s64(); break;
                case type::FLOAT: os << as_float(); break;
                case type::DOUBLE: os << as_double(); break;
                case type::LONG_DOUBLE: os << as_long_double(); break;
                case type::BOOL: os << (as_bool() ? L"true" : L"false"); break;
                case type::WCHAR: os << as_wchar(); break;
                case type::STRING: os << cast_to_wstring(); break;
                case type::WSTRING: os << as_wstring(); break;
                case type::CLASS: os << L"<" << str_util::from_utf8(deref().class_name()) << L">"; break;
                case type::POINTER: os << deref().wdump(); break;
                case type::UNDEFINED: os << L"<undefined>"; break;
                case type::FUNC: os << L"<function>"; break;
                case type::ARRAY: {
                    auto const &a{as_array()};
                    os << L"[";
                    std::wstring sep{};
                    for(auto &&v: a) {
                        os << sep << v.wdump(); sep = L",";
                    }
                    os << L"]";
                }
                break;
                case valbox::type::VEC4: {
                    std::wstringstream ss{};
                    ss << L"vec4{";
                    std::wstring sep{};
                    for(std::size_t i{0}; i < 4; ++i) {
                        ss << sep << std::fixed << as_vec4()[i];
                        if(sep.empty()) {
                            sep = L",";
                        }
                    }
                    ss << L"}";
                    os << ss.str();
                }
                break;
                case valbox::type::MAT4: {
                    std::wstringstream ss{};
                    ss << L"mat4{";
                    std::wstring sep1{};
                    for(std::size_t r{0}; r < 4; ++r) {
                        ss << sep1 << L"{";
                        std::wstring sep2{};
                        for(std::size_t c{0}; c < 4; ++c) {
                            ss << sep2 << std::fixed << as_mat4().at(r, c);
                            sep2 = L", ";
                        }
                        ss << L"}";
                        if(sep1.empty()) {
                            sep1 = L",";
                        }
                    }
                    ss << L"}";
                    os << ss.str();
                }
                break;
                case valbox::type::OBJECT: os << str_util::from_utf8(to_json().serialize5()); break;
                default: os << L"<corrupted>"; break;
            }
            return os.str();
        }

        template<typename T>
        json serialize(T *helper) const {
            json res{};
            res["type"] = type_to_str(val_or_pointed_type());
            json &v{res["value"]};
            if(is_undefined_ref()) {
                v = json{};
            } else if(is_string_ref()) {
                v = as_string();
            } else if(is_bool_ref()) {
                v = as_bool();
            } else if(is_char_ref()) {
                std::string s{}; s += as_char(); v = s;
            } else if(is_wchar_ref()) {
                std::wstring s{}; s += as_wchar(); v = s;
            } else if(is_wstring_ref()) {
                v = as_wstring();
            } else if(is_s8_ref()) {
                v = as_s8();
            } else if(is_u8_ref()) {
                v = as_u8();
            } else if(is_s16_ref()) {
                v = as_s16();
            } else if(is_u16_ref()) {
                v = as_u16();
            } else if(is_s32_ref()) {
                v = as_s32();
            } else if(is_u32_ref()) {
                v = as_u32();
            } else if(is_s64_ref()) {
                v = as_s64();
            } else if(is_u64_ref()) {
                v = as_u64();
            } else if(is_any_fp_number()) {
                v = cast_to_long_double();
            } else if(is_vec4_ref()) {
                v[0] = as_vec4()[0];
                v[1] = as_vec4()[1];
                v[2] = as_vec4()[2];
                v[3] = as_vec4()[3];
            } else if(is_mat4_ref()) {
                mat4_t const &m{as_mat4()};
                v[0][0] = m[0][0]; v[0][1] = m[0][1]; v[0][2] = m[0][2]; v[0][3] = m[0][3];
                v[1][0] = m[1][0]; v[1][1] = m[1][1]; v[1][2] = m[1][2]; v[1][3] = m[1][3];
                v[2][0] = m[2][0]; v[2][1] = m[2][1]; v[2][2] = m[2][2]; v[2][3] = m[2][3];
                v[3][0] = m[3][0]; v[3][1] = m[3][1]; v[3][2] = m[3][2]; v[3][3] = m[3][3];
            } else if(is_array_ref()) {
                v.become_array();
                for(auto &&val: as_array()) {
                    v.push_back(val.serialize(helper));
                }
            } else if(is_object_ref()) {
                v.become_object();
                object_t const &o{as_object()};
                for(auto &&p: o) {
                    v[p.first] = p.second.serialize(helper);
                }
            } else if(is_class_ref()) {
                auto s{helper->obj_svc_[class_name()].serializer(*this)};
                if(s) {
                    res["class"] = class_name();
                    v = *s;
                }
            } else if(is_func_ref()) {
                res["is_user_func"] = deref().is_user_func();
                v = deref().func_name();
            } else if(as_valbox_ptr() != nullptr) {
                v = deref().serialize(helper);
            }
            return res;
        }

        template<typename T>
        valbox &deserialize(json const &jv, T *helper) {
            valbox &vr{deref()};
            auto t{str_to_type(jv["type"].as_string())};
            switch(t) {
                case type::BOOL:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_boolean(), type::BOOL);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_boolean();
                        vr.box_->type_ = type::CHAR;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::CHAR:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<char>(v.as_unumber()), type::CHAR);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<char>(v.as_unumber());
                        vr.box_->type_ = type::CHAR;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::S8:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::int8_t>(v.as_number()), type::S8);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::int8_t>(v.as_number());
                        vr.box_->type_ = type::S8;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::U8:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::uint8_t>(v.as_unumber()), type::U8);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::uint8_t>(v.as_unumber());
                        vr.box_->type_ = type::U8;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::S16:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::int16_t>(v.as_number()), type::S16);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::int16_t>(v.as_number());
                        vr.box_->type_ = type::S16;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::U16:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::uint16_t>(v.as_unumber()), type::U16);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::uint16_t>(v.as_unumber());
                        vr.box_->type_ = type::U16;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::WCHAR:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<wchar_t>(v.as_unumber()), type::WCHAR);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<wchar_t>(v.as_unumber());
                        vr.box_->type_ = type::WCHAR;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::S32:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::int32_t>(v.as_number()), type::S32);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::int32_t>(v.as_number());
                        vr.box_->type_ = type::S32;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::U32:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::uint32_t>(v.as_unumber()), type::U32);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::uint32_t>(v.as_unumber());
                        vr.box_->type_ = type::U32;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::S64:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::int64_t>(v.as_number()), type::S64);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::int64_t>(v.as_number());
                        vr.box_->type_ = type::S64;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::U64:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(static_cast<std::uint64_t>(v.as_unumber()), type::U64);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = static_cast<std::uint64_t>(v.as_unumber());
                        vr.box_->type_ = type::U64;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::FLOAT:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_float(), type::FLOAT);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_double();
                        vr.box_->type_ = type::FLOAT;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::DOUBLE:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_double(), type::DOUBLE);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_double();
                        vr.box_->type_ = type::DOUBLE;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::LONG_DOUBLE:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_longdouble(), type::LONG_DOUBLE);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_longdouble();
                        vr.box_->type_ = type::LONG_DOUBLE;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::VEC4: {
                        json const &v{jv["value"]};
                        vec4_t v4{v[0].as_double(0), v[1].as_double(0), v[2].as_double(0), v[3].as_double(0)};
                        if(!vr.box_) {
                            vr.box_ = std::make_shared<box_data>(v4, type::VEC4);
                        } else {
                            vr.box_->value_ = v4;
                            vr.box_->type_ = type::VEC4;
                            vr.box_->pointed_type_ = type::UNDEFINED;
                            vr.box_->class_.clear();
                            vr.box_->func_name_.clear();
                            vr.box_->user_func_ = false;
                        }
                    }
                    break;
                case type::MAT4: {
                        json const &v{jv["value"]};
                        mat4_t m4{
                            v[0][0].as_double(0), v[0][1].as_double(0), v[0][2].as_double(0), v[0][3].as_double(0),
                            v[1][0].as_double(0), v[1][1].as_double(0), v[1][2].as_double(0), v[1][3].as_double(0),
                            v[2][0].as_double(0), v[2][1].as_double(0), v[2][2].as_double(0), v[2][3].as_double(0),
                            v[3][0].as_double(0), v[3][1].as_double(0), v[3][2].as_double(0), v[3][3].as_double(0)
                        };
                        if(!vr.box_) {
                            vr.box_ = std::make_shared<box_data>(m4, type::MAT4);
                        } else {
                            vr.box_->value_ = m4;
                            vr.box_->type_ = type::MAT4;
                            vr.box_->pointed_type_ = type::UNDEFINED;
                            vr.box_->class_.clear();
                            vr.box_->func_name_.clear();
                            vr.box_->user_func_ = false;
                        }
                    }
                    break;
                case type::POINTER: // TODO:
                    throw std::runtime_error{"unsupported or invalid conversion"};
                    break;
                case type::CLASS: {
                        valbox dv{
                            helper->obj_svc_[jv["class"].as_string()].deserializer(
                                jv["class"].as_string(), jv["value"].as_string()
                            )
                        };
                        if(!vr.box_) {
                            vr.box_ = std::move(dv.box_);
                        } else {
                            vr.box_->value_ = std::move(dv.box_->value_);
                            vr.box_->type_ = type::CLASS;
                            vr.box_->pointed_type_ = type::UNDEFINED;
                            vr.box_->class_ = dv.box_->class_;
                            vr.box_->func_name_.clear();
                            vr.box_->user_func_ = false;
                        }
                    }
                    break;
                case type::FUNC: {
                       json const &v{jv["value"]};
                        valbox fn{helper->find_func(v.as_string())};
                        if(fn.is_func_ref()) {
                            if(!vr.box_) {
                                vr.box_ = std::move(fn.box_);
                            } else {
                                vr.box_->value_ = fn.box_->value_;
                                vr.box_->type_ = type::FUNC;
                                vr.box_->pointed_type_ = type::UNDEFINED;
                                vr.box_->class_.clear();
                                vr.box_->func_name_ = fn.box_->func_name_;
                                if(jv["is_user_func"].as_boolean() != fn.box_->user_func_) {
                                    throw std::runtime_error{"not equivalent function by name"};
                                }
                                vr.box_->user_func_ = fn.box_->user_func_;
                            }
                        }
                    }
                    break;
                case type::ARRAY: {
                        vr.become_array();
                        vr.as_array().clear();
                        json const &v{jv["value"]};
                        auto sz{v.size()};
                        for(std::size_t i{}; i < sz; ++i) {
                            vr.as_array().emplace_back(valbox_no_initialize::dont_do_it);
                            vr.as_array().back().deserialize(v[i], helper);
                        }
                    }
                    break;
                case type::OBJECT: {
                        try {
                            vr.become_object();
                            vr.as_object().clear();
                            json const &v{jv["value"]};
                            v.traverse_object([&](std::string const &key, json const &val) {
                                vr.as_object()[key].deserialize(val, helper);
                            });
                        } catch (...) {
                        }
                    }
                    break;
                case type::STRING:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_string(), type::STRING);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_string();
                        vr.box_->type_ = type::STRING;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::WSTRING:
                    if(!vr.box_) {
                        json const &v{jv["value"]};
                        vr.box_ = std::make_shared<box_data>(v.as_wstring(), type::WSTRING);
                    } else {
                        json const &v{jv["value"]};
                        vr.box_->value_ = v.as_wstring();
                        vr.box_->type_ = type::WSTRING;
                        vr.box_->pointed_type_ = type::UNDEFINED;
                        vr.box_->class_.clear();
                        vr.box_->func_name_.clear();
                        vr.box_->user_func_ = false;
                    }
                    break;
                case type::UNDEFINED:
                    vr.become_undefined();
                    break;
                case type::VALBOX: // TODO:
                    throw std::runtime_error{"unsupported or invalid conversion"};
                    break;
                default: throw std::runtime_error{"unsupported or invalid conversion"};
            }
            return *this;
        }

    private:
        valbox(std::shared_ptr<box_data> &&b): box_{std::move(b)} {}

        std::shared_ptr<box_data> box_{};
        std::shared_ptr<box_data> pointed_box_{};
    };

    static std::ostream &operator<<(std::ostream &os, valbox const &v) {
        switch(v.val_or_pointed_type()) {
            case valbox::type::CHAR: os << v.as_char(); break;
            case valbox::type::U8: os << v.as_u8(); break;
            case valbox::type::S8: os << v.as_s8(); break;
            case valbox::type::U16: os << v.as_u16(); break;
            case valbox::type::S16: os << v.as_s16(); break;
            case valbox::type::U32: os << v.as_u32(); break;
            case valbox::type::S32: os << v.as_s32(); break;
            case valbox::type::U64: os << v.as_u64(); break;
            case valbox::type::S64: os << v.as_s64(); break;
            case valbox::type::FLOAT: os << v.as_float(); break;
            case valbox::type::DOUBLE: os << v.as_double(); break;
            case valbox::type::LONG_DOUBLE: os << v.as_long_double(); break;
            case valbox::type::BOOL: os << (v.as_bool() ? "true" : "false"); break;
            case valbox::type::WCHAR: { std::wstring ws{}; ws += v.as_wchar(); os << teal::str_util::to_utf8(ws); } break;
            case valbox::type::STRING: os << v.as_string(); break;
            case valbox::type::WSTRING: os << teal::str_util::to_utf8(v.as_wstring()); break;
            case valbox::type::CLASS: os << "<" << v.deref().class_name() << ">"; break;
            case valbox::type::POINTER: os << v.deref(); break;
            case valbox::type::UNDEFINED: os << "<undefined>"; break;
            case valbox::type::FUNC: os << "<function>"; break;
            case valbox::type::ARRAY: {
                    auto const &a{v.as_array()};
                    os << "[";
                    std::string sep{};
                    for(auto &&v: a) {
                        os << sep << v; sep = ",";
                    }
                    os << "]";
                }
                break;
            case valbox::type::VEC4: {
                    std::stringstream ss{};
                    ss << "vec4{";
                    std::string sep{};
                    for(std::size_t i{0}; i < 4; ++i) {
                        ss << sep << std::fixed << v.as_vec4()[i];
                        if(sep.empty()) {
                            sep = ",";
                        }
                    }
                    ss << "}";
                    os << ss.str();
                }
                break;
            case valbox::type::MAT4: {
                    std::stringstream ss{};
                    ss << "mat4{";
                    std::string sep1{};
                    for(std::size_t r{0}; r < 4; ++r) {
                        ss << sep1 << "{";
                        std::string sep2{};
                        for(std::size_t c{0}; c < 4; ++c) {
                            ss << sep2 << std::fixed << v.as_mat4().at(r, c);
                            sep2 = ", ";
                        }
                        ss << "}";
                        if(sep1.empty()) {
                            sep1 = ",";
                        }
                    }
                    ss << "}";
                    os << ss.str();
                }
                break;
            case valbox::type::OBJECT: os << v.to_json().serialize5(); break;
            default: os << "<corrupted>"; break;
        }
        return os;
    }

    static std::wostream &operator<<(std::wostream &os, valbox const &v) {
        switch(v.val_or_pointed_type()) {
            case valbox::type::CHAR: os << v.cast_to_wchar(); break;
            case valbox::type::U8: os << v.as_u8(); break;
            case valbox::type::S8: os << v.as_s8(); break;
            case valbox::type::U16: os << v.as_u16(); break;
            case valbox::type::S16: os << v.as_s16(); break;
            case valbox::type::U32: os << v.as_u32(); break;
            case valbox::type::S32: os << v.as_s32(); break;
            case valbox::type::U64: os << v.as_u64(); break;
            case valbox::type::S64: os << v.as_s64(); break;
            case valbox::type::FLOAT: os << v.as_float(); break;
            case valbox::type::DOUBLE: os << v.as_double(); break;
            case valbox::type::LONG_DOUBLE: os << v.as_long_double(); break;
            case valbox::type::BOOL: os << (v.as_bool() ? L"true" : L"false"); break;
            case valbox::type::WCHAR: os << v.as_wchar(); break;
            case valbox::type::STRING: os << v.cast_to_wstring(); break;
            case valbox::type::WSTRING: os << v.as_wstring(); break;
            case valbox::type::CLASS: os << L"<" << str_util::from_utf8(v.deref().class_name()) << L">"; break;
            case valbox::type::POINTER: os << v.deref(); break;
            case valbox::type::UNDEFINED: os << L"<undefined>"; break;
            case valbox::type::FUNC: os << L"<function>"; break;
            case valbox::type::ARRAY: {
                auto const &a{v.as_array()};
                os << L"[";
                std::wstring sep{};
                for(auto &&v: a) {
                    os << sep << v; sep = L",";
                }
                os << L"]";
            }
            break;
            case valbox::type::VEC4: {
                std::wstringstream ss{};
                ss << L"vec4{";
                std::wstring sep{};
                for(std::size_t i{0}; i < 4; ++i) {
                    ss << sep << std::fixed << v.as_vec4()[i];
                    if(sep.empty()) {
                        sep = L",";
                    }
                }
                ss << L"}";
                os << ss.str();
            }
            break;
            case valbox::type::MAT4: {
                std::wstringstream ss{};
                ss << L"mat4{";
                std::wstring sep1{};
                for(std::size_t r{0}; r < 4; ++r) {
                    ss << sep1 << L"{";
                    std::wstring sep2{};
                    for(std::size_t c{0}; c < 4; ++c) {
                        ss << sep2 << std::fixed << v.as_mat4().at(r, c);
                        sep2 = L", ";
                    }
                    ss << L"}";
                    if(sep1.empty()) {
                        sep1 = L",";
                    }
                }
                ss << L"}";
                os << ss.str();
            }
            break;
            case valbox::type::OBJECT: os << str_util::from_utf8(v.to_json().serialize5()); break;
            default: os << L"<corrupted>"; break;
        }
        return os;
    }

}
