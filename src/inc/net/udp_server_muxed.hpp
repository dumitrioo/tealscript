#pragma once

#include "../commondefs.hpp"
#include "../command_queue.hpp"
#include "../timespec_wrapper.hpp"
#include "../serialization.hpp"
#include "../bit_util.hpp"
#include "../terminable.hpp"
#include "../crypto/gamma.hpp"
#include "../sequence_generator.hpp"
#include "../emhash/hash_table8.hpp"
#include "net_utils.hpp"
#include "socket_wrapper.hpp"
#include "net_data_transfer.hpp"
#include "socket_poller.hpp"
#include "udp_hlp.hpp"

namespace teal::net {

    template<std::size_t NET_PACKET_PAYLOAD_SIZE_MAX = 1400>
    class udp_server_muxed: public terminable {
        union sockaddr_any;
        class udp_connection;

    public:
        udp_server_muxed(
            command_queue *cq,
            address_family af = address_family::inet4,
            long double stale_connections_removal_timeout = 10.0L
        ):
            cq_{cq},
            stale_connections_removal_timeout_{stale_connections_removal_timeout},
            sock_type_{af}
        {
        }

        udp_server_muxed(udp_server_muxed const &) = delete;

        udp_server_muxed(udp_server_muxed &&) = delete;

        udp_server_muxed &operator=(udp_server_muxed const &) = delete;

        udp_server_muxed &operator=(udp_server_muxed &&) = delete;

        ~udp_server_muxed() {
            stop();
        }

        void set_on_data_arrived(std::function<void(std::uint64_t, void const *, std::size_t)> const &on_data_from_client) {
            on_data_from_client_ = on_data_from_client;
        }

        void set_on_data_arrived(std::function<void(std::uint64_t, void const *, std::size_t)> &&on_data_from_client) {
            on_data_from_client_ = std::move(on_data_from_client);
        }

        void set_on_new_connection(std::function<void(std::uint64_t)> const &on_new_connection) {
            on_new_connection_ = on_new_connection;
        }

        void set_on_new_connection(std::function<void(std::uint64_t)> &&on_new_connection) {
            on_new_connection_ = std::move(on_new_connection);
        }

        void set_on_connection_closed(std::function<void(std::uint64_t)> const &on_connection_closed) {
            on_connection_closed_ = on_connection_closed;
        }

        void set_on_connection_closed(std::function<void(std::uint64_t)> &&on_connection_closed) {
            on_connection_closed_ = std::move(on_connection_closed);
        }

        void set_connection_local_key(std::uint64_t conn_id, bytevec const &key, bytevec const &iv) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->set_local_key(key, iv);
            }
        }

        void set_connection_remote_key(std::uint64_t conn_id, bytevec const &key, bytevec const &iv) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->set_remote_key(key, iv);
            }
        }

        void clear_connection_local_key(std::uint64_t conn_id) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->clear_local_key();
            }
        }

        void clear_connection_remote_key(std::uint64_t conn_id) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->clear_remote_key();
            }
        }

        void set_connection_encryption_enabled(std::uint64_t conn_id, bool val) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->set_encryption_enabled(val);
            }
        }

        bool connection_encryption_enabled(std::uint64_t conn_id) const {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                return conn_ptr->encryption_enabled();
            }
            return false;
        }

        template<typename T>
        void set_conn_user_data(std::uint64_t conn_id, T const &val) {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                conn_ptr->set_user_data(val);
            } else {
                throw std::runtime_error{"connection not found"};
            }
        }

        template<typename T>
        T conn_user_data(std::uint64_t conn_id) const {
            std::shared_ptr<udp_connection> conn_ptr{connection_by_id(conn_id)};
            if(conn_ptr) {
                return conn_ptr->template user_data<T>();
            }
            throw std::runtime_error{"connection not found"};
        }

        bool stop() {
            bool res{false};
            try {
                terminate();
                wait();
                remove_all_connections();
                {
                    if(sock_fd_ != -1) {
                        {
                        std::unique_lock pl{poller_mtp_};
                        poller_.close();
                        }
                        std::unique_lock sl{sock_fd_mtp_};
                        ::close(sock_fd_);
                        sock_fd_ = -1;
                    }
                }
                started_ = false;
            } catch(...) {
            }
            return res;
        }

        bool ok() const {
            std::shared_lock pl{sock_fd_mtp_};
            return sock_fd_ != -1;
        }

        void remove_all_connections() {
            std::unique_lock l{connections_mtp_};
            while(connections_by_id_.size()) {
                remove_conn_dont_send_message_unlocked(connections_by_id_.begin()->first);
            }
        }

        using in_struct = std::pair<std::array<std::uint8_t, NET_PACKET_PAYLOAD_SIZE_MAX + 256>, std::size_t>;
        void worker() {
            try {
                sockaddr_any cli_addr;
                std::memset(&cli_addr, 0, sizeof(cli_addr));

                ssize_t n{0};
                std::shared_ptr<in_struct> in_buff{std::make_shared<in_struct>()};
                std::vector<poll_event> events{};
                {
                    std::unique_lock l{poller_mtp_};
                    events = poller_.wait(1, timespec_wrapper{0.1});
                }
                if(events.size() == 1) {
                    if((events[0].events & net::POLL_EVENT_IN) == net::POLL_EVENT_IN) {
                        std::shared_lock l{sock_fd_mtp_};
                        if(sock_fd_ >= 0) {
                            socklen_t socklen{
                                static_cast<socklen_t>(sock_type_ == address_family::inet6 ?
                                    sizeof(sockaddr_in6) : sizeof(sockaddr_in))
                            };
                            n = ::recvfrom(
                                sock_fd_, in_buff->first.data(), NET_PACKET_PAYLOAD_SIZE_MAX + 256,
                                0, (struct sockaddr *)&cli_addr, &socklen
                            );
                            l.unlock();
                            if(n > 0) {
                                in_buff->second = n;
                                enqueue_job([this, cli_addr, in_buff]() { process_input(cli_addr, in_buff); });
                            } else {
                                enqueue_job([this]() { stop(); });
                            }
                        }
                    } else {
                        enqueue_job([this]() { stop(); });
                    }
                }
            } catch (...) {
                enqueue_job([this]() { stop(); });
            }
        }

        void wait() {
            while(started_.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }
        }

        bool started() const {
            return started_;
        }

        bool start(std::string const &addr, int port, int num_listen_threads) {
            bool res{false};
            std::unique_lock pl{poller_mtp_};
            std::unique_lock sl{sock_fd_mtp_};
            if(started_) {
                res = true;
            } else {
                unterminate();
                if(sock_type_ == address_family::inet4) {
                    if((sock_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
                        if(sock_fd_ != -1) {
                            if(!helpers::make_nonblocking(sock_fd_)) {
                                ::close(sock_fd_);
                                sock_fd_ = -1;
                            }
                        }
                        if(sock_fd_ != -1) {
                            try {
                                std::memset(&serv_addr_.v4_, 0, sizeof(serv_addr_.v4_));
                                serv_addr_.v4_.sin_family = AF_INET;
                                in_addr a{net::resolve(addr)};
                                serv_addr_.v4_.sin_addr.s_addr = a.s_addr;
                                serv_addr_.v4_.sin_port =
                                    bit_util::swap_on_le<decltype(serv_addr_.v4_.sin_port)>{
                                        static_cast<decltype(serv_addr_.v4_.sin_port)>(port)
                                    }.val;
                                if(::bind(sock_fd_, (const struct sockaddr *)&serv_addr_.v4_, sizeof(serv_addr_.v4_)) >= 0) {
                                    poller_.add_event(sock_fd_, net::POLL_EVENT_IN);
                                    if(num_listen_threads <= 0) {
                                        num_listen_threads = 1;
                                    }
                                    cq_->enqueue(worker_entry_);
                                    started_ = true;
                                    res = true;
                                }
                            } catch (...) {
                            }
                        }
                    }
                } else if(sock_type_ == address_family::inet6) {
                    if((sock_fd_ = ::socket(AF_INET6, SOCK_DGRAM, 0)) >= 0) {
                        if(sock_fd_ != -1) {
                            if(!helpers::make_nonblocking(sock_fd_)) {
                                ::close(sock_fd_);
                                sock_fd_ = -1;
                            }
                        }
                        if(sock_fd_ != -1) {
                            try {
                                std::memset(&serv_addr_.v6_, 0, sizeof(serv_addr_.v6_));
                                serv_addr_.v6_.sin6_family = AF_INET6;
                                in6_addr a{net::resolve6(addr)};
                                serv_addr_.v6_.sin6_addr = a;
                                serv_addr_.v6_.sin6_port =
                                    bit_util::swap_on_le<decltype(serv_addr_.v6_.sin6_port)>{
                                        static_cast<decltype(serv_addr_.v6_.sin6_port)>(port)
                                    }.val;
                                if(::bind(sock_fd_, (const struct sockaddr *)&serv_addr_.v6_, sizeof(serv_addr_.v6_)) >= 0) {
                                    poller_.add_event(sock_fd_, net::POLL_EVENT_IN);
                                    if(num_listen_threads <= 0) {
                                        num_listen_threads = 1;
                                    }
                                    cq_->enqueue(worker_entry_);
                                    started_ = true;
                                    res = true;
                                }
                            } catch (...) {
                            }
                        }
                    }
                } else {
                }
            }
            return res;
        }

        bool send(std::uint64_t conn_id, std::vector<std::uint8_t> const &data) {
            return send(conn_id, data.data(), data.size());
        }

        bool send(std::uint64_t conn_id, std::string const &data) {
            return send(conn_id, data.data(), data.size());
        }

        bool send(std::uint64_t conn_id, void const *data, std::size_t data_size) {
            bool res{false};
            if(data != nullptr && data_size && sock_fd_ >= 0) {
                try {
                    std::shared_ptr<udp_connection> cnn{connection_by_id(conn_id)};
                    if(cnn) {
                        serializer ser{};
                        if(cnn->encryption_enabled()) {
                            std::uint64_t ctr_start{cnn->crypto_ctr_for_data_size(data_size)};
                            ser << (std::uint8_t)1 << ctr_start << cnn->encrypt_data(data, data_size, ctr_start);
                        } else {
                            ser << (std::uint8_t)0;
                            ser.push_back(data, data_size);
                        }
                        cnn->set_output_message(ser.data_vec());
                        while(auto och{cnn->fetch_out_chunk()}) {
                            std::shared_lock l{sock_fd_mtp_};
                            if(sock_fd_ >= 0) {
                                socklen_t socklen{
                                    static_cast<socklen_t>(sock_type_ == address_family::inet6 ?
                                        sizeof(sockaddr_in6) : sizeof(sockaddr_in))
                                };
                                res = ::sendto(
                                          sock_fd_, och->data(), och->size(), 0,
                                          (const struct sockaddr *)cnn->addr_ptr(), socklen
                                      ) != -1;
                            }
                        }
                    }
                } catch (...) {
                    res = false;
                }
            }
            return res;
        }

        bool remove_conn(std::uint64_t conn_id) {
            bool res{false};
            std::unique_lock csl{connections_mtp_};
            auto it{connections_by_id_.find(conn_id)};
            if(it != connections_by_id_.end()) {
                std::shared_ptr<udp_connection> cnn{it->second};
                std::string h{conn_addr_str(cnn->cli_addr())};
                int p{conn_port(cnn->cli_addr())};
                connections_by_host_[h].erase(p);
                if(connections_by_host_[h].empty()) {
                    connections_by_host_.erase(h);
                }
                connections_by_id_.erase(it);
                csl.unlock();
                {
                    std::shared_lock l{sock_fd_mtp_};
                    if(sock_fd_ >= 0) {
                        socklen_t socklen{
                            static_cast<socklen_t>(sock_type_ == address_family::inet6 ?
                                                       sizeof(sockaddr_in6) : sizeof(sockaddr_in))
                        };
                        ::sendto(sock_fd_, "close", 5, MSG_DONTWAIT, cnn->addr_ptr(), socklen);
                    }
                }
                if(on_connection_closed_) {
                    on_connection_closed_(conn_id);
                }
                res = true;
            }
            return res;
        }

        std::shared_ptr<udp_connection> connection_by_id(std::uint64_t conn_id) const {
            std::shared_lock l{connections_mtp_};
            auto it{connections_by_id_.find(conn_id)};
            return it != connections_by_id_.end() ? it->second : std::shared_ptr<udp_connection>{};
        }

        bool connection_exists(std::uint64_t conn_id) const {
            std::shared_lock l{connections_mtp_};
            return connections_by_id_.find(conn_id) != connections_by_id_.end();
        }

    private:
        void recheck_conn_timeouts_and_clean(long double seconds) {
            std::unique_lock conn_lck{connections_mtp_};
            if(connections_by_id_.size() < 1000) {
                return;
            }
            std::map<std::uint64_t, std::shared_ptr<udp_connection>> connections_copy{connections_by_id_};
            for(auto &&p: connections_copy) {
                if(p.second->seconds_since_last_activity() > seconds) {
                    remove_conn(p.first);
                }
            }
        }

        bool remove_conn_dont_send_message_unlocked(std::uint64_t conn_id) {
            bool res{false};
            auto it{connections_by_id_.find(conn_id)};
            if(it != connections_by_id_.end()) {
                std::shared_ptr<udp_connection> cnn{it->second};
                std::string h{conn_addr_str(cnn->cli_addr())};
                int p{conn_port(cnn->cli_addr())};
                connections_by_host_[h].erase(p);
                if(connections_by_host_[h].empty()) {
                    connections_by_host_.erase(h);
                }
                connections_by_id_.erase(it);
                if(on_connection_closed_) {
                    on_connection_closed_(conn_id);
                }
                res = true;
            }
            return res;
        }

        bool remove_conn_dont_send_message(std::uint64_t conn_id) {
            bool res{false};
            std::unique_lock csl{connections_mtp_};
            auto it{connections_by_id_.find(conn_id)};
            if(it != connections_by_id_.end()) {
                std::shared_ptr<udp_connection> cnn{it->second};
                std::string h{conn_addr_str(cnn->cli_addr())};
                int p{conn_port(cnn->cli_addr())};
                connections_by_host_[h].erase(p);
                if(connections_by_host_[h].empty()) {
                    connections_by_host_.erase(h);
                }
                connections_by_id_.erase(it);
                csl.unlock();
                if(on_connection_closed_) {
                    on_connection_closed_(conn_id);
                }
                res = true;
            }
            return res;
        }

        bool remove_conn_dont_send_message(std::string const &h, int p) {
            bool res{false};
            std::unique_lock csl{connections_mtp_};

            std::shared_ptr<udp_connection> cnn{};

            auto hit{connections_by_host_.find(h)};
            if(hit != connections_by_host_.end()) {
                auto pit{hit->second.find(p)};
                if(pit != hit->second.end()) {
                    cnn = pit->second;
                    hit->second.erase(pit);
                }
                connections_by_host_.erase(hit);
            }
            if(cnn) {
                auto cnid{cnn->conn_id()};
                auto it{connections_by_id_.find(cnid)};
                if(it != connections_by_id_.end()) {
                    std::shared_ptr<udp_connection> cnn{it->second};
                    std::string h{conn_addr_str(cnn->cli_addr())};
                    int p{conn_port(cnn->cli_addr())};
                    connections_by_host_[h].erase(p);
                    if(connections_by_host_[h].empty()) {
                        connections_by_host_.erase(h);
                    }
                    connections_by_id_.erase(it);
                    csl.unlock();
                    if(on_connection_closed_) {
                        on_connection_closed_(cnid);
                    }
                    res = true;
                }
            }
            return res;
        }

        std::shared_ptr<udp_connection> process_new_connection(sockaddr_any cliaddr) {
            std::string h{conn_addr_str(cliaddr)};
            int p{conn_port(cliaddr)};
            std::shared_ptr<udp_connection> conn{get_conn_by_host(h, p)};
            if(!conn) {
                std::uint64_t conn_id{};
                conn_id = conn_id_gen_();
                conn = std::make_shared<udp_connection>(cliaddr, conn_id);
                {
                    std::unique_lock l{connections_mtp_};
                    conn->update_last_activity_time();
                    connections_by_id_[conn_id] = conn;
                    connections_by_host_[h][p] = conn;
                }
                if(on_new_connection_) {
                    on_new_connection_(conn_id);
                }
            }
            if(stale_connections_removal_timeout_ > 0) {
                enqueue_job([this]() { recheck_conn_timeouts_and_clean(stale_connections_removal_timeout_); });
            }
            return conn;
        }

        void process_input(sockaddr_any cliaddr, std::shared_ptr<in_struct> in_buff) {
            std::size_t insize{in_buff->second};
            std::shared_ptr<udp_connection> conn_ptr{process_new_connection(cliaddr)};
            if(conn_ptr) {
                if(insize == 13 && std::string{in_buff->first.begin(), in_buff->first.begin() + 5} == "close") {
                    if(conn_ptr) {
                        remove_conn_dont_send_message(conn_ptr->conn_id());
                    }
                } else if(insize > 0 && on_data_from_client_ != nullptr) {
                    if(conn_ptr) {
                        std::optional<bytevec> msg{conn_ptr->set_incoming_data(in_buff->first.data() + 8, insize - 8)};
                        if(msg) {
                            serial_reader const ser{msg->data(), msg->size()};
                            serial_reader::const_iterator iter{ser.cbegin()};
                            if(iter->as_unumber() == 0) {
                                ++iter;
                                on_data_from_client_(conn_ptr->conn_id(), iter->data(), iter->size());
                            } else {
                                ++iter;
                                std::uint64_t ctr_start{iter->as_unumber()};
                                ++iter;
                                std::vector<std::uint8_t> d{conn_ptr->decrypt_data(iter->data(), iter->size(), ctr_start)};
                                on_data_from_client_(conn_ptr->conn_id(), d.data(), d.size());
                            }
                        }
                    }
                }
            }
        }

    private:
        bool enqueue_job(std::function<void()> &&fn) {
            if(!cq_) {
                return false;
            }
            cq_->enqueue(std::move(fn));
            return true;
        }

        bool enqueue_job(std::function<void()> const &fn) {
            if(!cq_) {
                return false;
            }
            cq_->enqueue(fn);
            return true;
        }

    private:
        union sockaddr_any {
            struct sockaddr_in v4_;
            struct sockaddr_in6 v6_;
        };

        class udp_connection {
        public:
            udp_connection(
                sockaddr_any const &cliaddr,
                std::uint64_t conn_id
            ):
                cli_addr_{cliaddr},
                conn_id_{conn_id}
            {
            }
            udp_connection(udp_connection const &) = delete;
            udp_connection(udp_connection &&) = delete;
            udp_connection &operator=(udp_connection const &) = delete;
            udp_connection &operator=(udp_connection &&) = delete;
            ~udp_connection() = default;

            void update_last_activity_time() {
                std::unique_lock l{last_activity_time_mtp_};
                last_activity_time_ = steady_time_sec();
            }

            long double seconds_since_last_activity() const {
                std::shared_lock l{last_activity_time_mtp_};
                return steady_time_sec() - last_activity_time_;
            }

            std::uint64_t conn_id() const {
                return conn_id_;
            }

            struct sockaddr const *addr_ptr() const {
                return (struct sockaddr const *)&cli_addr_;
            }

            sockaddr_any const &cli_addr() const {
                return cli_addr_;
            }

            sockaddr_any &cli_addr() {
                return cli_addr_;
            }

            void set_ack() {
                ack_ = true;
            }

            bool ack() const {
                return ack_;
            }

            std::optional<bytevec> set_incoming_data(void const *d, std::uint64_t ds) {
                update_last_activity_time();
                std::unique_lock l{demuxer_mtp_};
                remove_stale_inputs_unlocked(120);
                return demuxer_.add_data(d, ds);
            }

            void set_output_message(std::vector<std::uint8_t> const &msg) {
                std::unique_lock l{muxer_mtp_};
                return muxer_.add_message(msg);
            }

            std::optional<bytevec> fetch_out_chunk() {
                std::unique_lock l{muxer_mtp_};
                return muxer_.fetch_out_chunk();
            }

            std::vector<std::uint8_t> encrypt_data(void const *p, std::size_t ps, std::uint64_t ctr_start) {
                if(!p || !ps) { return {}; }
                std::unique_lock lck{encrypt_mtp_};
                crypt::gamma out_gamma{out_gamma_};
                lck.unlock();
                std::vector<std::uint8_t> e{(std::uint8_t const *)p, (std::uint8_t const *)p + ps};
                std::uint64_t crypto_ctr{ctr_start};
                for(std::size_t i{0}; i < e.size(); ++i) {
                    e[i] = e[i] ^ out_gamma[crypto_ctr++];
                }
                return e;
            }

            std::vector<std::uint8_t> decrypt_data(void const *e, std::size_t es, std::uint64_t ctr_start) {
                if(!e || !es) { return {}; }
                std::vector<std::uint8_t> res{(std::uint8_t const *)e, (std::uint8_t const *)e + es};
                std::uint64_t crypto_ctr{ctr_start};
                std::unique_lock lck{decrypt_mtp_};
                crypt::gamma in_gamma{in_gamma_};
                lck.unlock();
                for(std::size_t i{0}; i < res.size(); ++i) {
                    res[i] = res[i] ^ in_gamma[crypto_ctr++];
                }
                return res;
            }

            void set_local_key(bytevec const &key, bytevec const &iv) {
                crypto_ctr_ = 0;
                out_gamma_.init(key.data(), key.size(), iv.data(), iv.size());
            }

            void set_remote_key(bytevec const &key, bytevec const &iv) {
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

            std::uint64_t crypto_ctr_for_data_size(std::uint64_t ds) {
                return crypto_ctr_.fetch_add(ds);
            }

            template<typename T>
            void set_user_data(T const &val) {
                user_data_ = val;
            }

            template<typename T>
            T const &user_data() const {
                return std::any_cast<T const &>(user_data_);
            }

        private:
            std::any user_data_{};

            std::mutex muxer_mtp_{};
            net::packets_muxer<std::uint16_t> muxer_{NET_PACKET_PAYLOAD_SIZE_MAX};
            std::mutex demuxer_mtp_{};
            net::packets_demuxer demuxer_{};
            long double last_time_demuxer_check_{0};

            void remove_stale_inputs_unlocked(long double seconds_old) {
                if(steady_time_sec() > last_time_demuxer_check_ + seconds_old) {
                    demuxer_.remove_queued_items_older_than_seconds(seconds_old);
                    last_time_demuxer_check_ = steady_time_sec();
                }
            }

        private:
            sockaddr_any cli_addr_;
            std::uint64_t conn_id_{0};

            mutable std::shared_mutex last_activity_time_mtp_{};
            long double last_activity_time_{steady_time_sec()};

            bool ack_{false};

            std::mutex encrypt_mtp_{};
            std::mutex decrypt_mtp_{};
            std::atomic<std::uint64_t> crypto_ctr_{0};
            crypt::gamma out_gamma_{};
            crypt::gamma in_gamma_{};
            std::atomic<bool> encryption_on_{false};
        };

    private:
        std::string conn_addr_str(sockaddr_any const &cliaddr) const {
            return (sock_type_ == address_family::inet6) ? ntop(cliaddr.v6_.sin6_addr) : ntop(cliaddr.v4_.sin_addr);
        }

        int conn_port(sockaddr_any const &cliaddr) const {
            return (sock_type_ == address_family::inet6) ? cliaddr.v6_.sin6_port : cliaddr.v4_.sin_port;
        }

        command_queue *cq_{nullptr};
        std::function<void()> worker_entry_{
            [this]() {
                if(!termination()) {
                    worker();
                    cq_->enqueue(worker_entry_);
                } else {
                    started_ = false;
                }
            }
        };

        long double stale_connections_removal_timeout_{10.0L};

        sockaddr_any serv_addr_;
        atomic_sequence_generator<std::uint64_t> conn_id_gen_{1};
        mutable std::shared_mutex connections_mtp_{};
        mutable std::map<std::uint64_t, std::shared_ptr<udp_connection>> connections_by_id_{};
        mutable std::map<std::string, std::map<int, std::shared_ptr<udp_connection>>> connections_by_host_{};
        std::shared_ptr<udp_connection> get_conn_by_host(std::string const &h, int p) const {
            std::shared_ptr<udp_connection> res{};
            std::shared_lock l{connections_mtp_};
            auto hit{connections_by_host_.find(h)};
            if(hit != connections_by_host_.end()) {
                auto pit{hit->second.find(p)};
                if(pit != hit->second.end()) {
                    res = pit->second;
                }
            }
            return res;
        }

        std::function<void(std::uint64_t)> on_new_connection_{nullptr};
        std::function<void(std::uint64_t, void const *, std::size_t)> on_data_from_client_{nullptr};
        std::function<void(std::uint64_t)> on_connection_closed_{nullptr};
        mutable std::shared_mutex sock_fd_mtp_{};
        int sock_fd_{-1};
        mutable std::shared_mutex poller_mtp_{};
        socket_poller poller_{};
        address_family sock_type_{address_family::unspecified};
        std::atomic<bool> started_{false};
    };

}
