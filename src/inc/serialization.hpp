#pragma once

#include "commondefs.hpp"
#include "sys_util.hpp"
#include "bit_util.hpp"

namespace teal {

    class serializer;
    class serial_reader;

#pragma pack(push, 1)
    namespace detail {

        class bad_serial_cast: public std::bad_cast {
        public:
            bad_serial_cast(std::string const &err): msg_{err} {
            }

            ~bad_serial_cast() noexcept {
            }

            char const *what() const noexcept {
                return msg_.c_str();
            }

        private:
            std::string msg_;
        };

#if !defined(USE_64_BIT_SERIALIZATION)
        static std::uint64_t n_size(std::uint8_t first) {
            if(first < 0x80) { return 1; }
            if(((first & 0xe0) == 0xc0)) { return 2; }
            if(((first & 0xf0) == 0xe0)) { return 3; }
            if(((first & 0xf8) == 0xf0)) { return 4; }
            if(((first & 0xfc) == 0xf8)) { return 5; }
            if(((first & 0xfe) == 0xfc)) { return 6; }
            return 0;
        }

        static std::uint64_t m_to_n(const std::uint8_t *mbuf, uint64_t *increment) {
            if(mbuf[0] < 0x80) {
                if(increment) { *increment = 1; }
                return mbuf[0] & 0xff;
            }
            if(((mbuf[0] & 0xe0) == 0xc0)) {
                if(increment) { *increment = 2; }
                return (((std::uint64_t)mbuf[0] & 0x1f) << 6) |
                       (((std::uint64_t)mbuf[1] & 0x3f));
            }
            if(((mbuf[0] & 0xf0) == 0xe0)) {
                if(increment) { *increment = 3; }
                return (((std::uint64_t)mbuf[0] & 0x0f) << 12) |
                       (((std::uint64_t)mbuf[1] & 0x3f) << 6) |
                       (((std::uint64_t)mbuf[2] & 0x3f));
            }
            if(((mbuf[0] & 0xf8) == 0xf0)) {
                if(increment) { *increment = 4; }
                return (((std::uint64_t)mbuf[0] & 0x07) << 18) |
                       (((std::uint64_t)mbuf[1] & 0x3f) << 12) |
                       (((std::uint64_t)mbuf[2] & 0x3f) << 6) |
                       (((std::uint64_t)mbuf[3] & 0x3f));
            }
            if(((mbuf[0] & 0xfc) == 0xf8)) {
                if(increment) { *increment = 5; }
                return (((std::uint64_t)mbuf[0] & 0x03) << 24) |
                       (((std::uint64_t)mbuf[1] & 0x3f) << 18) |
                       (((std::uint64_t)mbuf[2] & 0x3f) << 12) |
                       (((std::uint64_t)mbuf[3] & 0x3f) << 6) |
                       (((std::uint64_t)mbuf[4] & 0x3f));
            }
            if(((mbuf[0] & 0xfe) == 0xfc)) {
                if(increment) { *increment = 6; }
                return (((std::uint64_t)mbuf[0] & 0x01) << 30) |
                       (((std::uint64_t)mbuf[1] & 0x3f) << 24) |
                       (((std::uint64_t)mbuf[2] & 0x3f) << 18) |
                       (((std::uint64_t)mbuf[3] & 0x3f) << 12) |
                       (((std::uint64_t)mbuf[4] & 0x3f) << 6) |
                       (((std::uint64_t)mbuf[5] & 0x3f));
            }
            if(increment) { *increment = 0; }
            return mbuf[0];
        }

        static std::uint64_t n_to_m(std::uint32_t c, std::uint8_t *mbuf) {
            if(c <= 0x7f) {
                mbuf[0] = (std::uint8_t)c & 0x7f;
                return 1;
            } else  if(c <= 0x7ff) {
                mbuf[0] = (std::uint8_t)((c >> 6) & 0x1f) | 0xc0;
                mbuf[1] = (std::uint8_t)(c & 0x3f) | 0x80;
                return 2;
            } else  if(c <= 0xffff) {
                mbuf[0] = (std::uint8_t)((c >> 12) & 0xf) | 0xe0;
                mbuf[1] = (std::uint8_t)((c >> 6) & 0x3f) | 0x80;
                mbuf[2] = (std::uint8_t)(c & 0x3f) | 0x80;
                return 3;
            } else  if(c <= 0x1fffff) {
                mbuf[0] = (std::uint8_t)((c >> 18) & 7) | 0xf0;
                mbuf[1] = (std::uint8_t)((c >> 12) & 0x3f) | 0x80;
                mbuf[2] = (std::uint8_t)((c >> 6) & 0x3f) | 0x80;
                mbuf[3] = (std::uint8_t)(c & 0x3f) | 0x80;
                return 4;
            } else  if(c <= 0x3ffffff) {
                mbuf[0] = (std::uint8_t)((c >> 24) & 3) | 0xf8;
                mbuf[1] = (std::uint8_t)((c >> 18) & 0x3f) | 0x80;
                mbuf[2] = (std::uint8_t)((c >> 12) & 0x3f) | 0x80;
                mbuf[3] = (std::uint8_t)((c >> 6) & 0x3f) | 0x80;
                mbuf[4] = (std::uint8_t)(c & 0x3f) | 0x80;
                return 5;
            } else {
                mbuf[0] = (std::uint8_t)((c >> 1) & 1) | 0xfc;
                mbuf[1] = (std::uint8_t)((c >> 24) & 0x3f) | 0x80;
                mbuf[2] = (std::uint8_t)((c >> 18) & 0x3f) | 0x80;
                mbuf[3] = (std::uint8_t)((c >> 12) & 0x3f) | 0x80;
                mbuf[4] = (std::uint8_t)((c >> 6) & 0x3f) | 0x80;
                mbuf[5] = (std::uint8_t)(c & 0x3f) | 0x80;
                return 6;
            }
        }

#else

        static std::uint64_t n_size(std::uint8_t first) {
            first = ~first;
            first |= first >> 1;
            first |= first >> 2;
            first |= first >> 4;
            return 9 - teal::bit_util::num_of_set_bits(first);
        }

        static std::uint64_t m_to_n(std::uint8_t const *mbuf, std::uint64_t *increment) {
            std::uint64_t b0{static_cast<std::uint64_t>(mbuf[0])};
            if(b0 <= 0x7f) {
                if(increment) { *increment = 1; }
                return b0;
            }
            if(((b0 & 0xc0) == 0x80)) {
                if(increment) { *increment = 2; }
                return ((b0 & 0x3f) << 8) |
                       ((std::uint64_t)mbuf[1]);
            }
            if(((b0 & 0xe0) == 0xc0)) {
                if(increment) { *increment = 3; }
                return ((b0 & 0x1f) << 16) |
                       (((std::uint64_t)mbuf[1]) << 8) |
                       ((std::uint64_t)mbuf[2]);
            }
            if(((b0 & 0xf0) == 0xe0)) {
                if(increment) { *increment = 4; }
                return ((b0 & 0x0f) << 24) |
                       (((std::uint64_t)mbuf[1]) << 16) |
                       (((std::uint64_t)mbuf[2]) << 8) |
                       ((std::uint64_t)mbuf[3]);
            }
            if(((b0 & 0xf8) == 0xf0)) {
                if(increment) { *increment = 5; }
                return ((b0 & 0x07) << 32) |
                       (((std::uint64_t)mbuf[1]) << 24) |
                       (((std::uint64_t)mbuf[2]) << 16) |
                       (((std::uint64_t)mbuf[3]) << 8) |
                       ((std::uint64_t)mbuf[4]);
            }
            if(((b0 & 0xfc) == 0xf8)) {
                if(increment) { *increment = 6; }
                return ((b0 & 0x03) << 40) |
                       (((std::uint64_t)mbuf[1]) << 32) |
                       (((std::uint64_t)mbuf[2]) << 24) |
                       (((std::uint64_t)mbuf[3]) << 16) |
                       (((std::uint64_t)mbuf[4]) << 8) |
                       ((std::uint64_t)mbuf[5]);
            }
            if(((b0 & 0xfe) == 0xfc)) {
                if(increment) { *increment = 7; }
                return ((b0 & 0x01) << 48) |
                       (((std::uint64_t)mbuf[1]) << 40) |
                       (((std::uint64_t)mbuf[2]) << 32) |
                       (((std::uint64_t)mbuf[3]) << 24) |
                       (((std::uint64_t)mbuf[4]) << 16) |
                       (((std::uint64_t)mbuf[5]) << 8) |
                       ((std::uint64_t)mbuf[6])
                    ;
            }
            if(b0 == 0xfe) {
                if(increment) { *increment = 8; }
                return (((std::uint64_t)mbuf[1]) << 48) |
                       (((std::uint64_t)mbuf[2]) << 40) |
                       (((std::uint64_t)mbuf[3]) << 32) |
                       (((std::uint64_t)mbuf[4]) << 24) |
                       (((std::uint64_t)mbuf[5]) << 16) |
                       (((std::uint64_t)mbuf[6]) << 8)  |
                       ((std::uint64_t)mbuf[7])
                    ;
            }
            if(b0 == 0xff) {
                if(increment) { *increment = 9; }
                return (((std::uint64_t)mbuf[1]) << 56) |
                       (((std::uint64_t)mbuf[2]) << 48) |
                       (((std::uint64_t)mbuf[3]) << 40) |
                       (((std::uint64_t)mbuf[4]) << 32) |
                       (((std::uint64_t)mbuf[5]) << 24) |
                       (((std::uint64_t)mbuf[6]) << 16) |
                       (((std::uint64_t)mbuf[7]) << 8)  |
                       ((std::uint64_t)mbuf[8])
                    ;
            }
            if(increment) { *increment = 0; }
            return b0;
        }

        static std::uint64_t n_to_m(std::uint64_t c, std::uint8_t *mbuf) {
            if(c <= 0x7fULL) {
                mbuf[0] = (std::uint8_t)c;
                return 1;
            } else if(c <= 0x3f'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 8) | 0x80);
                mbuf[1] = (std::uint8_t)(c & 0xff);
                return 2;
            } else if(c <= 0x1f'ff'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 16) | 0xc0);
                mbuf[1] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[2] = (std::uint8_t)(c & 0xff);
                return 3;
            } else if(c <= 0x0f'ff'ff'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 24) | 0xe0);
                mbuf[1] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[3] = (std::uint8_t)(c & 0xff);
                return 4;
            } else if(c <= 0x07'ff'ff'ff'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 32) | 0xf0);
                mbuf[1] = (std::uint8_t)((c >> 24) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[3] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[4] = (std::uint8_t)(c & 0xff);
                return 5;
            } else if(c <= 0x03'ff'ff'ff'ff'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 40) | 0xf8);
                mbuf[1] = (std::uint8_t)((c >> 32) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 24) & 0xff);
                mbuf[3] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[4] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[5] = (std::uint8_t)(c & 0xff);
                return 6;
            } else if(c <= 0x01'ff'ff'ff'ff'ff'ffULL) {
                mbuf[0] = (std::uint8_t)((c >> 48) | 0xfc);
                mbuf[1] = (std::uint8_t)((c >> 40) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 32) & 0xff);
                mbuf[3] = (std::uint8_t)((c >> 24) & 0xff);
                mbuf[4] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[5] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[6] = (std::uint8_t)(c & 0xff);
                return 7;
            } else if(c <= 0xff'ff'ff'ff'ff'ff'ffULL) {
                mbuf[0] = 0xfe;
                mbuf[1] = (std::uint8_t)((c >> 48) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 40) & 0xff);
                mbuf[3] = (std::uint8_t)((c >> 32) & 0xff);
                mbuf[4] = (std::uint8_t)((c >> 24) & 0xff);
                mbuf[5] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[6] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[7] = (std::uint8_t)(c & 0xff);
                return 8;
            } else {
                mbuf[0] = 0xff;
                mbuf[1] = (std::uint8_t)((c >> 56) & 0xff);
                mbuf[2] = (std::uint8_t)((c >> 48) & 0xff);
                mbuf[3] = (std::uint8_t)((c >> 40) & 0xff);
                mbuf[4] = (std::uint8_t)((c >> 32) & 0xff);
                mbuf[5] = (std::uint8_t)((c >> 24) & 0xff);
                mbuf[6] = (std::uint8_t)((c >> 16) & 0xff);
                mbuf[7] = (std::uint8_t)((c >> 8) & 0xff);
                mbuf[8] = (std::uint8_t)(c & 0xff);
                return 9;
            }
        }

#endif

        static std::uint64_t unumber_to_mem(std::uint64_t s, void *bytes) {
            return n_to_m(s, reinterpret_cast<std::uint8_t *>(bytes));
        }

        static std::uint64_t n_bytes(std::uint64_t num) {
            std::uint8_t mbuf[16];
            return unumber_to_mem(num, mbuf);
        }

        static std::uint64_t mem_to_unumber(void const *bytes, std::uint64_t &num) {
            std::uint64_t incr{0};
            num = m_to_n(reinterpret_cast<std::uint8_t const *>(bytes), &incr);
            return incr;
        }


        class size_surrounded_buffer final {
            friend class teal::serializer;
            friend class teal::serial_reader;
            friend class serialized_data_iter;

        public:
            using size_type = std::uint64_t;

        public:
            size_type size() const {
                std::uint64_t size_val{};
                mem_to_unumber(buffer_start(), size_val);
                return size_val;
            }

            std::int64_t as_number() const {
                std::uint64_t s{size()};
                if(s == sizeof(std::int64_t)) {
                    return *reinterpret_cast<int64_t const *>(data());
                } else if(s == sizeof(std::int32_t)) {
                    return static_cast<std::int64_t>(*reinterpret_cast<std::int32_t const *>(data()));
                } else if(s == sizeof(std::int16_t)) {
                    return static_cast<std::int64_t>(*reinterpret_cast<std::int16_t const *>(data()));
                } else if(s == sizeof(std::int8_t)) {
                    return static_cast<std::int64_t>(*reinterpret_cast<std::int8_t const *>(data()));
                }
                throw bad_serial_cast{"Invalid content for being a number"};
            }

            std::uint64_t as_unumber() const {
                std::uint64_t s{size()};
                if(s == sizeof(std::uint64_t)) {
                    return *reinterpret_cast<std::uint64_t const *>(data());
                } else if(s == sizeof(std::uint32_t)) {
                    return static_cast<std::uint64_t>(*reinterpret_cast<std::uint32_t const *>(data()));
                } else if(s == sizeof(std::uint16_t)) {
                    return static_cast<uint64_t>(*reinterpret_cast<std::uint16_t const *>(data()));
                } else if(s == sizeof(std::uint8_t)) {
                    return static_cast<std::uint64_t>(*reinterpret_cast<std::uint8_t const *>(data()));
                }
                throw bad_serial_cast{"Invalid content for being a number"};
            }

            template<typename T>
            T as_fpnum() const {
                std::uint64_t s{size()};
                if(s == sizeof(float)) {
                    return *reinterpret_cast<float const *>(data());
                } else if(s == sizeof(double)) {
                    return *reinterpret_cast<double const *>(data());
                } else if(s == sizeof(long double)) {
                    return *reinterpret_cast<long double const *>(data());
                } else {
                    return as_number();
                }
            }

            std::string as_string() const {
                std::uint64_t s{size()};
                char const *d{reinterpret_cast<char const *>(data())};
                std::string res{d, d + s};
                return res;
            }

#if (__cplusplus >= 201700L)
            std::string_view as_string_view() const {
                std::uint64_t s{size()};
                return std::string_view{reinterpret_cast<char const *>(data()),
                                        static_cast<std::size_t>(s)};
            }
#endif

            std::wstring as_wstring() const {
                std::uint64_t s{size()};
                if(s % sizeof(std::wstring::value_type)) {
                    throw bad_serial_cast{"Invalid content for being a unicode string"};
                }
                return std::wstring{reinterpret_cast<std::wstring::value_type const *>(data()),
                                    reinterpret_cast<std::wstring::value_type const *>(data()) +
                                        static_cast<std::size_t>(s / sizeof(std::wstring::value_type))};
            }

            std::wstring_view as_wstring_view() const {
                std::uint64_t s{size()};
                if(s % sizeof(std::wstring_view::value_type)) {
                    throw bad_serial_cast{"Invalid content for being a unicode string"};
                }
                return std::wstring_view{reinterpret_cast<std::wstring_view::value_type const *>(data()),
                                         static_cast<std::size_t>(s / sizeof(std::wstring_view::value_type))};
            }

            teal::bytevec as_bytevec() const {
                if(size()) {
                    return teal::bytevec{
                        reinterpret_cast<std::uint8_t const *>(data()),
                        reinterpret_cast<std::uint8_t const *>(data()) + size()
                    };
                } else {
                    return teal::bytevec{};
                }
            }

            template<typename U>
            U const &as() const {
                if(size() != sizeof(U)) {
                    throw bad_serial_cast{"Invalid content cast in serialized data"};
                }
                return *reinterpret_cast<const U *>(data());
            }

            template<typename U>
            U &as() {
                if(size() != sizeof(U)) {
                    throw bad_serial_cast{"Invalid content cast in serialized data"};
                }
                return *reinterpret_cast<U *>(data());
            }

            void const *data() const {
                return buffer_start() + size_size();
            }

        private:
            void *data() {
                return buffer_start() + size_size();
            }

            void set_sizes(size_type s) {
                unumber_to_mem(s, buffer_start());
                unumber_to_mem(s, reinterpret_cast<std::uint8_t *>(data()) + s);
                teal::bit_util::inplace_swap(reinterpret_cast<std::uint8_t *>(data()) + s, size_size());
            }

            void set_data(void const *d, size_type s) {
                if(d && s) {
                    set_sizes(s);
                    std::copy(
                        reinterpret_cast<std::uint8_t const *>(d),
                        reinterpret_cast<std::uint8_t const *>(d) + s,
                        reinterpret_cast<std::uint8_t *>(data())
                    );
                }
            }

            size_type size_size() const {
                std::uint64_t size_val{};
                return mem_to_unumber(buffer_start(), size_val);
            }

            bool valid(std::uint8_t const *e) const {
                std::uint8_t const *b{buffer_start()};
                std::intptr_t const whole_size_limit{(std::intptr_t)e - (std::intptr_t)b};
                if(whole_size_limit > 0) {
                    std::uint64_t const ss{n_size(*b)};
                    if((std::int64_t)(ss * 2) <= whole_size_limit) {
                        std::uint64_t s{size()};
                        if((std::int64_t)(ss * 2 + s) <= whole_size_limit) {
                            std::uint8_t const *bs_last_ptr{b + ss * 2 + s - 1};
                            std::uint64_t const bss{n_size(*bs_last_ptr)};
                            if(bss == ss) {
                                std::uint8_t buff[16]{};
                                std::uint8_t const *bs_ptr{b + ss + s};
                                if(ss > 1) { std::memcpy(buff, bs_ptr, ss); } else { buff[0] = *bs_ptr; }
                                std::uint64_t bs{};
                                if(ss > 1) { teal::bit_util::inplace_swap(buff, ss); }
                                mem_to_unumber(buff, bs);
                                if(bs == s) {
                                    return true;
                                }
                            }
                        }
                    }
                }
                return false;
            }

            size_surrounded_buffer *prev() const {
                std::uint8_t const *prev_last_byte_ptr{buffer_start() - 1};
                std::uint64_t prev_size_size{n_size(*prev_last_byte_ptr)};

                std::uint8_t const *prev_last_size_ptr{buffer_start() - prev_size_size};

                std::uint8_t buff[16];
                std::memcpy(buff, prev_last_size_ptr, prev_size_size);
                teal::bit_util::inplace_swap(buff, prev_size_size);

                std::uint64_t prev_size{0};
                mem_to_unumber(buff, prev_size);
                return reinterpret_cast<size_surrounded_buffer *>(const_cast<std::uint8_t *>(buffer_start()) - (prev_size_size * 2 + prev_size));
            }

            size_surrounded_buffer *next() const {
                return (size_surrounded_buffer *)(buffer_start() + (size_size() * 2 + size()));
            }

        private:
            std::uint8_t *buffer_start() { return reinterpret_cast<std::uint8_t *>(this); }
            std::uint8_t const *buffer_start() const { return reinterpret_cast<std::uint8_t const *>(this); }
        };


        class serialized_data_iter final {
        public:
            using size_type = std::uint64_t;

        public:
            serialized_data_iter() = default;

            serialized_data_iter(size_surrounded_buffer const *x,
                                 std::uint8_t const *whole_data,
                                 size_type data_size):
                p_{x},
                whole_data_{whole_data},
                data_size_{data_size}
            {
            }

            serialized_data_iter(serialized_data_iter const &) = default;

            serialized_data_iter &operator=(serialized_data_iter const &) = default;

            serialized_data_iter(serialized_data_iter &&) = default;

            serialized_data_iter &operator=(serialized_data_iter &&) = default;

            ~serialized_data_iter() = default;

            bool valid() const {
                if(p_) {
                    return p_->valid(whole_data_ + data_size_);
                }
                return false;
            }

            serialized_data_iter &operator--() {
                if(reinterpret_cast<size_surrounded_buffer const *>(p_) != reinterpret_cast<size_surrounded_buffer const *>(whole_data_)) {
                    if(((std::uint8_t *)p_) > whole_data_) {
                        p_ = p_->prev();
                        return_into_bounds_if_needed();
                    }
                }
                return *this;
            }

            serialized_data_iter operator--(int) {
                serialized_data_iter tmp{*this};
                operator--();
                return tmp;
            }

            serialized_data_iter &operator++() {
                std::uint8_t const *e{whole_data_ + data_size_};
                if(((std::uint8_t *)p_) > e) {
                    p_ = reinterpret_cast<size_surrounded_buffer const *>(e);
                } else {
                    p_ = p_->next();
                    return_into_bounds_if_needed();
                }
                return *this;
            }

            serialized_data_iter operator++(int) {
                serialized_data_iter tmp{*this};
                operator++();
                return tmp;
            }

            serialized_data_iter operator+(std::int64_t incr) const {
                serialized_data_iter tmp{*this};

                if(incr < 0) {
                    for(std::int64_t i{0}; i < -incr; ++i) { --tmp; }
                } else {
                    for(std::int64_t i{0}; i < incr; ++i) { ++tmp; }
                }

                return tmp;
            }

            serialized_data_iter operator-(std::int64_t incr) const {
                serialized_data_iter tmp{*this};

                if(incr < 0) {
                    for(std::int64_t i{0}; i < -incr; i++) { ++tmp; }
                } else {
                    for(std::int64_t i{0}; i < incr; i++) { --tmp; }
                }

                return tmp;
            }

            serialized_data_iter &operator+=(std::int64_t incr) {
                *this = *this + incr;
                return *this;
            }

            serialized_data_iter &operator-=(std::int64_t incr) {
                *this = *this - incr;
                return *this;
            }

            bool operator<(serialized_data_iter const &rhs) const {
                return p_ < rhs.p_;
            }

            bool operator<=(serialized_data_iter const &rhs) const {
                return p_ <= rhs.p_;
            }

            bool operator>(serialized_data_iter const &rhs) const {
                return p_ > rhs.p_;
            }

            bool operator>=(serialized_data_iter const &rhs) const {
                return p_ >= rhs.p_;
            }

            bool operator==(serialized_data_iter const &rhs) const {
                return p_ == rhs.p_;
            }

            bool operator!=(serialized_data_iter const &rhs) const {
                return p_ != rhs.p_;
            }

            size_surrounded_buffer const &operator*() const {
                return *p_;
            }

            size_surrounded_buffer const *operator->() const {
                return p_;
            }

            operator bool() const {
                return !!p_;
            }

        private:
            void return_into_bounds_if_needed() {
                std::uint8_t const *e{whole_data_ + data_size_};
                if(((std::uint8_t *)p_) > e) {
                    p_ = reinterpret_cast<size_surrounded_buffer const *>(e);
                } else if(((std::uint8_t *)p_) < whole_data_) {
                    p_ = reinterpret_cast<size_surrounded_buffer const *>(whole_data_);
                }
            }

        private:
            size_surrounded_buffer const *p_{nullptr};
            std::uint8_t const *whole_data_{nullptr};
            std::uint64_t data_size_{0};
        };
    }


    class serializer final {
    public:
        using size_type = std::uint64_t;
        using iterator = detail::serialized_data_iter;
        using const_iterator = detail::serialized_data_iter;

    public:
        serializer() = default;
        serializer(serializer const &) = default;
        serializer(serializer &&) = default;
        serializer &operator=(serializer const &) = default;
        serializer &operator=(serializer &&) = default;
        ~serializer() = default;

        serializer(void const *data, size_type size) {
            if(data && size) {
                serialized_data_ = teal::bytevec{
                    reinterpret_cast<std::uint8_t const *>(data),
                    reinterpret_cast<std::uint8_t const *>(data) + size
                };
            }
        }

        serializer(teal::bytevec const &data):
            serialized_data_{data} {
        }

        serializer(teal::bytevec &&data):
            serialized_data_{std::move(data)} {
        }

        serializer &operator=(teal::bytevec const &data) {
            serialized_data_ = data;
            return *this;
        }

        serializer &operator=(teal::bytevec &&data) {
            serialized_data_ = std::move(data);
            return *this;
        }

        teal::bytevec const &data_vec() const & {
            return serialized_data_;
        }

        teal::bytevec take_vec() {
            return std::move(serialized_data_);
        }

        void reset() {
            serialized_data_.clear();
        }

        void clear() {
            serialized_data_.clear();
        }

        void reserve(size_type rsv_size) {
            serialized_data_.reserve(rsv_size);
        }

        size_type size() const {
            return serialized_data_.size();
        }

        std::uint8_t const *data() const {
            return serialized_data_.data();
        }

        void push_back(void const *d, size_type s) {
            std::uint64_t new_data_pos{size()};
            serialized_data_.resize(new_data_pos + s + detail::n_bytes(s) * 2);
            detail::size_surrounded_buffer *buf{
                reinterpret_cast<detail::size_surrounded_buffer *>(
                    (reinterpret_cast<char *>(serialized_data_.data())) + new_data_pos
                )
            };
            buf->set_data(d, s);
        }

        void push_back(std::string const &str) {
            push_back(str.c_str(), str.size() * sizeof(std::string::value_type));
        }

        void push_back(std::string_view str) {
            push_back(str.data(), str.size() * sizeof(std::string_view::value_type));
        }

        void push_back(char const *str) {
            push_back(str, strlen(str) * sizeof(char));
        }

        void push_back(std::wstring const &str) {
            push_back(str.data(), str.size() * sizeof(std::wstring::value_type));
        }

        void push_back(std::wstring_view const &str) {
            push_back(str.data(), str.size() * sizeof(std::wstring_view::value_type));
        }

        void push_back(wchar_t const *str) {
            push_back(str, (::wcslen(str)) * sizeof(wchar_t));
        }

        void push_back(teal::bytevec const &vec) {
            push_back(vec.data(), vec.size());
        }

        template<size_type N>
        void push_back(std::array<std::uint8_t, N> const &arr) {
            push_back(arr.data(), arr.size());
        }

        template<typename PUSHED_T>
        void push_back(PUSHED_T const &d) {
            if constexpr (sys_util::little_endian()) {
                push_back(&d, sizeof(d));
            } else {
                PUSHED_T bd{bit_util::swap_on_be<PUSHED_T>{d}.val};
                push_back(&bd, sizeof(bd));
            }
        }

        iterator operator[](size_type indx) {
            size_type i{0};
            for(iterator it{begin()}; it != end(); ++it, ++i) {
                if(i == indx) {
                    return it;
                }
            }
            throw std::range_error{"index out of range"};
        }

        const_iterator operator[](size_type indx) const {
            size_type i{0};
            for(const_iterator it{cbegin()}; it != cend(); ++it, ++i) {
                if(i == indx) {
                    return it;
                }
            }
            throw std::range_error{"index out of range"};
        }

        iterator begin() {
            iterator res{
                reinterpret_cast<detail::size_surrounded_buffer *>(
                    serialized_data_.data()
                ),
                serialized_data_.data(),
                serialized_data_.size()
            };
            return res;
        }

        iterator end() {
            return iterator{
                reinterpret_cast<detail::size_surrounded_buffer *>(
                    serialized_data_.data() + serialized_data_.size()
                ),
                serialized_data_.data(),
                serialized_data_.size()
            };
        }

        const_iterator begin() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(serialized_data_.data()),
                serialized_data_.data(),
                serialized_data_.size()
            };
        }

        const_iterator end() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(
                    serialized_data_.data() + serialized_data_.size()
                ),
                serialized_data_.data(),
                serialized_data_.size()
            };
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        template<typename PUSHED_T>
        serializer &operator<<(PUSHED_T const &v) {
            push_back(v);
            return *this;
        }

        serializer &operator<<(std::string const &str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(std::string_view str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(char const *str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(std::wstring const &str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(std::wstring_view str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(wchar_t const *str) {
            push_back(str);
            return *this;
        }

        serializer &operator<<(teal::bytevec const &vec) {
            push_back(vec);
            return *this;
        }

        template<size_type N>
        serializer &operator<<(std::array<std::uint8_t, N> const &arr) {
            push_back(arr.data(), arr.size());
            return *this;
        }

    private:
        teal::bytevec serialized_data_{};
    };


    class serial_reader final {
    public:
        using size_type = std::uint64_t;
        using const_iterator = detail::serialized_data_iter;

    public:
        serial_reader() = default;

        serial_reader(void const *data, size_type data_size):
            data_{reinterpret_cast<std::uint8_t const *>(data)},
            data_size_{data_size}
        {
        }

        serial_reader(teal::bytevec const &data):
            data_{reinterpret_cast<std::uint8_t const *>(data.data())},
            data_size_{data.size()}
        {
        }

        void assign(void const *data, size_type data_size) {
            data_ = reinterpret_cast<std::uint8_t const *>(data);
            data_size_ = data_size;
        }

        size_type size() const {
            return data_size_;
        }

        std::uint8_t const *data() const {
            return data_;
        }

        std::vector<std::uint8_t> data_vec() const {
            if(data_ != nullptr && data_size_ > 0) {
                return std::vector<std::uint8_t>{data_, data_ + data_size_};
            }
            return {};
        }

        const_iterator begin() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(data()), data(), size()
            };
        }

        const_iterator end() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(data() + size()), data(), size()
            };
        }

        const_iterator cbegin() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(data()), data(), size()
            };
        }

        const_iterator cend() const {
            return const_iterator{
                reinterpret_cast<detail::size_surrounded_buffer const *>(data() + size()), data(), size()
            };
        }

    private:
        std::uint8_t const *data_{nullptr};
        std::uint64_t data_size_{0};
    };
#pragma pack(pop)

}
