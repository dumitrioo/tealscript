#pragma once

#include "inc/commondefs.hpp"

#include "tealscript_util.hpp"
#include "tealscript_value.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_exec_ctx.hpp"

namespace teal {

    class statement {
    public:
        virtual ~statement() {}
        virtual void exec(execution_context *) = 0;
        virtual void skip_frame_creation() {}
        virtual bool empty_statement() const { return false; }
        void set_loc(std::int64_t line, std::int64_t col) { line_ = line; col_ = col; }
        std::int64_t line() const { return line_; }
        std::int64_t col() const { return col_; }

    private:
        std::int64_t line_{0};
        std::int64_t col_{0};
    };

    using statement_ptr = std::shared_ptr<statement>;

    class statement_empty: public statement {
    public:
        void exec(execution_context *) override {}
        bool empty_statement() const override { return true; }
    };

    class statement_throw: public statement {
    public:
        statement_throw(expr_ptr throwed): throwed_{throwed} {}

        void exec(execution_context *ctx) override {
            throw throwed_->eval(ctx, eval_caller_type::no_matter, nullptr);
        }

    private:
        expr_ptr throwed_{};
    };

    class statement_try_catch: public statement {
    public:
        statement_try_catch(
            statement_ptr try_stat,
            expr_ptr catch_expr,
            statement_ptr catch_stat
        ):
            try_stat_{try_stat},
            catch_expr_{catch_expr},
            catch_stat_{catch_stat}
        {
            try_stat_->skip_frame_creation();
            catch_stat_->skip_frame_creation();
        }

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) { return; }
            ctx->new_stack_frame();
            teal::shut_on_destroy leave_try_frame{[&]() { ctx->del_stack_frame(); }};
            std::string err_msg{""};
            valbox err_obj{valbox_no_initialize::dont_do_it};
            bool excepted_obj{false};
            bool excepted{false};
            try {
                try_stat_->exec(ctx);
            } catch(valbox const &e) {
                err_obj = std::move(e);
                excepted_obj = true;
                excepted = true;
            } catch(std::exception const &e) {
                err_msg = e.what();
                excepted = true;
            } catch(...) {
                excepted = true;
            }
            if(excepted) {
                if(catch_expr_->is_symbolic()) {
                    ctx->new_stack_frame();
                    teal::shut_on_destroy leave_catch_frame{[&]() { ctx->del_stack_frame(); }};
                    bool old{ctx->set_create_if_not_exists(true)};
                    valbox ce{catch_expr_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    ctx->set_create_if_not_exists(old);
                    if(excepted_obj) {
                        ce.assign(std::move(err_obj));
                    } else {
                        ce.assign(err_msg);
                    }
                    catch_stat_->exec(ctx);
                } else {
                    catch_stat_->exec(ctx);
                }
            }
        }

    private:
        statement_ptr try_stat_{};
        expr_ptr catch_expr_{};
        statement_ptr catch_stat_{};
    };

    class statement_if_else: public statement {
    public:
        statement_if_else(
            expr_ptr cond_expr,
            statement_ptr if_stat,
            statement_ptr else_stat
        ):
            cond_expr_{cond_expr},
            if_stat_{if_stat},
            else_stat_{else_stat}
        {
            if_stat_->skip_frame_creation();
            if(else_stat_) {
                else_stat_->skip_frame_creation();
            }
        }

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) { return; }
            ctx->new_stack_frame();
            teal::shut_on_destroy leave_frame{[&]() {
                ctx->del_stack_frame();
            }};
            bool bcond{false};
            valbox cond{cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
            if(cond.is_class_ref()) {
                str_map_t<std::function<valbox(valbox &)>> const *unops{
                    &(ctx->rt_interface()->get_object_services(cond.class_name())->unops)
                };
                if(unops != nullptr) {
                    auto it{unops->find("(bool)")};
                    if(it != unops->end()) {
                        bcond = it->second(cond).cast_to_bool();
                    }
                }
            } else {
                bcond = cond.cast_to_bool();
            }
            if(bcond) {
                if_stat_->exec(ctx);
            } else {
                if(else_stat_) {
                    else_stat_->exec(ctx);
                }
            }
        }

    private:
        expr_ptr cond_expr_{};
        statement_ptr if_stat_{};
        statement_ptr else_stat_{};
    };

    class statement_expr: public statement {
    public:
        statement_expr(expr_ptr expr): expr_{expr} {}

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) { return; }
            expr_->eval(ctx, eval_caller_type::no_matter, nullptr);
        }

    private:
        expr_ptr expr_{};
    };

    class statement_compound: public statement {
    public:
        statement_compound() = default;

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested() || stats_.empty()) {
                return;
            }
            if(own_frame_) {
                ctx->new_stack_frame();
                teal::shut_on_destroy leave_frame{[&]() { ctx->del_stack_frame(); }};
                process(ctx);
            } else {
                process(ctx);
            }
        }

        void push_back(statement_ptr const &s) {
            stats_.push_back(s);
        }

        void skip_frame_creation() override { own_frame_ = false; }

        size_t num_substatements() const {
            return stats_.size();
        }

        statement_ptr get_tatement_at(size_t indx) const {
            if(indx >= stats_.size()) {
                throw range_error{line(), col(), "statement index out of range"};
            }
            return stats_.at(indx);
        }

    private:
        void process(execution_context *ctx) {
            for(auto &&s: stats_) {
                s->exec(ctx);
                if(ctx->some_jump_requested()) {
                    return;
                }
            }
        }
        std::vector<statement_ptr> stats_{};
        bool own_frame_{true};
    };

    class statement_while: public statement {
    public:
        statement_while(expr_ptr cond_expr, statement_ptr const &stat):
            cond_expr_{cond_expr},
            stat_{stat}
        {
            stat_->skip_frame_creation();
        }

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) { return; }
            ctx->new_stack_frame();
            teal::shut_on_destroy leave_frame{[&]() {
                ctx->del_stack_frame();
            }};

            valbox cond{cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
            bool bcond{false};
            std::string classname{};
            str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
            std::function<valbox(valbox &)> converter{};
            if(cond.is_class_ref()) {
                classname = cond.class_name();
                unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                if(unops == nullptr) {
                    throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                }
                auto it{unops->find("(bool)")};
                if(it == unops->end()) {
                    throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                }
                converter = it->second;
                bcond = converter(cond).cast_to_bool();
            } else {
                bcond = cond.cast_to_bool();
            }
            while(bcond) {
                if(stat_) { stat_->exec(ctx); }
                if(ctx->return_requested()) { return; }
                if(ctx->continue_requested()) { ctx->clear_continue_request(); continue; }
                if(ctx->break_requested()) { ctx->clear_break_request(); break; }
                cond = cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                if(cond.is_class_ref()) {
                    if(cond.class_name() != classname) {
                        classname = cond.class_name();
                        unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                        if(unops == nullptr) {
                            throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                        }
                        auto it{unops->find("(bool)")};
                        if(it == unops->end()) {
                            throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                        }
                        converter = it->second;
                    }
                    bcond = converter(cond).cast_to_bool();
                } else {
                    classname.clear();
                    bcond = cond.cast_to_bool();
                }
            }
        }

    private:
        expr_ptr cond_expr_{};
        statement_ptr stat_{};
    };

    class statement_for: public statement {
    public:
        statement_for(
            expr_ptr const &init_expr,
            expr_ptr const &cond_expr,
            expr_ptr const &incr_expr,
            statement_ptr const &stat
        ):
            init_expr_{init_expr},
            cond_expr_{cond_expr},
            incr_expr_{incr_expr},
            stat_{stat}
        {
            stat_->skip_frame_creation();
        }

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) { return; }
            ctx->new_stack_frame();
            teal::shut_on_destroy leave_frame{[&]() {
                ctx->del_stack_frame();
            }};
            init_expr_->eval(ctx, eval_caller_type::no_matter, nullptr);
            if(stat_->empty_statement()) {
                valbox cond{cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                bool bcond{false};
                std::string classname{};
                str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                std::function<valbox(valbox &)> converter{};
                if(cond.is_class_ref()) {
                    classname = cond.class_name();
                    unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                    if(unops == nullptr) {
                        throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                    }
                    auto it{unops->find("(bool)")};
                    if(it == unops->end()) {
                        throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                    }
                    converter = it->second;
                    bcond = converter(cond).cast_to_bool();
                } else {
                    bcond = cond.cast_to_bool();
                }
                while(bcond) {
                    stat_->exec(ctx);
                    if(ctx->return_requested()) { return; }
                    if(ctx->continue_requested()) { ctx->clear_continue_request(); continue; }
                    if(ctx->break_requested()) { ctx->clear_break_request(); break; }
                    incr_expr_->eval(ctx, eval_caller_type::no_matter, nullptr);
                    cond = cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                    if(cond.is_class_ref()) {
                        if(cond.class_name() != classname) {
                            classname = cond.class_name();
                            unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                            if(unops == nullptr) {
                                throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                            }
                            auto it{unops->find("(bool)")};
                            if(it == unops->end()) {
                                throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                            }
                            converter = it->second;
                        }
                        bcond = converter(cond).cast_to_bool();
                    } else {
                        classname.clear();
                        bcond = cond.cast_to_bool();
                    }
                }
            } else {
                valbox cond{cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                bool bcond{false};
                std::string classname{};
                str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                std::function<valbox(valbox &)> converter{};
                if(cond.is_class_ref()) {
                    classname = cond.class_name();
                    unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                    if(unops == nullptr) {
                        throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                    }
                    auto it{unops->find("(bool)")};
                    if(it == unops->end()) {
                        throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                    }
                    converter = it->second;
                    bcond = converter(cond).cast_to_bool();
                } else {
                    bcond = cond.cast_to_bool();
                }
                while(bcond) {
                    stat_->exec(ctx);
                    if(ctx->return_requested()) { return; }
                    if(ctx->continue_requested()) { ctx->clear_continue_request(); continue; }
                    if(ctx->break_requested()) { ctx->clear_break_request(); break; }
                    incr_expr_->eval(ctx, eval_caller_type::no_matter, nullptr);
                    cond = cond_expr_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                    if(cond.is_class_ref()) {
                        if(cond.class_name() != classname) {
                            classname = cond.class_name();
                            unops = &(ctx->rt_interface()->get_object_services(classname)->unops);
                            if(unops == nullptr) {
                                throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                            }
                            auto it{unops->find("(bool)")};
                            if(it == unops->end()) {
                                throw runtime_error{line(), col(), "invalid condition conversion to logical value"};
                            }
                            converter = it->second;
                        }
                        bcond = converter(cond).cast_to_bool();
                    } else {
                        classname.clear();
                        bcond = cond.cast_to_bool();
                    }
                }
            }
        }

    private:
        expr_ptr init_expr_{};
        expr_ptr cond_expr_{};
        expr_ptr incr_expr_{};
        statement_ptr stat_{};
    };

    class statement_continue: public statement {
    public:
        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) {
                return;
            }
            ctx->request_continue();
        }
    };

    class statement_break: public statement {
    public:
        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) {
                return;
            }
            ctx->request_break();
        }
    };

    class statement_return: public statement {
    public:
        statement_return(expr_ptr ret_expr): ret_expr_{ret_expr} {}

        void exec(execution_context *ctx) override {
            if(ctx->some_jump_requested()) {
                return;
            }
            ctx->set_return_result(ret_expr_->eval(ctx, eval_caller_type::no_matter, nullptr));
            ctx->request_return();
        }

    private:
        expr_ptr ret_expr_{};
    };

}
