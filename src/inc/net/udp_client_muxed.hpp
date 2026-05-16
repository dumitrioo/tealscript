#pragma once

#include "../commondefs.hpp"
#include "../command_queue.hpp"
#include "../timespec_wrapper.hpp"
#include "../serialization.hpp"
#include "../terminable.hpp"
#include "../crypto/gamma.hpp"
#include "../sequence_generator.hpp"
#include "socket_poller.hpp"
#include "net_utils.hpp"
#include "socket_wrapper.hpp"
#include "net_data_transfer.hpp"
#include "udp_hlp.hpp"

namespace teal::net {

    template<std::size_t NET_PACKET_PAYLOAD_SIZE_MAX = 1400>
    class udp_client_muxed: public terminable {
    public:
        udp_client_muxed(
            command_queue *cq,
            long double recv_timeout = 0.1L,
            address_family af = address_family::inet4
        ):
            cq_{cq},
            recv_timeout_{recv_timeout},
            sock_type_{af}
        {
        }

        udp_client_muxed(udp_client_muxed const &) = delete;
        udp_client_muxed(udp_client_muxed &&) = delete;
        udp_client_muxed &operator=(udp_client_muxed const &) = delete;
        udp_client_muxed &operator=(udp_client_muxed &&) = delete;
        ~udp_client_muxed() { try { close(); } catch (...) {} }

        void set_local_key(teal::bytevec const &key, teal::bytevec const &iv) {
            crypto_ctr_ = 0;
            out_gamma_.init(key.data(), key.size(), iv.data(), iv.size());
        }

        void set_remote_key(teal::bytevec const &key, teal::bytevec const &iv) {
            in_gamma_.init(key.data(), key.size(), iv.data(), iv.size());
        }

        void clear_local_key() {
            out_gamma_.clear();
        }

        void clear_remote_key() {
            in_gamma_.clear();
        }

        void set_encryption_enabled(bool val) {
            encryption_on_ = val;
        }

        bool encryption_enabled() const {
            return encryption_on_;
        }

        void set_on_data_arrived(std::function<void(bytevec const &)> fun) {
            std::unique_lock cl{callback_mtp_};
            on_data_arrived_ = fun;
        }

        bool connect(std::string const &addr, std::uint16_t port) {
            bool result{false};
            std::unique_lock l{sock_fd_mtp_};
            if(sock_fd_ != -1) {
                result = true;
            } else {
                std::string const conn_data{"con"};
                socklen_t socklen{0};
                if(sock_type_ == address_family::inet4) {
                    if((sock_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
                        if(sock_fd_ != -1) {
                            try {
                                if(helpers::set_rcv_timeout(sock_fd_, 3)) {
                                    in_addr dst_ip{teal::net::resolve(addr)};
                                    std::memset(&serv_addr_.v4_, 0, sizeof(serv_addr_.v4_));
                                    serv_addr_.v4_.sin_family = AF_INET;
                                    serv_addr_.v4_.sin_port = ::htons(port);
                                    serv_addr_.v4_.sin_addr.s_addr = dst_ip.s_addr;
                                    socklen = static_cast<socklen_t>(
                                        sock_type_ == address_family::inet6 ?
                                            sizeof(sockaddr_in6) : sizeof(sockaddr_in)
                                    );
                                }
                            } catch (...) {
                            }
                        }
                    }
                } else if(sock_type_ == address_family::inet6) {
                    if((sock_fd_ = ::socket(AF_INET6, SOCK_DGRAM, 0)) != -1) {
                        if(sock_fd_ != -1) {
                            try {
                                if(helpers::set_rcv_timeout(sock_fd_, 3)) {
                                    in6_addr dst_ip{teal::net::resolve6(addr)};
                                    std::memset(&serv_addr_.v6_, 0, sizeof(serv_addr_.v6_));
                                    serv_addr_.v6_.sin6_family = AF_INET6;
                                    serv_addr_.v6_.sin6_port = ::htons(port);
                                    serv_addr_.v6_.sin6_addr = dst_ip;
                                    socklen = static_cast<socklen_t>(
                                        sock_type_ == address_family::inet6 ?
                                            sizeof(sockaddr_in6) : sizeof(sockaddr_in)
                                    );
                                }
                            } catch (...) {
                            }
                        }
                    }
                }
                if(::sendto(sock_fd_, conn_data.data(), conn_data.size(), 0, serv_addr(), socklen) != -1) {
                    std::array<std::uint8_t, NET_PACKET_PAYLOAD_SIZE_MAX + 256> buffer{};
                    ssize_t n = ::recvfrom(sock_fd_, buffer.data(), buffer.size(), 0, serv_addr(), &socklen);
                    helpers::set_rcv_timeout(sock_fd_, recv_timeout_);
                    if(is_data_arrived_set() && !helpers::make_nonblocking(sock_fd_)) {
                        ::close(sock_fd_);
                        sock_fd_ = -1;
                    }
                    if(sock_fd_ != -1 && n > 0) {
                        teal::serial_reader sr{buffer.data(), (std::size_t)n};
                        teal::serial_reader::const_iterator it{sr.cbegin()};
                        if(it->as_string() == "ok") {
                            ++it;
                            conn_id_ = bit_util::hnswap<std::uint64_t>{it->as<std::uint64_t>()}.val;
                            if(
                                poller_.create() &&
                                poller_.add_event(sock_fd_, net::POLL_EVENT_IN)
                            ) {
                                unterminate();
                                if(is_data_arrived_set()) {
                                    worker_entry_procesing_ = true;
                                    cq_->enqueue(worker_entry_);
                                }
                                result = true;
                            }
                        }
                    }
                }
            }
            if(!result) {
                sock_fd_ = -1;
                sock_type_ = address_family::unspecified;
                conn_id_ = 0;
            }
            return result;
        }

        bool connected() const {
            return sock_fd_ != -1 && !conn_broken_.load(std::memory_order_acquire);
        }

        bool ok() const {
            std::shared_lock l{sock_fd_mtp_};
            return sock_fd_ != -1 && (sock_type_ == address_family::inet4 || sock_type_ == address_family::inet6);
        }

        void close() {
            terminate();
            while(worker_entry_procesing_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }
            poller_.close();
            std::vector<std::uint8_t> close_data{'c', 'l', 'o', 's', 'e', 0, 0, 0, 0, 0, 0, 0, 0};
            std::unique_lock l{sock_fd_mtp_};
            if(sock_fd_ >= 0) {
                std::memcpy(&close_data[5], &conn_id_, 8);
                ::sendto(sock_fd_, close_data.data(), close_data.size(), MSG_DONTWAIT, serv_addr(), sizeof(struct sockaddr_in));
                ::close(sock_fd_);
                sock_fd_ = -1;
                sock_type_ = address_family::unspecified;
            }
        }

        bool send(std::vector<std::uint8_t> const &d) {
            teal::serializer ser{};
            if(encryption_on_) {
                std::uint64_t ctr_start{crypto_ctr_.fetch_add(d.size())};
                ser << (std::uint8_t)1 << ctr_start << encrypt_data(d, ctr_start);
            } else {
                ser << (std::uint8_t)0 << d;
            }
            {
                std::unique_lock l{muxer_mtp_};
                muxer_.add_message(ser.data_vec());
            }
            teal::bytevec ch{};
            std::array<std::uint8_t, NET_PACKET_PAYLOAD_SIZE_MAX + 256> buff{};
            while(true) {
                {
                    std::unique_lock l{muxer_mtp_};
                    auto och{muxer_.fetch_out_chunk()};
                    if(!och) {
                        break;
                    }
                    ch = std::move(*och);
                }
                std::memcpy(&buff[0], &conn_id_, 8);
                std::memcpy(&buff[8], ch.data(), ch.size());
                if(::sendto(sock_fd_, buff.data(), ch.size() + 8, MSG_DONTWAIT, serv_addr(), sizeof(struct sockaddr_in)) == -1) {
                    ::close(sock_fd_);
                    sock_fd_ = -1;
                    sock_type_ = address_family::unspecified;
                    return false;
                }
            }
            return true;
        }

        bool send(std::string const &data) {
            return send(std::vector<std::uint8_t>{data.begin(), data.end()});
        }

        std::optional<std::vector<std::uint8_t>> receive() {
            std::optional<std::vector<std::uint8_t>> result{};
            std::shared_lock l{sock_fd_mtp_};
            if(sock_fd_ >= 0) {
                std::array<std::uint8_t, NET_PACKET_PAYLOAD_SIZE_MAX + 256> buffer{};
                socklen_t socklen{
                    static_cast<socklen_t>(
                        sock_type_ == address_family::inet6 ?
                            sizeof(sockaddr_in6) : sizeof(sockaddr_in)
                        )
                };
                ssize_t n = ::recvfrom(sock_fd_, buffer.data(), buffer.size(), 0, serv_addr(), &socklen);
                l.unlock();
                if(n == 5 && std::memcmp("close", buffer.data(), 5) == 0) {
                    ::close(sock_fd_);
                    sock_fd_ = -1;
                    sock_type_ = address_family::unspecified;
                } else if(n > 0) {
                    std::unique_lock l{demuxer_mtp_};
                    if(auto demuxed{demuxer_.add_data(buffer.data(), (teal::serial_reader::size_type)n)}) {
                        teal::serial_reader const ser{demuxed->data(), (teal::serial_reader::size_type)demuxed->size()};
                        teal::serial_reader::const_iterator iter{ser.cbegin()};
                        if(iter->as_unumber() == 0) {
                            ++iter;
                            result = iter->as_bytevec();
                        } else {
                            ++iter;
                            std::uint64_t ctr_start{iter->as_unumber()};
                            ++iter;
                            result = decrypt_data(iter->data(), iter->size(), ctr_start);
                        }
                        remove_stale_inputs_unlocked(180);
                    }
                }
            }
            return result;
        }

    private:
        bool is_data_arrived_set() const {
            std::shared_lock cl{callback_mtp_};
            return on_data_arrived_ != nullptr;
        }

        void remove_stale_inputs_unlocked(long double seconds_old) {
            if(steady_time_sec() > last_time_demuxer_check_ + seconds_old) {
                demuxer_.remove_queued_items_older_than_seconds(seconds_old);
                last_time_demuxer_check_ = steady_time_sec();
            }
        }

        void notify_on_data_arrived(bytevec const &bv) {
            std::shared_lock cl{callback_mtp_};
            if(on_data_arrived_) on_data_arrived_(bv);
        }

        struct sockaddr const *serv_addr() const {
            return reinterpret_cast<struct sockaddr const *>(&serv_addr_);
        }

        struct sockaddr *serv_addr() {
            return reinterpret_cast<struct sockaddr *>(&serv_addr_);
        }

        std::vector<std::uint8_t> encrypt_data(std::vector<std::uint8_t> const &p, std::uint64_t ctr_start) {
            std::unique_lock lck{encrypt_mtp_};
            teal::crypt::gamma out_gamma{out_gamma_};
            lck.unlock();
            std::vector<std::uint8_t> e{p};
            std::uint64_t crypto_ctr{ctr_start};
            for(std::size_t i{0}; i < e.size(); ++i) {
                e[i] = e[i] ^ out_gamma[crypto_ctr++];
            }
            return e;
        }

        std::vector<std::uint8_t> decrypt_data(void const *e, std::size_t es, std::uint64_t ctr_start) {
            if(!e || !es) {
                return {};
            }
            std::vector<std::uint8_t> res{(std::uint8_t const *)e, (std::uint8_t const *)e + es};
            std::uint64_t crypto_ctr{ctr_start};
            std::unique_lock lck{decrypt_mtp_};
            teal::crypt::gamma in_gamma{in_gamma_};
            lck.unlock();
            for(std::size_t i{0}; i < res.size(); ++i) {
                res[i] = res[i] ^ in_gamma[crypto_ctr++];
            }
            return res;
        }

    private:
        std::atomic<bool> worker_entry_procesing_{false};
        std::function<void()> worker_entry_{
            [this]() {
                if(!termination() && !conn_broken_.load(std::memory_order_acquire)) {
                    std::vector<net::poll_event> evs{poller_.wait(1, timespec_wrapper{5.0})};
                    if(!evs.empty()) {
                        if((evs[0].events & net::POLL_EVENT_IN) == net::POLL_EVENT_IN) {
                            std::shared_lock l{sock_fd_mtp_};
                            if(sock_fd_ >= 0) {
                                std::array<std::uint8_t, NET_PACKET_PAYLOAD_SIZE_MAX + 256> buffer{};
                                socklen_t socklen{
                                    static_cast<socklen_t>(
                                        sock_type_ == address_family::inet6 ?
                                            sizeof(sockaddr_in6) : sizeof(sockaddr_in)
                                    )
                                };
                                ssize_t n = ::recvfrom(sock_fd_, buffer.data(), buffer.size(), 0, serv_addr(), &socklen);
                                l.unlock();
                                if(n == 5 && std::memcmp("close", buffer.data(), 5) == 0) {
                                    ::close(sock_fd_);
                                    sock_fd_ = -1;
                                    sock_type_ = address_family::unspecified;
                                } else if(n > 0) {
                                    std::unique_lock l{demuxer_mtp_};
                                    if(auto demuxed{demuxer_.add_data(buffer.data(), (teal::serial_reader::size_type)n)}) {
                                        teal::serial_reader const ser{
                                            demuxed->data(), (teal::serial_reader::size_type)demuxed->size()
                                        };
                                        teal::serial_reader::const_iterator iter{ser.cbegin()};
                                        if(iter->as_unumber() == 0) {
                                            ++iter;
                                            cq_->enqueue([this, bv = iter->as_bytevec()]() {
                                                notify_on_data_arrived(bv);
                                            });
                                        } else {
                                            ++iter;
                                            std::uint64_t ctr_start{iter->as_unumber()};
                                            ++iter;
                                            cq_->enqueue([this, bv = decrypt_data(iter->data(), iter->size(), ctr_start)]() {
                                                notify_on_data_arrived(bv);
                                            });
                                        }
                                        remove_stale_inputs_unlocked(180);
                                    }
                                }
                            }
                        } else {
                            conn_broken_ = true;
                        }
                    }
                    cq_->enqueue(worker_entry_);
                } else {
                    worker_entry_procesing_ = false;
                }
            }
        };

    private:
        command_queue *cq_{nullptr};
        std::mutex muxer_mtp_{};
        teal::net::packets_muxer<std::uint16_t> muxer_{NET_PACKET_PAYLOAD_SIZE_MAX};
        std::mutex demuxer_mtp_{};
        teal::net::packets_demuxer demuxer_{};
        long double last_time_demuxer_check_{0};
        net::socket_poller poller_{};
        std::uint64_t conn_id_{0};
        long double recv_timeout_{3.0L};
        union {
            struct sockaddr_in v4_;
            struct sockaddr_in6 v6_;
        } serv_addr_;
        mutable std::shared_mutex sock_fd_mtp_{};
        int sock_fd_{-1};
        address_family sock_type_{address_family::unspecified};
        mutable std::shared_mutex callback_mtp_{};
        std::function<void(bytevec const &)> on_data_arrived_{nullptr};
        std::mutex encrypt_mtp_{};
        std::mutex decrypt_mtp_{};
        std::atomic<std::uint64_t> crypto_ctr_{0};
        teal::crypt::gamma out_gamma_{};
        teal::crypt::gamma in_gamma_{};
        std::atomic<bool> encryption_on_{false};
        std::atomic<bool> conn_broken_{false};
    };

}
