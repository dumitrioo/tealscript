#pragma once

#include "inc/commondefs.hpp"

#include "tealscript_util.hpp"
#include "tealscript_value.hpp"
#include "tealscript_token.hpp"
#include "tealscript_exec_ctx.hpp"

namespace teal {

    enum class eval_caller_type {
        no_matter,
        func_call
    };

    class expression {
    public:
        virtual ~expression() {}
        virtual valbox eval(execution_context *, eval_caller_type, valbox *) = 0;
        virtual std::string const &symbol() const & { static const std::string res{}; return res; }
        virtual bool primary() const { return false; }
        virtual void reset_primary() {}
        void mark_fun_ref() { fun_ref_ = true; }
        bool fun_ref() const { return fun_ref_; }

        void set_loc(std::int64_t line, std::int64_t col) { line_ = line; col_ = col; }
        std::int64_t line() const { return line_; }
        std::int64_t col() const { return col_; }

        virtual bool is_symbolic() const { return false; }
        virtual bool is_binop() const { return false; }
        virtual token::type binop_type() const { return token::type::NONE; }

    protected:
        std::int64_t line_{0};
        std::int64_t col_{0};
        bool fun_ref_{false};
    };

    using expr_ptr = std::shared_ptr<expression>;

    class void_expression: public expression {
    public:
        void_expression() {}
        valbox eval(execution_context *, eval_caller_type, valbox *) override {
            return valbox{valbox_no_initialize::dont_do_it};
        }
    };

    class primary_expression: public expression {
    public:
        primary_expression(valbox const &val): val_{val} {}
        valbox eval(execution_context *, eval_caller_type, valbox *) override { return val_/*.clone()*/; }
        bool primary() const override { return true; }

    private:
        valbox val_{};
    };

    class sym_expression: public expression {
    public:
        sym_expression(std::string const &name): name_{name} {}

        bool primary() const override {
            return primary_.load(std::memory_order_acquire);
        }

        void reset_primary() override {
            primary_ = false;
        }

        valbox eval(execution_context *ctx, eval_caller_type, valbox *) override {
            if(primary()) {
                std::shared_lock l{primary_val_mtp_};
                return primary_val_;
            }
            valbox res{valbox_no_initialize::dont_do_it};
            if(fun_ref()) {
                execution_context::obj_type objtyp{objtyp_.load(std::memory_order_acquire)};
                res = ctx->find_func(name_, objtyp);
                if(!res.is_func_ref()) {
                    res = ctx->find_val_by_sym_name(name_, line(), col(), objtyp);
                }
                objtyp_.store(objtyp, std::memory_order_release);
            } else {
                bool excepted{false};
                runtime_error er{{}, {}, {}};
                try {
                    execution_context::obj_type objtyp{objtyp_.load(std::memory_order_acquire)};
                    res = ctx->find_val_by_sym_name(name_, line(), col(), objtyp);
                    objtyp_.store(objtyp, std::memory_order_release);
                    if(objtyp == execution_context::obj_type::global_var) {
                        std::unique_lock l{primary_val_mtp_};
                        primary_val_ = res;
                        primary_ = true;
                    }
                } catch (runtime_error const &e) {
                    er = e;
                    excepted = true;
                } catch (std::exception const &e) {
                    er = runtime_error{line_, col_, e.what()};
                    excepted = true;
                } catch (...) {
                    er = runtime_error{line_, col_, "unknown error"};
                    excepted = true;
                }
                if(excepted) {
                    throw er;
                }
            }
            return res;
        }

        std::string const &symbol() const & override {
            return name_;
        }

        bool is_symbolic() const override { return true; }

    private:
        mutable shared_mutex primary_val_mtp_{};
        valbox primary_val_{};
        std::string name_{};
        std::atomic<execution_context::obj_type> objtyp_{execution_context::obj_type::unknown};
        std::atomic<bool> primary_{false};
    };

    class prefix_unop_expression: public expression {
        struct enum_hash {
            std::size_t operator()(token::type tkn) const {
                return static_cast<std::size_t>(tkn);
            }
        };

    public:
        prefix_unop_expression(token::type opcode, expr_ptr val, std::string const &new_val_type = std::string{}):
            opcode_{opcode},
            val_{val},
            conversion_type_{new_val_type.empty() ? valbox::type::UNDEFINED : valbox::str_to_type(new_val_type)}
        {
        }

        bool primary() const override {
            return primary_.load(std::memory_order_acquire);
        }

        void reset_primary() override {
            val_->reset_primary();
            primary_ = false;
        }

        valbox eval(execution_context *ctx, eval_caller_type, valbox *) override {
            static std::array<valbox(*)(prefix_unop_expression *, execution_context *), 69> const ops{
                /* NONE */ nullptr,
                /* INT_LITERAL */ nullptr,
                /* HEX_LITERAL */ nullptr,
                /* OCT_LITERAL */ nullptr,
                /* BIN_LITERAL */ nullptr,
                /* FP_LITERAL */ nullptr,
                /* FP_EXP_LITERAL */ nullptr,
                /* STRING_LITERAL */ nullptr,
                /* CHAR_LITERAL */ nullptr,
                /* SPACE */ nullptr,
                /* IDENTIFIER */ nullptr,
                /* PLUS */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    expr_ptr &val{this_->val_};
                    bool old{ctx->set_create_if_not_exists(false)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        valbox t = val->eval(ctx, eval_caller_type::no_matter, nullptr);
                        if(t.is_class_ref()) {
                            str_map_t<std::function<valbox(valbox &)>> const *unops{
                                &(ctx->rt_interface()->get_object_services(t.class_name())->unops)
                            };
                            if(unops != nullptr) {
                                auto it{unops->find("+")};
                                if(it != unops->end()) {
                                    res = it->second(t);
                                } else {
                                    throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                                }
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            res = std::move(t);
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    if(val->primary()) {
                        std::unique_lock l{this_->primary_val_mtp_};
                        this_->primary_val_ = res; this_->primary_ = true;
                    }
                    return res;
                },
                /* MINUS */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    expr_ptr &val{this_->val_};
                    bool old{ctx->set_create_if_not_exists(false)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        valbox t = val->eval(ctx, eval_caller_type::no_matter, nullptr);
                        if(t.is_class_ref()) {
                            str_map_t<std::function<valbox(valbox &)>> const *unops{
                                &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                            };
                            if(unops != nullptr) {
                                auto it{unops->find("-")};
                                if(it != unops->end()) {
                                    res = it->second(t);
                                } else {
                                    throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                                }
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            res = -t;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error{this_->line_, this_->col_, e.what()};
                        excepted = true;
                    } catch (...) {
                        er = runtime_error{this_->line_, this_->col_, "unknown error"};
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    if(val->primary()) {
                        std::unique_lock l{this_->primary_val_mtp_};
                        this_->primary_val_ = res; this_->primary_ = true;
                    }
                    return res;
                },
                /* STAR */ nullptr,
                /* SLASH */ nullptr,
                /* MOD */ nullptr,
                /* AT */ nullptr,
                /* EQUAL */ nullptr,
                /* NOT */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    expr_ptr &val{this_->val_};
                    bool old{ctx->set_create_if_not_exists(false)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    valbox t{val->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    if(t.is_class_ref()) {
                        str_map_t<std::function<valbox(valbox &)>> const *unops{
                            &(ctx->rt_interface()->get_object_services(t.class_name())->unops)
                        };
                        if(unops != nullptr) {
                            auto it{unops->find("!")};
                            if(it != unops->end()) {
                                res = it->second(t);
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                        }
                    } else {
                        res = !t.cast_to_bool();
                    }
                    if(val->primary()) {
                        std::unique_lock l{this_->primary_val_mtp_};
                        this_->primary_val_ = res; this_->primary_ = true;
                    }
                    return res;
                },
                /* NOTEQUAL */ nullptr,
                /* LESSEQUAL */ nullptr,
                /* LESS */ nullptr,
                /* GREATER */ nullptr,
                /* GREATEREQUAL */ nullptr,
                /* SPACESHIP */ nullptr,
                /* ASSIGN */ nullptr,
                /* ADDASSIGN */ nullptr,
                /* SUBASSIGN */ nullptr,
                /* MULASSIGN */ nullptr,
                /* DIVASSIGN */ nullptr,
                /* MODASSIGN */ nullptr,
                /* BITANDASSIGN */ nullptr,
                /* ANDASSIGN */ nullptr,
                /* XORASSIGN */ nullptr,
                /* BITORASSIGN */ nullptr,
                /* ORASSIGN */ nullptr,
                /* COLONCOLONASSIGN */ nullptr,
                /* LSHIFTASSIGN */ nullptr,
                /* RSHIFTASSIGN */ nullptr,
                /* INCREMENT */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    bool old{ctx->set_create_if_not_exists(true)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    runtime_error er{{}, {}, {}};
                    valbox res{valbox_no_initialize::dont_do_it};
                    try {
                        res = this_->val_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                        if(res.is_class_ref()) {
                            str_map_t<std::function<valbox(valbox &)>> const *unops{
                                &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                            };
                            if(unops != nullptr) {
                                auto it{unops->find("prefix++")};
                                if(it != unops->end()) {
                                    res = it->second(res);
                                } else {
                                    throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                                }
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            ++res;
                        }
                        return res;
                    } catch (runtime_error const &e) {
                        er = e;
                    } catch (std::exception const &e) {
                        er = runtime_error{this_->line_, this_->col_, e.what()};
                    } catch (...) {
                        er = runtime_error{this_->line_, this_->col_, "unknown error"};
                    }
                    throw er;
                },
                /* DECREMENT */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    bool old{ctx->set_create_if_not_exists(true)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    runtime_error er{{}, {}, {}};
                    valbox res{valbox_no_initialize::dont_do_it};
                    try {
                        res = this_->val_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                        if(res.is_class_ref()) {
                            str_map_t<std::function<valbox(valbox &)>> const *unops{
                                &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                            };
                            if(unops != nullptr) {
                                auto it{unops->find("prefix--")};
                                if(it != unops->end()) {
                                    res = it->second(res);
                                } else {
                                    throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                                }
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            --res;
                        }
                        return res;
                    } catch (runtime_error const &e) {
                        er = e;
                    } catch (std::exception const &e) {
                        er = runtime_error{this_->line_, this_->col_, e.what()};
                    } catch (...) {
                        er = runtime_error{this_->line_, this_->col_, "unknown error"};
                    }
                    throw er;
                    return res;
                },
                /* LSHIFT */ nullptr,
                /* RSHIFT */ nullptr,
                /* DOTDOT */ nullptr,
                /* STARSTAR */ nullptr,
                /* QUESTIONCOLON */ nullptr,
                /* QUESTIONQUESTION */ nullptr,
                /* BITNOT */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    expr_ptr &val{this_->val_};
                    bool old{ctx->set_create_if_not_exists(false)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        res = this_->val_->eval(ctx, eval_caller_type::no_matter, nullptr);
                        if(res.is_class_ref()) {
                            str_map_t<std::function<valbox(valbox &)>> const *unops{
                                &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                            };
                            if(unops != nullptr) {
                                auto it{unops->find("~")};
                                if(it != unops->end()) {
                                    res = it->second(res);
                                } else {
                                    throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                                }
                            } else {
                                throw runtime_error{this_->val_->line(), this_->val_->col(), "unsupported operation"};
                            }
                        } else {
                            res = ~res;
                        }
                        return res;
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    if(val->primary()) {
                        std::unique_lock l{this_->primary_val_mtp_};
                        this_->primary_val_ = res; this_->primary_ = true;
                    }
                    return res;
                },
                /* XOR */ nullptr,
                /* CIRCUMFLEXCIRCUMFLEX */ nullptr,
                /* BITOR */ nullptr,
                /* OR */ nullptr,
                /* BITAND */ nullptr,
                /* AND */ nullptr,
                /* QUESTION */ nullptr,
                /* COLON */ nullptr,
                /* COLONCOLON */ nullptr,
                /* SEMICOLON */ nullptr,
                /* LPAREN */ nullptr,
                /* RPAREN */ nullptr,
                /* LBRACKET */ nullptr,
                /* RBRACKET */ nullptr,
                /* LCURLY */ nullptr,
                /* RCURLY */ nullptr,
                /* COMMA */ nullptr,
                /* DOT */ nullptr,
                /* FUNCCALL */ nullptr,
                /* TYPECAST */
                [](prefix_unop_expression *this_, execution_context *ctx) -> valbox {
                    expr_ptr &expr{this_->val_};
                    valbox v{};
                    bool old{ctx->set_create_if_not_exists(false)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        res.become_type(this_->conversion_type_);
                        res.assign_preserving_type(expr->eval(ctx, eval_caller_type::no_matter, nullptr));
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    if(expr->primary()) {
                        std::unique_lock l{this_->primary_val_mtp_};
                        this_->primary_val_ = res; this_->primary_ = true;
                    }
                    return res;
                },
                /* ENDOFFILE */ nullptr,
            };
            if(primary()) {
                std::shared_lock l{primary_val_mtp_};
                return primary_val_;
            }
            auto &&fn{ops[static_cast<std::size_t>(opcode_)]};
            if(fn == nullptr) {
                throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
            }
            return fn(this, ctx);
        }

    private:
        token::type opcode_{};
        expr_ptr val_{};
        valbox::type conversion_type_{};
        mutable shared_mutex primary_val_mtp_{};
        valbox primary_val_{};
        std::atomic<bool> primary_{false};
    };

    class postfix_unop_expression: public expression {
    public:
        postfix_unop_expression(expr_ptr val, token::type opcode):
            opcode_{opcode},
            val_{val}
        {
        }

        void reset_primary() override {
            val_->reset_primary();
        }

        valbox eval(execution_context *ctx, eval_caller_type, valbox *) override {
            switch(opcode_) {
                case token::type::INCREMENT: {
                    bool old{ctx->set_create_if_not_exists(true)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{val_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    if(res.is_class_ref()) {
                        str_map_t<std::function<valbox(valbox &)>> const *unops{
                            &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                        };
                        if(unops != nullptr) {
                            auto it{unops->find("postfix++")};
                            if(it != unops->end()) {
                                res = it->second(res);
                            } else {
                                throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
                            }
                        } else {
                            throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
                        }
                    } else {
                        res = res++;
                    }
                    return res;
                }
                case token::type::DECREMENT: {
                    bool old{ctx->set_create_if_not_exists(true)};
                    teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{val_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    if(res.is_class_ref()) {
                        str_map_t<std::function<valbox(valbox &)>> const *unops{
                            &(ctx->rt_interface()->get_object_services(res.class_name())->unops)
                        };
                        if(unops != nullptr) {
                            auto it{unops->find("postfix--")};
                            if(it != unops->end()) {
                                res = it->second(res);
                            } else {
                                throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
                            }
                        } else {
                            throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
                        }
                    } else {
                        res = res--;
                    }
                    return res;
                }
                default:
                    throw runtime_error{val_->line(), val_->col(), "unsupported operation"};
            }
        }

    private:
        token::type opcode_{};
        expr_ptr val_{};
    };

    class func_call_expression: public expression {
    public:
        func_call_expression(expr_ptr func, std::vector<expr_ptr> &&args):
            func_{func},
            args_{std::move(args)}
        {
            func_->mark_fun_ref();
        }

        func_call_expression(expr_ptr func, std::vector<expr_ptr> const &args):
            func_{func},
            args_{args}
        {
            func_->mark_fun_ref();
        }

        valbox eval(execution_context *ctx, eval_caller_type, valbox *) override {
            bool old{ctx->set_create_if_not_exists(false)};
            teal::shut_on_destroy sod{[ctx, old]() { ctx->set_create_if_not_exists(old); }};
            std::string errstr{};
            valbox left_of_dot{valbox_no_initialize::dont_do_it};
            valbox fn{func_->eval(ctx, eval_caller_type::func_call, &left_of_dot)};
            if(!fn.is_func_ref()) {
                throw runtime_error{func_->line(), func_->col(), "calling of non-function"};
            }
            std::string fsym{fn.func_name()};
            bool user_fn_seltor{fn.is_user_func()};
            std::vector<valbox> act_args{};
            if(fsym == "exit" || fsym == "assert") {
                if(fsym == "exit") {
                    act_args.reserve(args_.size() + 2);
                    act_args.push_back((void *)ctx);
                } else if(fsym == "assert") {
                    act_args.reserve(args_.size() + 3);
                    act_args.push_back(line());
                    act_args.push_back(col());
                }
            } else {
                if(user_fn_seltor) {
                    act_args.reserve(args_.size() + 3);
                    act_args.push_back((void *)ctx);
                    act_args.push_back(fsym);
                } else {
                    act_args.reserve(args_.size() + 1);
                }
            }
            if(
                func_->is_binop() &&
                (
                    func_->binop_type() == token::type::DOT ||
                    func_->binop_type() == token::type::LBRACKET
                )
            ) {
                act_args.push_back(left_of_dot);
            }
            for(auto &&ex: args_) {
                valbox v{ex->eval(ctx, eval_caller_type::no_matter, nullptr)};
                act_args.push_back(v);
            }
            if(user_fn_seltor) {
                ctx->set_stack_barrier();
                teal::shut_on_destroy csb{[ctx]() { ctx->clear_stack_barrier(); }};
                valbox res{fn.as_func()(act_args)};
                ctx->clear_return_request();
                return res;
            } else {
                runtime_error rte{0, 0, ""};
                try {
                    return fn.as_func()(act_args);
                } catch (runtime_error const &e) {
                    rte = e;
                } catch (std::exception const &e) {
                    rte = runtime_error{line(), col(), e.what()};
                }
                throw rte;
            }
        }

    private:
        expr_ptr func_{};
        std::vector<expr_ptr> args_{};
    };

    class ternop_expression: public expression {
    public:
        ternop_expression(expr_ptr cond, expr_ptr on_true_val, expr_ptr on_false_rval):
            cond_{cond},
            on_true_{on_true_val},
            on_false_{on_false_rval}
        {
        }

        void reset_primary() override {
            on_true_->reset_primary();
            on_false_->reset_primary();
        }

        valbox eval(execution_context *ctx, eval_caller_type, valbox *) override {
            if(cond_->eval(ctx, eval_caller_type::no_matter, nullptr).deref().cast_to_bool()) {
                return on_true_->eval(ctx, eval_caller_type::no_matter, nullptr);
            } else {
                return on_false_->eval(ctx, eval_caller_type::no_matter, nullptr);
            }
        }

    private:
        expr_ptr cond_{};
        expr_ptr on_true_{};
        expr_ptr on_false_{};
    };

    class binop_expression: public expression {
    public:
        binop_expression(token::type opcode, expr_ptr lval, expr_ptr rval):
            opcode_{opcode},
            lval_{lval},
            rval_{rval}
        {
        }

        bool is_binop() const override { return true; }
        token::type binop_type() const override { return opcode_; }

        bool primary() const override {
            return primary_.load(std::memory_order_acquire);
        }

        void reset_primary() override {
            lval_->reset_primary();
            rval_->reset_primary();
            primary_ = false;
        }

        valbox eval(execution_context *ctx, eval_caller_type caller_type, valbox *dotlptr) override {
            static std::array<valbox(*)(binop_expression *, execution_context *, eval_caller_type, valbox *), 69> const ops{
                /* NONE */ nullptr,
                /* INT_LITERAL */ nullptr,
                /* HEX_LITERAL */ nullptr,
                /* OCT_LITERAL */ nullptr,
                /* BIN_LITERAL */ nullptr,
                /* FP_LITERAL */ nullptr,
                /* FP_EXP_LITERAL */ nullptr,
                /* STRING_LITERAL */ nullptr,
                /* CHAR_LITERAL */ nullptr,
                /* SPACE */ nullptr,
                /* IDENTIFIER */ nullptr,
                /* PLUS */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("+")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l + r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* MINUS */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("-")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l - r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* STAR */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("*")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l * r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* SLASH */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("/")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l / r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* MOD */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("%")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l % r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* AT */ nullptr,
                /* EQUAL */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    auto l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    auto r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    auto lt{l.val_or_pointed_type()};
                    auto rt{r.val_or_pointed_type()};
                    if(
                        (lt == valbox::type::UNDEFINED && rt != valbox::type::UNDEFINED) ||
                        (rt == valbox::type::UNDEFINED && lt != valbox::type::UNDEFINED)
                    ) {
                        return false;
                    }
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("==")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l == r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* NOT */ nullptr,
                /* NOTEQUAL */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    auto l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    auto r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    auto lt{l.val_or_pointed_type()};
                    auto rt{r.val_or_pointed_type()};
                    if(
                        (lt == valbox::type::UNDEFINED && rt != valbox::type::UNDEFINED) ||
                        (rt == valbox::type::UNDEFINED && lt != valbox::type::UNDEFINED)
                        ) {
                        return true;
                    }
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("==")};
                            if(it != binops->end()) {
                                res = !it->second(l, r).cast_to_bool();
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l != r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* LESSEQUAL */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<=>")};
                            if(it != binops->end()) {
                                res = it->second(l, r).cast_to_int() <= 0;
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l <= r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* LESS */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<=>")};
                            if(it != binops->end()) {
                                res = it->second(l, r).cast_to_int() < 0;
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = l < r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* GREATER */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<=>")};
                            if(it != binops->end()) {
                                res = it->second(l, r).cast_to_int() > 0;
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = r < l;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* GREATEREQUAL */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<=>")};
                            if(it != binops->end()) {
                                res = it->second(l, r).cast_to_int() >= 0;
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
                            res = !(l < r);
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* SPACESHIP */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<=>")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "unsupported operation"};
                            }
                        } else {
#if (__cplusplus >= 202000L)
                            res = l <=> r;
#else
                            res = valbox::operator_spaceship(l,  r);
#endif
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                }
                ,
                /* ASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    auto vopt{r.val_or_pointed_type()};
                    if(vopt == valbox::type::ARRAY || vopt == valbox::type::OBJECT) {
                        r = r.clone();
                    }
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(l.is_ptr()) {
                        l.assign_preserving_type(r);
                    } else {
                        l.assign(r);
                    }
                    return l;
                },
                /* ADDASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(!r.is_undefined_ref()) {
                        if(l.is_undefined_ref()) {
                            l.assign(r);
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find("+")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l + r);
                                    }
                                } else {
                                    l.assign_preserving_type(l + r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* SUBASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(!r.is_undefined_ref()) {
                        if(l.is_undefined_ref()) {
                            l.assign(-r);
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find("-")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l - r);
                                    }
                                } else {
                                    l.assign_preserving_type(l - r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* MULASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                        if(!l.is_undefined_ref()) {
                            l.assign_preserving_type(0);
                        }
                    } else {
                        if(l.is_undefined_ref()) {
                            l.become_same_type_as(r);
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find("*")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l * r);
                                    }
                                } else {
                                    l.assign_preserving_type(l * r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* DIVASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                        throw runtime_error{this_->line_, this_->col_, "division by undefined value"};
                    } else {
                        bool excepted{false};
                        runtime_error er{{}, {}, {}};
                        try {
                            str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                            if(l.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                            } else if(r.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                            }
                            if(binops != nullptr) {
                                auto add_it{binops->find("/")};
                                auto assign_it{binops->find("=")};
                                if(add_it != binops->end() && assign_it != binops->end()) {
                                    valbox res{add_it->second(l, r)};
                                    assign_it->second(l, res);
                                } else {
                                    l.assign_preserving_type(l / r);
                                }
                            } else {
                                l.assign_preserving_type(l / r);
                            }
                        } catch (runtime_error const &e) {
                            er = e;
                            excepted = true;
                        } catch (std::exception const &e) {
                            er = runtime_error(this_->line_, this_->col_, e.what());
                            excepted = true;
                        } catch (...) {
                            er = runtime_error(this_->line_, this_->col_, "unknown error");
                            excepted = true;
                        }
                        if(excepted) {
                            throw er;
                        }
                    }
                    return l;
                },
                /* MODASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                        throw runtime_error{this_->line_, this_->col_, "division by undefined value"};
                    } else {
                        bool excepted{false};
                        runtime_error er{{}, {}, {}};
                        try {
                            str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                            if(l.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                            } else if(r.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                            }
                            if(binops != nullptr) {
                                auto add_it{binops->find("%")};
                                auto assign_it{binops->find("=")};
                                if(add_it != binops->end() && assign_it != binops->end()) {
                                    valbox res{add_it->second(l, r)};
                                    assign_it->second(l, res);
                                } else {
                                    l.assign_preserving_type(l % r);
                                }
                            } else {
                                l.assign_preserving_type(l % r);
                            }
                        } catch (runtime_error const &e) {
                            er = e;
                            excepted = true;
                        } catch (std::exception const &e) {
                            er = runtime_error(this_->line_, this_->col_, e.what());
                            excepted = true;
                        } catch (...) {
                            er = runtime_error(this_->line_, this_->col_, "unknown error");
                            excepted = true;
                        }
                        if(excepted) {
                            throw er;
                        }
                        // }
                    }
                    return l;
                },
                /* BITANDASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                        l.assign_preserving_type(0);
                    } else {
                        if(l.is_undefined_ref()) {
                            l.become_same_type_as(r);
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find("&")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l & r);
                                    }
                                } else {
                                    l.assign_preserving_type(l & r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* ANDASSIGN */ nullptr,
                /* XORASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                    } else {
                        if(l.is_undefined_ref()) {
                            l.become_same_type_as(r);
                        }
                        bool excepted{false};
                        runtime_error er{{}, {}, {}};
                        try {
                            str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                            if(l.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                            } else if(r.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                            }
                            if(binops != nullptr) {
                                auto add_it{binops->find("^")};
                                auto assign_it{binops->find("=")};
                                if(add_it != binops->end() && assign_it != binops->end()) {
                                    valbox res{add_it->second(l, r)};
                                    assign_it->second(l, res);
                                } else {
                                    l.assign_preserving_type(l ^ r);
                                }
                            } else {
                                l.assign_preserving_type(l ^ r);
                            }
                        } catch (runtime_error const &e) {
                            er = e;
                            excepted = true;
                        } catch (std::exception const &e) {
                            er = runtime_error(this_->line_, this_->col_, e.what());
                            excepted = true;
                        } catch (...) {
                            er = runtime_error(this_->line_, this_->col_, "unknown error");
                            excepted = true;
                        }
                        if(excepted) {
                            throw er;
                        }
                    }
                    return l;
                },
                /* BITORASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                    } else {
                        if(l.is_undefined_ref()) {
                            l.become_same_type_as(r);
                        }
                        bool excepted{false};
                        runtime_error er{{}, {}, {}};
                        try {
                            str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                            if(l.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                            } else if(r.is_class_ref()) {
                                binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                            }
                            if(binops != nullptr) {
                                auto add_it{binops->find("|")};
                                auto assign_it{binops->find("=")};
                                if(add_it != binops->end() && assign_it != binops->end()) {
                                    valbox res{add_it->second(l, r)};
                                    assign_it->second(l, res);
                                } else {
                                    l.assign_preserving_type(l | r);
                                }
                            } else {
                                l.assign_preserving_type(l | r);
                            }
                        } catch (runtime_error const &e) {
                            er = e;
                            excepted = true;
                        } catch (std::exception const &e) {
                            er = runtime_error(this_->line_, this_->col_, e.what());
                            excepted = true;
                        } catch (...) {
                            er = runtime_error(this_->line_, this_->col_, "unknown error");
                            excepted = true;
                        }
                        if(excepted) {
                            throw er;
                        }
                    }
                    return l;
                },
                /* ORASSIGN */ nullptr,
                /* COLONCOLONASSIGN */ nullptr,
                /* LSHIFTASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                    } else {
                        if(l.is_undefined_ref()) {
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find("<<")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l << r);
                                    }
                                } else {
                                    l.assign_preserving_type(l << r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* RSHIFTASSIGN */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    bool old{ctx->set_create_if_not_exists(true)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    this_->lval_->reset_primary();
                    if(r.is_undefined_ref()) {
                    } else {
                        if(l.is_undefined_ref()) {
                        } else {
                            bool excepted{false};
                            runtime_error er{{}, {}, {}};
                            try {
                                str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                                if(l.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                                } else if(r.is_class_ref()) {
                                    binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                                }
                                if(binops != nullptr) {
                                    auto add_it{binops->find(">>")};
                                    auto assign_it{binops->find("=")};
                                    if(add_it != binops->end() && assign_it != binops->end()) {
                                        valbox res{add_it->second(l, r)};
                                        assign_it->second(l, res);
                                    } else {
                                        l.assign_preserving_type(l >> r);
                                    }
                                } else {
                                    l.assign_preserving_type(l >> r);
                                }
                            } catch (runtime_error const &e) {
                                er = e;
                                excepted = true;
                            } catch (std::exception const &e) {
                                er = runtime_error(this_->line_, this_->col_, e.what());
                                excepted = true;
                            } catch (...) {
                                er = runtime_error(this_->line_, this_->col_, "unknown error");
                                excepted = true;
                            }
                            if(excepted) {
                                throw er;
                            }
                        }
                    }
                    return l;
                },
                /* INCREMENT */ nullptr,
                /* DECREMENT */ nullptr,
                /* LSHIFT */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("<<")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                res = l << r;
                            }
                        } else {
                            res = l << r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* RSHIFT */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find(">>")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                res = l >> r;
                            }
                        } else {
                            res = l >> r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* DOTDOT */ nullptr,
                /* STARSTAR */ nullptr,
                /* QUESTIONCOLON */ nullptr,
                /* QUESTIONQUESTION */ nullptr,
                /* BITNOT */ nullptr,
                /* XOR */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("^")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                res = l ^ r;
                            }
                        } else {
                            res = l ^ r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* CIRCUMFLEXCIRCUMFLEX */ nullptr,
                /* BITOR */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("|")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                res = l | r;
                            }
                        } else {
                            res = l | r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* OR */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                    if(l.is_class_ref()) {
                        unops = &(ctx->rt_interface()->get_object_services(l.class_name())->unops);
                    }
                    if(unops != nullptr) {
                        auto it{unops->find("(bool)")};
                        if(it != unops->end()) {
                            if(it->second(l).cast_to_bool()) {
                                return true;
                            }
                        } else {
                            if(l.cast_to_bool()) { return true; }
                        }
                    } else {
                        if(l.cast_to_bool()) { return true; }
                    }
                    {
                        valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                        str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                        if(r.is_class_ref()) {
                            unops = &(ctx->rt_interface()->get_object_services(r.class_name())->unops);
                        }
                        if(unops != nullptr) {
                            auto it{unops->find("(bool)")};
                            if(it != unops->end()) {
                                return it->second(r).cast_to_bool();
                            }
                        }
                        return r.cast_to_bool();
                    }
                },
                /* BITAND */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr)};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        str_map_t<std::function<valbox(valbox &, valbox &)>> const *binops{nullptr};
                        if(l.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(l.class_name())->binops);
                        } else if(r.is_class_ref()) {
                            binops = &(ctx->rt_interface()->get_object_services(r.class_name())->binops);
                        }
                        if(binops != nullptr) {
                            auto it{binops->find("&")};
                            if(it != binops->end()) {
                                res = it->second(l, r);
                            } else {
                                res = l & r;
                            }
                        } else {
                            res = l & r;
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    return res;
                },
                /* AND */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *) -> valbox {
                    bool old{ctx->set_create_if_not_exists(false)};
                    shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                    if(l.is_class_ref()) {
                        unops = &(ctx->rt_interface()->get_object_services(l.class_name())->unops);
                    }
                    if(unops != nullptr) {
                        auto it{unops->find("(bool)")};
                        if(it != unops->end()) {
                            if(!it->second(l).cast_to_bool()) {
                                return false;
                            }
                        }
                    }
                    if(!l.cast_to_bool()) { return false; }
                    {
                        valbox r{this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                        str_map_t<std::function<valbox(valbox &)>> const *unops{nullptr};
                        if(r.is_class_ref()) {
                            unops = &(ctx->rt_interface()->get_object_services(r.class_name())->unops);
                        }
                        if(unops != nullptr) {
                            auto it{unops->find("(bool)")};
                            if(it != unops->end()) {
                                return it->second(r).cast_to_bool();
                            }
                        }
                        return r.cast_to_bool();
                    }
                },
                /* QUESTION */ nullptr,
                /* COLON */ nullptr,
                /* COLONCOLON */ nullptr,
                /* SEMICOLON */ nullptr,
                /* LPAREN */ nullptr,
                /* RPAREN */ nullptr,
                /* LBRACKET */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type, valbox *dotlptr) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                    if(dotlptr) {
                        *dotlptr = l;
                    }
                    valbox r{valbox_no_initialize::dont_do_it};
                    {
                        bool old{ctx->set_create_if_not_exists(false)};
                        shut_on_destroy sod{[&]() { ctx->set_create_if_not_exists(old); }};
                        r = this_->rval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref();
                    }
                    valbox::type lt{l.val_or_pointed_type()};
                    valbox::type rt{r.val_or_pointed_type()};
                    bool excepted{false};
                    runtime_error er{{}, {}, {}};
                    try {
                        if(ctx->create_if_not_exists()) {
                            if(l.is_undefined()) {
                                if(valbox::is_numeric_type(rt)) {
                                    l.become_array();
                                } else if(valbox::is_any_string_type(rt)) {
                                    l.become_object();
                                }
                            }
                            res = l[r];
                        } else {
                            if(valbox::is_any_string_type(rt)) {
                                if(lt != valbox::type::OBJECT) {
                                    throw runtime_error{this_->line(), this_->col(), "wrong left operand for indirection"};
                                }
                                if(l.as_object().end() == l.as_object().find(r.cast_to_string())) {
                                    throw runtime_error{this_->line(), this_->col(), "field not found"};
                                }
                                res = l[r];
                            } else if(valbox::is_numeric_type(rt)) {
                                if(lt == valbox::type::ARRAY) {
                                    auto const &ar{l.as_array()};
                                    auto indx{r.cast_to_u64()};
                                    if(indx < ar.size()) {
                                        res = ar.at(indx);
                                    }
                                } else {
                                    res = l[r];
                                }
                            }
                        }
                    } catch (runtime_error const &e) {
                        er = e;
                        excepted = true;
                    } catch (std::exception const &e) {
                        er = runtime_error(this_->line_, this_->col_, e.what());
                        excepted = true;
                    } catch (...) {
                        er = runtime_error(this_->line_, this_->col_, "unknown error");
                        excepted = true;
                    }
                    if(excepted) {
                        throw er;
                    }
                    res.set_pointed(l);
                    return res;
                },
                /* RBRACKET */ nullptr,
                /* LCURLY */ nullptr,
                /* RCURLY */ nullptr,
                /* COMMA */ nullptr,
                /* DOT */
                [](binop_expression *this_, execution_context *ctx, eval_caller_type caller_type, valbox *dotlptr) -> valbox {
                    valbox res{valbox_no_initialize::dont_do_it};
                    if(this_->lval_->symbol() == "this") {
                        if(ctx->is_inside_function()) {
                            throw runtime_error{this_->line_, this_->col_, "function can not have own context"};
                        }
                        this_->sym_ = this_->rval_->symbol();
                        if(ctx->create_if_not_exists()) {
                            res = (*ctx->self_fields())[this_->sym_];
                        } else {
                            auto it{ctx->self_fields()->find(this_->sym_)};
                            if(it != ctx->self_fields()->end()) {
                                res = it->second;
                            } else {
                                res = valbox{valbox_no_initialize::dont_do_it};
                            }
                        }
                    } else {
                        valbox l{this_->lval_->eval(ctx, eval_caller_type::no_matter, nullptr).deref()};
                        if(dotlptr) {
                            *dotlptr = l;
                        }
                        if(l.is_class_ref()) {
                            res = ctx->find_method(l.ref_class_name(), this_->rval_->symbol(),
                                                   this_->rval_->line(), this_->rval_->col());
                            if(res.is_undefined_ref()) {
                                valbox found_fn{};
                                execution_context::obj_type objtyp{execution_context::obj_type::unknown};
                                if(ctx->find_func(this_->rval_->symbol(), found_fn, objtyp)) {
                                    res = found_fn;
                                    this_->sym_ = this_->rval_->symbol();
                                } else {
                                    res = valbox{valbox_no_initialize::dont_do_it};
                                }
                            } else {
                                this_->sym_ = this_->rval_->symbol();
                            }
                        } else {
                            if(caller_type == eval_caller_type::func_call) {
                                bool func_found{false};
                                if(l.is_object_ref()) {
                                    auto it{l.as_object().find(this_->rval_->symbol())};
                                    if(
                                        it != l.as_object().end() &&
                                        it->second.is_func_ref()
                                    ) {
                                        func_found = true;
                                        res = it->second;
                                    }
                                }
                                if(!func_found) {
                                    valbox found_fn{};
                                    execution_context::obj_type objtyp{execution_context::obj_type::unknown};
                                    if(ctx->find_func(this_->rval_->symbol(), found_fn, objtyp)) {
                                        res = found_fn;
                                        this_->sym_ = this_->rval_->symbol();
                                    } else {
                                        res = valbox{valbox_no_initialize::dont_do_it};
                                    }
                                }
                            } else if(l.is_undefined_ref()) {
                                if(!this_->rval_->is_symbolic()) {
                                    throw runtime_error{this_->line(), this_->col(), "wrong expression from right of \".\" operator"};
                                }
                                if(ctx->create_if_not_exists()) {
                                    l.become_object();
                                    res = l[this_->rval_->symbol()];
                                    res.set_pointed(l);
                                    this_->sym_ = this_->rval_->symbol();
                                }
                            } else if(l.is_object_ref()) {
                                if(!this_->rval_->is_symbolic()) {
                                    throw runtime_error{this_->line(), this_->col(), "wrong expression from right of \".\" operator"};
                                }
                                if(ctx->create_if_not_exists()) {
                                    res = l[this_->rval_->symbol()];
                                    res.set_pointed(l);
                                    this_->sym_ = this_->rval_->symbol();
                                } else {
                                    valbox::object_t &o{l.as_object()};
                                    auto it{o.find(this_->rval_->symbol())};
                                    if(it != o.end()) {
                                        res = it->second;
                                        res.set_pointed(l);
                                        this_->sym_ = this_->rval_->symbol();
                                    }
                                }
                            } else {
                                throw runtime_error{this_->line(), this_->col(), "wrong operand"};
                            }
                        }
                    }
                    return res;
                },
                /* FUNCCALL */ nullptr,
                /* TYPECAST */ nullptr,
                /* ENDOFFILE */ nullptr,
            };
            if(primary()) {
                std::shared_lock l{primary_val_mtp_};
                return primary_val_;
            }
            auto &&fn{ops[static_cast<std::size_t>(opcode_)]};
            if(fn == nullptr) {
                throw runtime_error{line(), col(), "unsupported operation"};
            }
            valbox res{fn(this, ctx, caller_type, dotlptr)};
            if(lval_->primary() && rval_->primary()) {
                std::unique_lock l{primary_val_mtp_};
                primary_val_ = res;
                primary_ = true;
            }
            return res;
        }

        std::string const &symbol() const & override {
            return sym_;
        }

    private:
        token::type opcode_{};
        expr_ptr lval_{};
        expr_ptr rval_{};
        std::string sym_{};
        mutable shared_mutex primary_val_mtp_{};
        valbox primary_val_{};
        std::atomic<bool> primary_{false};
    };

}
