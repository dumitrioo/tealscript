#pragma once

#include "commondefs.hpp"

namespace teal::mt {

    class atomic_spin_mutex {
    public:
        atomic_spin_mutex() = default;
        ~atomic_spin_mutex() = default;
#ifdef MUTEX_COPYABLE_WITHOUT_ACTUAL_COPYING
        atomic_spin_mutex(const atomic_spin_mutex &) {}
        atomic_spin_mutex& operator=(const atomic_spin_mutex &) { return *this; }
        atomic_spin_mutex(atomic_spin_mutex &&) noexcept {}
        atomic_spin_mutex &operator=(atomic_spin_mutex &&) noexcept { return *this; }
#else
        atomic_spin_mutex(const atomic_spin_mutex &) = delete;
        atomic_spin_mutex& operator=(const atomic_spin_mutex &) = delete;
        atomic_spin_mutex(atomic_spin_mutex &&that) = delete;
        atomic_spin_mutex &operator=(atomic_spin_mutex &&that) = delete;
#endif
        void reset() noexcept {
            current_ = 0;
        }

        // stl inerface
        void lock() { lock_for_write();  }
        bool try_lock() { return try_lock_for_write(); }
        void unlock() { write_unlock(); }

        typedef void *native_handle_type;
        native_handle_type native_handle() { return &current_; }

    private:
        void lock_for_write() noexcept {
            std::int64_t wanted{0};
            while(!current_.compare_exchange_weak(wanted, 1, std::memory_order_release, std::memory_order_acquire)) {
                wanted = 0;
#if MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
                maybe_wait();
#endif
            }
        }

        bool try_lock_for_write() noexcept {
            std::int64_t zero{0};
            return current_.compare_exchange_strong(zero, 1, std::memory_order_release, std::memory_order_acquire);
        }

        void write_unlock() {
            if(current_.fetch_sub(1) != 1) {
                throw std::runtime_error{"locking counting error"};
            }
        }

    private:
#if MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
        void maybe_wait() {
            std::this_thread::sleep_for(std::chrono::nanoseconds{MUTEX_ATOMIC_SM_SLEEP_NANOS});
        }
#endif

    private:
        std::atomic<std::int64_t> current_{0};
    };


    class atomic_rw_spin_mutex {
    public:
        atomic_rw_spin_mutex(
#ifdef RW_MUTEX_PRIORITIES
            bool prefer_writers = true
#endif
        )
#ifdef RW_MUTEX_PRIORITIES
            : prefer_writers_{prefer_writers ? 1 : 0}
#endif
        {}
        ~atomic_rw_spin_mutex() = default;
#ifdef RW_MUTEX_COPYABLE_WITHOUT_ACTUAL_COPYING
        atomic_rw_spin_mutex(const atomic_rw_spin_mutex &) {}
        atomic_rw_spin_mutex& operator=(const atomic_rw_spin_mutex &) { return *this; }
        atomic_rw_spin_mutex(atomic_rw_spin_mutex &&) noexcept {}
        atomic_rw_spin_mutex &operator=(atomic_rw_spin_mutex &&) noexcept { return *this; }
#else
        atomic_rw_spin_mutex(const atomic_rw_spin_mutex &) = delete;
        atomic_rw_spin_mutex& operator=(const atomic_rw_spin_mutex &) = delete;
        atomic_rw_spin_mutex(atomic_rw_spin_mutex &&that) = delete;
        atomic_rw_spin_mutex &operator=(atomic_rw_spin_mutex &&that) = delete;
#endif
        bool lock(bool w) noexcept { return w ? lock_for_write() : lock_for_read(); }
        void unlock(bool w) noexcept { if(w) write_unlock(); else read_unlock(); }
        bool try_lock(bool w) noexcept { return w ? try_lock_for_write() : try_lock_for_read(); }
#ifdef RW_MUTEX_UPGRADEABLE
        bool upgrade() {
            if(current_.load() <= 0) {
                throw std::runtime_error{"locking counting error"};
            }
            std::int64_t wanted{0};
            if(!pending_upgraders_.compare_exchange_strong(wanted, 1)) {
                return false;
            }
            shut_on_destroy unpend{[this]() { pending_upgraders_.store(0); }};
            wanted = 1;
            while(!current_.compare_exchange_weak(wanted, -1, std::memory_order_release, std::memory_order_acquire)) {
                if(wanted <= 0) {
                    throw std::runtime_error{"locking counting error"};
                }
                wanted = 1;
#if RW_MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
                maybe_wait();
#endif
            }
            return true;
        }

        void downgrade() {
            std::int64_t wanted{-1};
            if(!current_.compare_exchange_strong(wanted, 1)) {
                throw std::runtime_error{"locking counting error"};
            }
        }
#endif
        void reset(
#ifdef RW_MUTEX_PRIORITIES
            bool prefer_writers = true
#endif
        ) noexcept {
            current_ = 0;
#ifdef RW_MUTEX_UPGRADEABLE
            pending_upgraders_ = 0;
#endif
#ifdef RW_MUTEX_PRIORITIES
            pending_readers_ = 0;
            pending_writers_ = 0;
            prefer_writers_ = prefer_writers ? 1 : 0;
#endif
        }

        // stl inerface
        void lock() { lock_for_write();  }
        bool try_lock() { return try_lock_for_write(); }
        void unlock() { write_unlock(); }
        void lock_shared() { lock_for_read(); }
        bool try_lock_shared() { return try_lock_for_read(); }
        void unlock_shared() { read_unlock(); }

    private:
        bool lock_for_write() noexcept {
#ifdef RW_MUTEX_PRIORITIES
            pending_writers_.fetch_add(1);
            shut_on_destroy up{[this]() { pending_writers_.fetch_sub(1); }};
#endif
            while(true) {
#if defined(RW_MUTEX_UPGRADEABLE) || defined(RW_MUTEX_PRIORITIES)
                if(
#ifdef RW_MUTEX_UPGRADEABLE
                    pending_upgraders_.load() == 0
#endif
#if defined(RW_MUTEX_UPGRADEABLE) && defined(RW_MUTEX_PRIORITIES)
                    &&
#endif
#ifdef RW_MUTEX_PRIORITIES
                    (prefer_writers_ == 1 || pending_readers_.load() == 0)
#endif
                ) {
#endif
                    std::int64_t wanted{0};
                    if(current_.compare_exchange_weak(wanted, -1, std::memory_order_release, std::memory_order_acquire)) {
                        return true;
                    }
#if defined(RW_MUTEX_UPGRADEABLE) || defined(RW_MUTEX_PRIORITIES)
                }
#endif
#if RW_MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
                maybe_wait();
#endif
            }
            return false;
        }

        bool lock_for_read() noexcept {
#ifdef RW_MUTEX_PRIORITIES
            pending_readers_.fetch_add(1);
            shut_on_destroy up{[this]() { pending_readers_.fetch_sub(1); }};
#endif
            std::int64_t curr{current_.load()};
            while(true) {
#if defined(RW_MUTEX_UPGRADEABLE) || defined(RW_MUTEX_PRIORITIES)
                if(
#ifdef RW_MUTEX_UPGRADEABLE
                    (prefer_writers_ == 0 || pending_upgraders_.load() == 0)
#endif
#if defined(RW_MUTEX_UPGRADEABLE) && defined(RW_MUTEX_PRIORITIES)
                    &&
#endif
#ifdef RW_MUTEX_PRIORITIES
                    (prefer_writers_ == 0 || pending_writers_.load() == 0)
#endif
                ) {
#endif
                    if(curr >= 0) {
                        if(current_.compare_exchange_weak(curr, curr + 1, std::memory_order_release, std::memory_order_acquire)) {
                            return true;
                        }
                    } else {
                        curr = current_.load();
                    }
#if defined(RW_MUTEX_UPGRADEABLE) || defined(RW_MUTEX_PRIORITIES)
                }
#endif
#if RW_MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
                maybe_wait();
#endif
            }
            return false;
        }

        bool try_lock_for_write() noexcept {
            std::int64_t zero{0};
            if(
#ifdef RW_MUTEX_UPGRADEABLE
                pending_upgraders_.load() == 0 &&
#endif
#ifdef RW_MUTEX_PRIORITIES
                (prefer_writers_ == 1 || pending_readers_.load() == 0) &&
#endif
                current_.compare_exchange_strong(zero, -1)
            ) {
                return true;
            }
            return false;
        }

        bool try_lock_for_read() noexcept {
            std::int64_t zero_or_more{current_.load()};
            if(
                zero_or_more >= 0
#ifdef RW_MUTEX_UPGRADEABLE
                && (prefer_writers_ == 1 && pending_upgraders_.load() == 0)
#endif
#ifdef RW_MUTEX_PRIORITIES
                && (prefer_writers_ == 0 || pending_writers_.load() == 0)
#endif
                && current_.compare_exchange_strong(zero_or_more, zero_or_more + 1)
            ) {
                return true;
            }
            return false;
        }

        void read_unlock() {
            std::int64_t curr{current_.load()};
            if(curr < 0) {
                current_.store(0);
            } else if(curr > 0) {
                current_.fetch_sub(1);
            } else {
                throw std::runtime_error{"locking counting error"};
            }
        }

        void write_unlock() {
            if(current_.fetch_add(1) != -1) {
                throw std::runtime_error{"locking counting error"};
            }
        }

    private:
        void maybe_wait() {
#if RW_MUTEX_ATOMIC_SM_SLEEP_NANOS > 0
            std::this_thread::sleep_for(std::chrono::nanoseconds{RW_MUTEX_ATOMIC_SM_SLEEP_NANOS});
#endif
        }

    private:
        std::atomic<std::int64_t> current_{0};
#ifdef RW_MUTEX_UPGRADEABLE
        std::atomic<std::int64_t> pending_upgraders_{0};
#endif
#ifdef RW_MUTEX_PRIORITIES
        std::atomic<std::int64_t> pending_readers_{0};
        std::atomic<std::int64_t> pending_writers_{0};
        std::int64_t prefer_writers_{1};
#endif
    };

}
