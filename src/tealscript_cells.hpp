#pragma once

#include "inc/commondefs.hpp"
#include "inc/file_util.hpp"
#include "inc/str_util.hpp"
#include "inc/net/url.hpp"

#include "tealscript_util.hpp"
#include "tealscript_token.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_statement.hpp"
#include "tealscript_exec_ctx.hpp"

namespace teal {

    class cell_base {
    public:
        virtual ~cell_base() {}
        virtual void exec(execution_context *) {}
        virtual void set_value(valbox const &val) { std::unique_lock l{val_mtp_}; val_ = val.clone(); }
        virtual valbox value() const { std::shared_lock l{val_mtp_}; return val_; }
        virtual valbox value_clone() const { std::shared_lock l{val_mtp_}; return val_.clone(); }
        virtual void set_inst_name(std::string const &name) { inst_name_ = name; }
        virtual std::string const &inst_name() const & { return inst_name_; }
        virtual void set_loc(std::int64_t l, std::int64_t c) { line_ = l; col_ = c; }
        virtual std::int64_t line() const { return line_; }
        virtual std::int64_t col() const { return col_; }

    private:
        std::int64_t line_{};
        std::int64_t col_{};
        mutable shared_mutex val_mtp_{};
        valbox val_{};
        std::string inst_name_{};
    };


    class input_cell: public cell_base {
    public:
        void set_input_name(std::string const &name) { input_name_ = name; }
        std::string const &input_name() const { return input_name_; }

    private:
        std::string input_name_{};
    };


    class extern_cell: public cell_base {
    public:
        virtual void exec(execution_context *) {
        }

        void set_remote_var_name(std::string const &name) { remote_name_ = name; }
        std::string const &remote_var_name() const { return remote_name_; }

        void set_url(url const &u) { url_ = u; }
        url const &remote_url() const { return url_; }

        void set_remote_host(std::string const &v) { remote_host_ = v; }
        std::string const &remote_host() const { return remote_host_; }

    private:
        url url_{};
        std::string remote_name_{};
        std::string remote_host_{};
    };

    class worker_cell_definition_info {
    public:
        void set_arg_name(std::size_t indx, std::string const &name) {
            if(arg_number_by_name_.find(name) != arg_number_by_name_.end()) {
                throw runtime_error{
                    line_, col_, name + ": cell instance duplicated argument identifier"
                };
            }
            if(arg_names_.size() <= indx) {
                arg_names_.resize(indx + 1);
            }
            arg_names_[indx] = name;
            arg_number_by_name_[name] = indx;
        }

        std::int64_t arg_number_by_name(std::string const &name) const {
            auto it{arg_number_by_name_.find(name)};
            if(arg_number_by_name_.end() == it) {
                return -1;
            }
            return it->second;
        }

        std::string arg_name_by_number(std::size_t num) const {
            if(num >= arg_names_.size()) {
                return {};
            }
            return arg_names_[num];
        }

        void set_type_name(std::string const &name) {
            type_name_ = name;
        }

        std::string const &type_name() const & {
            return type_name_;
        }

        std::int64_t num_args() const {
            return arg_names_.size();
        }

        std::vector<std::string> const &arg_names() const {
            return arg_names_;
        }

        str_map_t<std::int64_t> const &arg_numbers_by_name() const {
            return arg_number_by_name_;
        }

        void set_loc(std::int64_t l, std::int64_t c) {
            line_ = l;
            col_ = c;
        }

        std::int64_t line() const {
            return line_;
        }

        std::int64_t col() const {
            return col_;
        }

    private:
        std::int64_t line_{};
        std::int64_t col_{};
        std::string type_name_{};
        std::vector<std::string> arg_names_{};
        str_map_t<std::int64_t> arg_number_by_name_{};
    };


    class worker_cell_instance: public cell_base {
    public:
        struct arg_info {
            std::string argname{};
            bool is_cell{false};
            std::string cell_name{};
            expr_ptr expr{nullptr};
            valbox expr_val{valbox_no_initialize::dont_do_it};
            cell_base *cell_ptr{nullptr};
        };

        void set_type_info(
            std::int64_t num_args,
            std::vector<std::string> const &arg_names
        ) {
            type_info_transferred_ = 1;

            if((std::int64_t)args_info_.size() != num_args) {
                args_info_.resize(num_args);
            }
            for(std::size_t curr_arg_number{0}; curr_arg_number < args_info_.size(); ++curr_arg_number) {
                args_info_[curr_arg_number].argname = arg_names.at(curr_arg_number);
            }
        }

        bool type_info_transferred() const {
            return type_info_transferred_ != 0;
        }

        void set_act_arg_source(std::size_t indx, std::string const &cell_name) {
            if(args_info_.size() <= indx) {
                args_info_.resize(indx + 1);
                args_info_[indx].is_cell = true;
                args_info_[indx].cell_name = cell_name;
            }
        }

        void set_act_arg_expr(std::size_t indx, expr_ptr ex) {
            if(args_info_.size() <= indx) {
                args_info_.resize(indx + 1);
                args_info_[indx].is_cell = false;
                args_info_[indx].expr = ex;
            }
        }

        std::vector<arg_info> &actual_args_info() & {
            return args_info_;
        }

        std::vector<arg_info> const &actual_args_info() const & {
            return args_info_;
        }

        void set_type_name(std::string const &name) {
            type_name_ = name;
        }

        std::string const &type_name() const & {
            return type_name_;
        }

        void set_output_name(std::string const &name) {
            output_name_ = name;
        }

        std::string const &output_name() const & {
            return output_name_;
        }

        str_map_t<valbox> *cell_self_values_ptr() & {
            return &cell_self_values_;
        }

        bool try_lock() {
            return locker_->try_lock();
        }

        void unlock() {
            locker_->unlock();
        }

        statement_ptr body() {
            return body_ptr_;
        }

        void set_body(statement_ptr val) {
            body_ptr_ = val;
        }

    private:
        statement_ptr body_ptr_{};
        std::string type_name_{};
        str_map_t<valbox> cell_self_values_{};
        std::vector<arg_info> args_info_{};
        std::string output_name_{};
        std::uint64_t type_info_transferred_{0};
        mutable std::unique_ptr<shared_mutex> locker_{std::make_unique<shared_mutex>()};
    };


    class function_definition {
    public:
        void set_arg_name(std::size_t indx, std::string const &name) {
            if(arg_number_by_name_.find(name) != arg_number_by_name_.end()) {
                throw runtime_error{
                    line_, col_, name + ": duplicated argument identifier"
                };
            }
            if(arg_names_.size() <= indx) {
                arg_names_.resize(indx + 1);
            }
            arg_names_[indx] = name;
            arg_number_by_name_[name] = indx;
        }

        int arg_number_by_name(std::string const &name) const {
            auto it{arg_number_by_name_.find(name)};
            if(arg_number_by_name_.end() == it) {
                return -1;
            }
            return it->second;
        }

        std::string arg_name_by_number(std::size_t num) const {
            if(num >= arg_names_.size()) {
                return {};
            }
            return arg_names_[num];
        }

        void set_name(std::string const &val) {
            name_ = val;
        }

        std::string const &name() const & {
            return name_;
        }

        std::int64_t num_args() const {
            return arg_names_.size();
        }

        std::vector<std::string> const &arg_names() const {
            return arg_names_;
        }

        str_map_t<std::int64_t> const &arg_number_by_name() const {
            return arg_number_by_name_;
        }

        void set_loc(int l, int c) {
            line_ = l;
            col_ = c;
        }

        std::int64_t line() const {
            return line_;
        }

        std::int64_t col() const {
            return col_;
        }

        void set_body(statement_ptr const &val) {
            body_ = val;
        }

        statement_ptr const &body() const & {
            return body_;
        }

    private:
        std::int64_t line_{};
        std::int64_t col_{};
        std::string name_{};
        statement_ptr body_{};
        std::vector<std::string> arg_names_{};
        str_map_t<std::int64_t> arg_number_by_name_{};
    };

}
