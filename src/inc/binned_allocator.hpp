#pragma once

#include "commondefs.hpp"
#include "mt_synchro.hpp"

namespace teal {

    template<
        std::size_t MEMORY_SIZE = 128 * 1024 * 1024,
        std::size_t ALIGNMENT = 16,
        std::size_t MAX_ALLOC_SIZE = 1024
    >
    class binned_allocator {
        static_assert((ALIGNMENT & (ALIGNMENT - 1)) == 0, "ALIGNMENT must be a power of 2");
        static_assert((MAX_ALLOC_SIZE & (MAX_ALLOC_SIZE - 1)) == 0, "MAX_ALLOC_SIZE must be a power of 2 for this implementation");

        static constexpr size_t NUM_BINS = MAX_ALLOC_SIZE / ALIGNMENT;

        struct alloc_unit_hdr {
            alloc_unit_hdr *next;
            size_t size;
            void *user_ptr() {
                return ((uint8_t *)this) + allocation_header_size();
            }
        };

        struct alignas(64) free_list {
            std::atomic<alloc_unit_hdr *> head{nullptr};
            std::shared_mutex mtp{};
        };

        static size_t constexpr size_to_aligned(size_t n) {
            return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        static size_t constexpr allocation_header_size() {
            return ALIGNMENT < sizeof(alloc_unit_hdr) ?
                       size_to_aligned(sizeof(alloc_unit_hdr)) : ALIGNMENT;
        }

        static size_t constexpr bin_index(size_t aligned_size) {
            return (aligned_size / ALIGNMENT) - 1;
        }

    public:
        binned_allocator() {
            reset();
        }
        binned_allocator(binned_allocator const &) = delete;
        binned_allocator& operator=(binned_allocator const &) = delete;
        binned_allocator(binned_allocator &&) = delete;
        binned_allocator& operator=(binned_allocator &&) = delete;
        ~binned_allocator() = default;

        void *allocate(size_t s) {
            if(s == 0) {
                s = 1;
            }
            size_t const aligned_size{size_to_aligned(s)};
            size_t const full_size{aligned_size + allocation_header_size()};
            if(aligned_size > MAX_ALLOC_SIZE) {
                alloc_unit_hdr *full_chnk{(alloc_unit_hdr *)::malloc(full_size)};
                full_chnk->size = aligned_size;
                return full_chnk->user_ptr();
            }

            alloc_unit_hdr *res_ptr{nullptr};
            free_list &fl{free_lists_[bin_index(aligned_size)]};
            {
                std::unique_lock l1{fl.mtp};
                res_ptr = fl.head.load();
                while(res_ptr != nullptr && !fl.head.compare_exchange_strong(res_ptr, res_ptr->next)) {
                }
            }
            if(res_ptr == nullptr) {
                size_t prev_offs{offset_.fetch_add(full_size)};
                if(prev_offs + full_size > MEMORY_SIZE) {
                    offset_.fetch_sub(full_size);
                    return nullptr;
                }
                res_ptr = (alloc_unit_hdr *)(buffer_.data() + prev_offs);
                res_ptr->size = aligned_size;
            }
            allocated_.fetch_add(aligned_size);
            return res_ptr->user_ptr();
        }

        bool deallocate(void *ptr, size_t s) {
            if(ptr == nullptr) {
                return false;
            }
            alloc_unit_hdr *res_ptr{
                (alloc_unit_hdr *)(((uint8_t *)ptr) - allocation_header_size())
            };
            size_t const aligned_size{size_to_aligned(s)};
            size_t const stored_size{res_ptr->size};
            if(stored_size != aligned_size) {
                return false;
            }
            if(aligned_size > MAX_ALLOC_SIZE) {
                ::free(res_ptr);
                return true;
            }
            free_list &fl{free_lists_[bin_index(aligned_size)]};
            {
                for(
                    res_ptr->next = fl.head.load();
                    !fl.head.compare_exchange_strong(res_ptr->next, res_ptr)
                    ;
                );
            }
            allocated_.fetch_sub(stored_size);
            return true;
        }

        bool deallocate(void *ptr) {
            if(ptr == nullptr) {
                return false;
            }
            alloc_unit_hdr *res_ptr{
                (alloc_unit_hdr *)(((uint8_t *)ptr) - allocation_header_size())
            };
            size_t const stored_size{res_ptr->size};
            if(stored_size > MAX_ALLOC_SIZE) {
                ::free(res_ptr);
                return true;
            }
            free_list &fl{free_lists_[bin_index(stored_size)]};
            {
                for(
                    res_ptr->next = fl.head.load()
                    ;
                    !fl.head.compare_exchange_strong(res_ptr->next, res_ptr)
                    ;
                );
            }
            allocated_.fetch_sub(stored_size);
            return true;
        }

        void reset() {
            offset_ = 0;
            allocated_ = 0;
            for(auto &&p: free_lists_) {
                p.head = nullptr;
            }
        }

        size_t allocated() const {
            return allocated_;
        }

    private:
        alignas(ALIGNMENT) std::array<uint8_t, MEMORY_SIZE> buffer_{};
        alignas(64) std::array<free_list, NUM_BINS> free_lists_{};
        std::atomic<size_t> allocated_{0};
        std::atomic<size_t> offset_{0};
        std::mutex mtp_{};
    };

}
