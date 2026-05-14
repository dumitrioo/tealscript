#pragma once

#include "inc/commondefs.hpp"
#include "inc/file_util.hpp"
#include "inc/str_util.hpp"
#include "inc/hash/hash.hpp"
#include "inc/mt_synchro.hpp"
#include "inc/math/math_util.hpp"
#include "inc/json.hpp"
#ifndef NOT_USING_TEAL_USE_EMHASH8_MAP
#include "inc/emhash/hash_set8.hpp"
#include "inc/emhash/hash_table8.hpp"
#endif

#define TEALFUN(ARGS) [&](std::vector<teal::valbox> &ARGS) -> teal::valbox
#define TEALNUMARG(ARGS, INDX, TYPE) ARGS[INDX].cast_num_to_num<TYPE>()
#define TEAL_CHCK_FUN_PARMS_NUM_EQ(ARGS, NUM_ARGS) \
    if(ARGS.size() != (NUM_ARGS)) { \
        throw std::runtime_error{"wrong actual function arguments number"}; \
    }
#define TEAL_CHCK_FUN_PARMS_NUM_LE(ARGS, NUM_ARGS) \
    if(ARGS.size() > (NUM_ARGS)) { \
        throw std::runtime_error{"wrong actual function arguments number"}; \
    }
#define TEAL_CHCK_FUN_PARMS_NUM_GE(ARGS, NUM_ARGS) \
    if(ARGS.size() < (NUM_ARGS)) { \
        throw std::runtime_error{"wrong actual function arguments number"}; \
    }
#define TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(ARGS, NUM_ARGS_MIN, NUM_ARGS_MAX) \
    if(!(ARGS.size() >= (NUM_ARGS_MIN) && ARGS.size() <= (NUM_ARGS_MAX))) { \
        throw std::runtime_error{"wrong actual function arguments number"}; \
    }
#define TEALCLASSARG(ARGS, INDX, TYPE) ARGS[INDX].as_class<TYPE>()
#define TEALTHIS(ARGS, TYPE) ARGS[0].as_class<TYPE>()

namespace teal {

#define TEAL_DEFINE_BASE_LINE_COL_ERROR(CLASS, BASE_CLASS) class CLASS: public BASE_CLASS { \
    public: \
        CLASS(std::int64_t l, std::int64_t c, std::string const &msg): \
            BASE_CLASS{msg}, \
            l_{l}, \
            c_{c} \
        { \
        } \
        char const *what() const noexcept override { \
            try { \
                std::stringstream ss{}; \
                ss << "at " << l_ + 1 << ":" << c_ + 1 << " - " << BASE_CLASS::what(); \
                buf_ = ss.str(); \
                return buf_.c_str(); \
            } catch(...) { \
            } \
            return def_; \
        } \
    private: \
        static inline char const *def_{""}; \
        mutable std::string buf_{}; \
        std::int64_t l_{}; \
        std::int64_t c_{}; \
    };

#define TEAL_DEFINE_DERIVED_ERROR(CLASS, BASE_CLASS) class CLASS: public BASE_CLASS { \
    public: \
        CLASS(std::int64_t l, std::int64_t c, const std::string &msg): BASE_CLASS{l, c, msg} { \
        } \
    };

    TEAL_DEFINE_BASE_LINE_COL_ERROR(logic_error, std::logic_error)
    TEAL_DEFINE_BASE_LINE_COL_ERROR(runtime_error, std::runtime_error)
    TEAL_DEFINE_DERIVED_ERROR(compilation_error, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(domain_error, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(future_error, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(invalid_argument, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(length_error, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(out_of_range, logic_error)
    TEAL_DEFINE_DERIVED_ERROR(range_error, runtime_error)
    TEAL_DEFINE_DERIVED_ERROR(overflow_error, runtime_error)
    TEAL_DEFINE_DERIVED_ERROR(underflow_error, runtime_error)
    TEAL_DEFINE_DERIVED_ERROR(system_error, runtime_error)

#undef TEAL_DEFINE_BASE_LINE_COL_ERROR
#undef TEAL_DEFINE_DERIVED_ERROR

    template <std::uint8_t T_numBytes>
    using UintSelector =
        typename std::conditional<T_numBytes == 1, std::uint8_t,
            typename std::conditional<T_numBytes == 2, std::uint16_t,
                typename std::conditional<T_numBytes == 3 || T_numBytes == 4, std::uint32_t,
                    std::uint64_t
                >::type
            >::type
        >::type;

#ifdef TEAL_DEBUGGING
    template<typename K_T, typename V_T>
    using num_map_t = std::map<K_T, V_T>;
    template<typename V_T>
    using str_map_t = std::map<std::string, V_T>;
#else
    #ifndef NOT_USING_TEAL_USE_EMHASH8_MAP
    template<typename K_T, typename V_T>
    using num_map_t = emhash_8::HashMap<K_T, V_T, num_cast_hash<K_T>>;
    template<typename V_T>
    using str_map_t = emhash_8::HashMap<std::string, V_T/*, str_crc<std::string>*/>;
    #else
    template<typename K_T, typename V_T>
    using num_map_t = std::unordered_map<K_T, V_T>;
    template<typename V_T>
    using str_map_t = std::unordered_map<std::string, V_T>;
    #endif
#endif


#ifdef TEAL_USE_CUSTOM_SHARED_MUTEX
    using shared_mutex = mt::atomic_rw_spin_mutex;
#else
    using shared_mutex = std::shared_mutex;
#endif

    using mutex = std::mutex;

    static bool is_identifier(std::string const &ident) {
        if(ident.size() == 0) {
            return false;
        }
        std::wstring wid{teal::str_util::from_utf8(ident)};
        if(!(teal::str_util::fltr<std::wstring>::isalpha(wid[0]) || wid[0] == '_')) {
            return false;
        }
        for(std::size_t i{1}; i < wid.size(); ++i) {
            if(!(teal::str_util::fltr<std::wstring>::isalnum(wid[0]) || wid[0] == '_')) {
                return false;
            }
        }
        return true;
    }

    static bool is_keyword(std::wstring const &str) {
        return str == L"if" ||
               str == L"else" ||
               str == L"true" ||
               str == L"false" ||
               str == L"undefined" ||
               str == L"for" ||
               str == L"while" ||
               str == L"return" ||
               str == L"break" ||
               str == L"continue" ||
               str == L"try" ||
               str == L"catch" ||
               str == L"throw" ||
               str == L"function" ||
               str == L"this" ||
               str == L"sizeof" ||
               str == L"typeof"
        ;
    }

    static bool expression_part(std::wstring const &str) {
        return str != L"if" &&
               str != L"else" &&
               str != L"for" &&
               str != L"while" &&
               str != L"return" &&
               str != L"break" &&
               str != L"continue" &&
               str != L"try" &&
               str != L"catch" &&
               str != L"throw" &&
               str != L"function"
        ;
    }

    template<typename T>
    teal::math::vector4<T> vec4_from_json(json const &j) {
        teal::math::vector4<T> res{};
        try {
            if(j.is_array()) {
                for(std::size_t i = 0; i < 4 && i < j.size(); ++i) {
                    res[i] = j[i].try_as_long_double();
                }
            } else if(j.is_object()) {
                res.x() = j.key_exists("x") ? j["x"].try_as_long_double() : 0;
                res.y() = j.key_exists("y") ? j["y"].try_as_long_double() : 0;
                res.z() = j.key_exists("z") ? j["z"].try_as_long_double() : 0;
                res.w() = j.key_exists("w") ? j["w"].try_as_long_double() : 0;
            }
        } catch(...) {
            res = teal::math::vector4<T>{};
        }
        return res;
    }

    template<typename T>
    teal::math::vector4<T> vec4_from_str(std::string const &s) {
        teal::math::vector4<T> res{};
        try {
            json j{json::deserialize(s)};
            res = vec4_from_json<T>(j);
        } catch(...) {
            res = teal::math::vector4<T>{};
        }
        return res;
    }

    template<typename T>
    teal::math::vector4<T> vec4_from_str(std::wstring const &s) {
        teal::math::vector4<T> res{};
        try {
            json j{json::deserialize(teal::str_util::to_utf8(s))};
            res = vec4_from_json<T>(j);
        } catch(...) {
            res = teal::math::vector4<T>{};
        }
        return res;
    }

    template<typename T>
    teal::math::matrix4<T> mat4_from_json(json const &j) {
        teal::math::matrix4<T> res{};
        try {
            if(j.is_array()) {
                for(std::size_t i = 0; i < 16 && i < j.size(); ++i) {
                    res[i] = j[i].try_as_long_double();
                }
            } else if(j.is_object()) {
                res.x() = j.key_exists("x") ? j["x"].try_as_long_double() : 0;
                res.y() = j.key_exists("y") ? j["y"].try_as_long_double() : 0;
                res.z() = j.key_exists("z") ? j["z"].try_as_long_double() : 0;
                res.w() = j.key_exists("w") ? j["w"].try_as_long_double() : 0;
            }
        } catch(...) {
            res = teal::math::matrix4<T>{};
        }
        return res;
    }

    template<typename T>
    class map_array {
    public:
        map_array() {}

        T &operator[](std::size_t indx) & {
            if(indx >= size_) {
                size_ = indx + 1;
            }
            return map_[indx];
        }

        T at(std::size_t indx) const {
            if(indx >= size_) {
                throw std::runtime_error{"index out of range"};
            }
            auto it{map_.find(indx)};
            if(it == map_.end()) {
                return T{};
            }
            return it->second;
        }

        std::size_t size() const {
            return size_;
        }

        void resize(std::size_t n) {
            size_ = n;
            for(auto it{map_.lower_bound(n)}; it != map_.end(); ) {
                it = map_.erase(it);
            }
        }

        std::map<std::size_t, T> const &m() const & {
            return map_;
        }

        map_array subarray(
            std::size_t start_index,
            std::size_t count = std::numeric_limits<std::size_t>::max()
        ) const {
            map_array res{};
            res.resize(std::min(count, size() - start_index));
            for(auto it{map_.lower_bound(start_index)}; it != map_.end(); ++ it) {
                if(it->first - start_index >= count) {
                    break;
                }
                res[it->first] = it->second;
            }
            return res;
        }

        bool contains(size_t indx) const {
            return indx < size_ && map_.find(indx) != map_.end();
        }

        void traverse_full(
            std::function<void(size_t, T const &)> fun,
            size_t start = 0,
            size_t count = std::numeric_limits<std::size_t>::max()
        ) const {
            auto it{map_.lower_bound(start)};
            for(size_t i{start}; i < size_ && i < start + count; ++i) {
                if(it != map_.end() && i == it->first) {
                    fun(i, it->second);
                    ++it;
                } else {
                    fun(i, T{});
                }
            }
        }

        void traverse_real(
            std::function<void(size_t, T const &)> fun,
            size_t start = 0,
            size_t count = std::numeric_limits<std::size_t>::max()
        ) const {
            for(
                auto it{map_.lower_bound(start)};
                it != map_.end() && it->first < start + count;
                ++it
            ) {
                fun(it->first, it->second);
            }
        }

        void clear() {
            size_ = 0;
            map_.clear();
        }

        bool empty() const {
            return size_ == 0;
        }

        void push_back(T const &v) {
            map_[size_++] = v;
        }

        void push_back(T &&v) {
            map_[size_++] = std::move(v);
        }

        T pop_back() {
            T res{};
            if(empty()) {
                throw std::runtime_error{"array empty"};
            }
            --size_;
            auto it{map_.lower_bound(size_)};
            if(it != map_.end()) {
                res = std::move(it->second);
                map_.erase(it);
            }
            return res;
        }

    private:
        std::map<std::size_t, T> map_{};
        std::size_t size_{0};
    };

}
