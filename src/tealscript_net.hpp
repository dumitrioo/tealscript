#pragma once

#include "inc/commondefs.hpp"
#include "inc/str_util.hpp"
#include "inc/terminable.hpp"
#include "inc/timespec_wrapper.hpp"
#include "inc/emhash/hash_table8.hpp"
#include "inc/hash/hash.hpp"
#include "inc/net/net_utils.hpp"
#include "inc/net/tcp_client.hpp"
#include "inc/net/tcp_server.hpp"
#include "inc/net/udp_client_muxed.hpp"
#include "inc/net/udp_server_muxed.hpp"
#include "inc/net/net_data_transfer.hpp"
#include "inc/net/url.hpp"

#include "tealscript_util.hpp"
#include "tealscript_token.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_statement.hpp"
#include "tealscript_exec_ctx.hpp"

namespace teal {

    class pp_server_tcp {
        struct multiplexing;

    public:
        pp_server_tcp() = default;
        pp_server_tcp(pp_server_tcp const &) = delete;
        pp_server_tcp &operator=(pp_server_tcp const &) = delete;
        pp_server_tcp(pp_server_tcp &&) = delete;
        pp_server_tcp &operator=(pp_server_tcp &&) = delete;
        ~pp_server_tcp() {
            stop();
        }

        void set_on_data_arrived(std::function<void(conn_id_t, bytevec const &)> &&on_data_arrived) {
            std::unique_lock l{on_data_arrived_mtp_};
            on_data_arrived_ = std::move(on_data_arrived);
        }

        bool started() const {
            return tns_.started();
        }

        void start(const std::string &address, std::uint16_t port, std::size_t num_work_threads, bool no_delay = false) {
            if(tns_.started()) { return; }
#if 0
            tns_.set_on_new_connection([](conn_id_t conn_id) {
            });
#endif
            tns_.set_on_data_arrived([this](conn_id_t conn_id) {
                auto d{tns_.get_conn_data(conn_id)};
                if(d) {
                    std::shared_ptr<multiplexing> mxr{get_or_create_muxerset(conn_id)};
                    std::unique_lock l{mxr->demux_mtp_};
                    mxr->demux_bottom_layer_.set_network_input(*d);
                    while(auto ofid{mxr->demux_bottom_layer_.fetch_in_data()}) {
                        if(std::optional<bytevec> message{mxr->demuxer_.add_data(*ofid)}) {
                            notify_on_data_arrived(conn_id, *message);
                        }
                    }
                }
            });
            tns_.set_on_connection_closed([this](conn_id_t conn_id) {
                remove_muxerset_by_conn_id(conn_id);
            });
            tns_.start(address, port, num_work_threads, no_delay);
        }

        void stop() {
            tns_.stop();
            tns_.set_on_new_connection(nullptr);
            tns_.set_on_data_arrived(nullptr);
            tns_.set_on_connection_closed(nullptr);
            {
                std::unique_lock l{muxers_mtp_};
                muxers_.clear();
            }
        }

        int send(conn_id_t conn_id, void const *data, std::size_t dsize) {
            std::shared_ptr<multiplexing> mxr{get_or_create_muxerset(conn_id)};
            std::unique_lock l{mxr->mux_mtp_};
            mxr->muxer_.add_message(data, dsize);
            int res{};
            while(auto chnk{mxr->muxer_.fetch_out_chunk()}) {
                bytevec pckt{mxr->mux_bottom_layer_.output_data_to_network_frame(*chnk)};
                res += tns_.send(conn_id, pckt);
            }
            return res;
        }

        int send(conn_id_t conn_id, std::string const &data) {
            return send(conn_id, data.data(), data.size());
        }

        int send(conn_id_t conn_id, bytevec const &data) {
            return send(conn_id, data.data(), data.size());
        }

    private:
        void notify_on_data_arrived(conn_id_t c, bytevec const &d) {
            std::shared_lock l{on_data_arrived_mtp_};
            if(on_data_arrived_) {
                on_data_arrived_(c, d);
            }
        }

        std::shared_ptr<multiplexing> get_or_create_muxerset(conn_id_t conn_id) {
            std::shared_lock l{muxers_mtp_};
            std::shared_ptr<multiplexing> res{};
            auto it{muxers_.find(conn_id)};
            if(it != muxers_.end()) {
                res = it->second;
            } else {
                l.unlock();
                std::unique_lock l1{muxers_mtp_};
                res = muxers_[conn_id];
                if(!res) {
                    res = std::make_shared<multiplexing>();
                    muxers_[conn_id] = res;
                }
            }
            return res;
        }

        void remove_muxerset_by_conn_id(conn_id_t conn_id) {
            std::unique_lock l{muxers_mtp_};
            muxers_.erase(conn_id);
        }

    private:
        teal_net_server tns_{};

        mutable std::shared_mutex on_data_arrived_mtp_{};
        std::function<void(conn_id_t, bytevec const &)> on_data_arrived_{nullptr};

        struct multiplexing {
            mutable std::shared_mutex mux_mtp_{};
            net::sized_packets_exchanger mux_bottom_layer_{};
            net::packets_muxer<std::uint32_t> muxer_{1400};
            mutable std::shared_mutex demux_mtp_{};
            net::sized_packets_exchanger demux_bottom_layer_{};
            net::packets_demuxer demuxer_{};
        };
        mutable std::shared_mutex muxers_mtp_{};
        emhash_8::HashMap<conn_id_t, std::shared_ptr<multiplexing>> muxers_{};
    };

    class pp_client_tcp {
    public:
        pp_client_tcp() = default;
        pp_client_tcp(pp_client_tcp const &) = delete;
        pp_client_tcp &operator=(pp_client_tcp const &) = delete;
        pp_client_tcp(pp_client_tcp &&) = delete;
        pp_client_tcp &operator=(pp_client_tcp &&) = delete;
        ~pp_client_tcp() {
            try {
                stop();
            } catch (...) {
            }
        }

        void set_on_data_arrived(std::function<void(pp_client_tcp *)> const &fun) {
            std::unique_lock cl{callback_mtp_};
            on_data_arrived_ = fun;
        }

        void set_on_data_arrived(std::function<void(pp_client_tcp *)> &&fun) {
            std::unique_lock cl{callback_mtp_};
            on_data_arrived_ = std::move(fun);
        }

        void start(std::string host, std::uint16_t port, bool no_delay = false) {
            connect_ = true;
            host_ = host;
            port_ = port;
            tnc_.set_on_data_arrived([&]() {
                notify_on_data_arrived();
            });
            tnc_.connect(host, port, no_delay);
        }

        void stop() {
            tnc_.disconnect();
            tnc_.set_on_data_arrived(nullptr);
        }

        bool connected() const {
            return tnc_.connected();
        }

        int send(const void *data, size_t data_size) {
            int res{};
            std::unique_lock l{muxing_mtp_};
            muxer_.add_message(data, data_size);
            while(auto ochnk{muxer_.fetch_out_chunk()}) {
                bytevec pkt{mux_bottom_layer_.output_data_to_network_frame(*ochnk)};
                res += tnc_.send(pkt);
            }
            return res;
        }

        int send(bytevec const &data) {
            return send(data.data(), data.size());
        }

        int send(std::string const &data) {
            return send(data.data(), data.size());
        }

        std::optional<bytevec> receive(long double timeout = 0) {
            std::optional<bytevec> in{tnc_.receive(timeout)};
            if(in) {
                std::unique_lock l{muxing_mtp_};
                demux_bottom_layer_.set_network_input(*in);
                if(auto ofid{demux_bottom_layer_.fetch_in_data()}) {
                    return demuxer_.add_data(*ofid);
                }
            }
            return {};
        }

        std::string const &host() const {
            return host_;
        }

        std::uint16_t port() const {
            return port_;
        }

    private:
        void notify_on_data_arrived() {
            std::shared_lock cl{callback_mtp_};
            if(on_data_arrived_) on_data_arrived_(this);
        }

    private:
        teal_net_client tnc_{};

        std::shared_mutex callback_mtp_{};
        std::function<void(pp_client_tcp *)> on_data_arrived_{nullptr};

        std::shared_mutex muxing_mtp_{};
        net::sized_packets_exchanger mux_bottom_layer_{};
        net::packets_muxer<std::uint32_t> muxer_{1400};
        net::sized_packets_exchanger demux_bottom_layer_{};
        net::packets_demuxer demuxer_{};

        std::string host_{};
        std::uint16_t port_{0};
        bool connect_{false};
    };


    class pp_server_udp {
        struct multiplexing;

    public:
        pp_server_udp(
            command_queue *cq,
            net::address_family af = net::address_family::inet4,
            long double stale_connections_removal_timeout = 0.0L
        ):
            cq_{cq},
            usm_{
                std::make_unique<net::udp_server_muxed<1400>>(
                    cq_,
                    af,
                    stale_connections_removal_timeout = 0
                )
            }
        {
        }
        pp_server_udp(pp_server_udp const &) = delete;
        pp_server_udp&operator=(pp_server_udp const &) = delete;
        pp_server_udp(pp_server_udp &&) = delete;
        pp_server_udp&operator=(pp_server_udp &&) = delete;
        ~pp_server_udp() {
            stop();
        }

        void set_on_data_arrived(std::function<void(conn_id_t, bytevec const &)> const &on_data_arrived) {
            std::unique_lock l{on_data_arrived_mtp_};
            on_data_arrived_ = on_data_arrived;
        }

        void set_on_data_arrived(std::function<void(conn_id_t, bytevec const &)> &&on_data_arrived) {
            std::unique_lock l{on_data_arrived_mtp_};
            on_data_arrived_ = std::move(on_data_arrived);
        }

        bool started() const {
            return usm_->started();
        }

        void start(const std::string &address, std::uint16_t port, std::size_t num_listen_threads) {
            if(usm_->started()) { return; }
            usm_->set_on_data_arrived([this](conn_id_t conn_id, void const *d, std::size_t s) {
                if(d) {
                    notify_on_data_arrived(conn_id, bytevec{static_cast<uint8_t const *>(d), static_cast<uint8_t const *>(d) + s});
                }
            });
            usm_->start(address, port, num_listen_threads);
        }

        void stop() {
            usm_->stop();
            usm_->set_on_new_connection(nullptr);
            usm_->set_on_data_arrived(nullptr);
            usm_->set_on_connection_closed(nullptr);
        }

        int send(conn_id_t conn_id, void const *data, std::size_t dsize) {
            return usm_->send(conn_id, data, dsize) ? dsize : 0;
        }

        int send(conn_id_t conn_id, std::string const &data) {
            return send(conn_id, data.data(), data.size());
        }

        int send(conn_id_t conn_id, bytevec const &data) {
            return send(conn_id, data.data(), data.size());
        }

    private:
        void notify_on_data_arrived(conn_id_t c, bytevec const &d) {
            std::shared_lock l{on_data_arrived_mtp_};
            if(on_data_arrived_) {
                on_data_arrived_(c, d);
            }
        }

    private:
        command_queue *cq_{nullptr};
        std::unique_ptr<teal::net::udp_server_muxed<1400>> usm_{};
        mutable std::shared_mutex on_data_arrived_mtp_{};
        std::function<void(conn_id_t, bytevec const &)> on_data_arrived_{nullptr};
    };

    class pp_client_udp {
    public:
        pp_client_udp(command_queue *cq): cq_{cq} {
        }
        pp_client_udp(pp_client_udp const &) = delete;
        pp_client_udp&operator=(pp_client_udp const &) = delete;
        pp_client_udp(pp_client_udp &&) = delete;
        pp_client_udp&operator=(pp_client_udp &&) = delete;
        ~pp_client_udp() {
            try {
                stop();
            } catch (...) {
            }
        }

        void set_on_data_arrived(std::function<void(bytevec const &)> const &fun) {
            std::unique_lock cl{callback_mtp_};
            on_data_arrived_ = fun;
        }

        void set_on_data_arrived(std::function<void(bytevec const &)> &&fun) {
            std::unique_lock cl{callback_mtp_};
            on_data_arrived_ = std::move(fun);
        }

        long double last_data_arrived_time() const {
            return last_data_arrived_time_;
        }

        long double seconds_with_no_data_arrivals() const {
            return steady_time_sec() - last_data_arrived_time_;
        }

        void start(std::string host, std::uint16_t port) {
            stop_ = false;
            host_ = host;
            port_ = port;
            std::unique_lock lck{ucm_mtp_};
            ucm_ = std::make_unique<teal::net::udp_client_muxed<1400>>(cq_);
            ucm_->set_on_data_arrived([this](bytevec const &bv) {
                last_data_arrived_time_ = steady_time_sec();
                notify_on_data_arrived(bv);
            });
            last_data_arrived_time_ = steady_time_sec();
            if(!ucm_->create(host, port)) {
                ucm_->set_on_data_arrived(nullptr);
            }
        }

        void stop() {
            stop_ = true;
            std::unique_lock lck{ucm_mtp_};
            ucm_.reset();
        }

        bool connected() const {
            std::shared_lock lck{ucm_mtp_};
            return ucm_ && ucm_->connected() && seconds_with_no_data_arrivals() < 5;
        }

        int send(const void *data, size_t data_size) {
            std::shared_lock lck{ucm_mtp_};
            if(!ucm_) {
                throw std::runtime_error{"connection not ready"};
            }
            return ucm_->send(bytevec{static_cast<uint8_t const *>(data),
                   static_cast<uint8_t const *>(data) + data_size}) ?
                   data_size : 0;
        }

        int send(bytevec const &data) {
            return send(data.data(), data.size());
        }

        int send(std::string const &data) {
            return send(data.data(), data.size());
        }

        std::optional<bytevec> receive() {
            std::shared_lock lck{ucm_mtp_};
            if(!ucm_) {
                throw std::runtime_error{"connection not ready"};
            }
            std::optional<bytevec> in{ucm_->receive()};
            lck.unlock();
            if(in) {
                std::unique_lock l{muxing_mtp_};
                demux_bottom_layer_.set_network_input(*in);
                if(auto ofid{demux_bottom_layer_.fetch_in_data()}) {
                    return demuxer_.add_data(*ofid);
                }
            }
            return {};
        }

        std::string const &host() const {
            return host_;
        }

        std::uint16_t port() const {
            return port_;
        }

    private:
        void notify_on_data_arrived(bytevec const &d) {
            std::shared_lock cl{callback_mtp_};
            if(on_data_arrived_) on_data_arrived_(d);
        }

    private:
        long double last_data_arrived_time_{};
        command_queue *cq_{nullptr};
        mutable std::shared_mutex ucm_mtp_{};
        std::unique_ptr<teal::net::udp_client_muxed<1400>> ucm_{};
        mutable std::shared_mutex callback_mtp_{};
        std::function<void(bytevec const &)> on_data_arrived_{nullptr};
        mutable std::shared_mutex muxing_mtp_{};
        net::sized_packets_exchanger mux_bottom_layer_{};
        net::packets_muxer<std::uint32_t> muxer_{1400};
        net::sized_packets_exchanger demux_bottom_layer_{};
        net::packets_demuxer demuxer_{};
        std::string host_{};
        std::uint16_t port_{0};
        bool stop_{false};
    };

}
