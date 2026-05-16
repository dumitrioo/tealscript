#pragma once

#include "inc/commondefs.hpp"
#include "inc/file_util.hpp"
#include "inc/str_util.hpp"

#include "tealscript_util.hpp"
#include "tealscript_value.hpp"
#include "tealscript_token.hpp"
#include "tealscript_lexer.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_statement.hpp"

namespace teal {

    class parser {
    public:
        void add_token(token const &tk) {
            tokens_.push_back(tk);
        }

        void reset() {
            tokens_.clear();
            tokens_pos_ = -1;
        }

        json parse() {
            json res{};
            tokens_pos_ = tokens_.empty() ? -1 : 0;
            while(!get_token(0).is_eof()) {
                auto s{get_top_level_statement()};
                if(!s.is_object()) { break; }
                res.push_back(std::move(s));
            }
            return res;
        }

    private:
        std::list<std::string> loops_nesting_{};

        json get_top_level_statement() {
            json res{};
            res = get_func_def_statement();
            if(res.is_object()) { return res; }
            res = get_cell_def_statement();
            if(res.is_object()) { return res; }
            res = get_cell_instantiation_statement();
            if(res.is_object()) { return res; }
            throw compilation_error{
                get_token(0).line(),
                get_token(0).col(),
                "invalid statement"
            };
        }

        json get_statement() {
            json res{};
            res = get_empty_statement();
            if(res.is_object()) { return res; }
            res = get_if_else_statement();
            if(res.is_object()) { return res; }
            res = get_throw_statement();
            if(res.is_object()) { return res; }
            res = get_try_block_statement();
            if(res.is_object()) { return res; }
            res = get_while_statement();
            if(res.is_object()) { return res; }
            res = get_for_statement();
            if(res.is_object()) { return res; }
            res = get_compound_statement();
            if(res.is_object()) { return res; }
            res = get_return_statement();
            if(res.is_object()) { return res; }
            res = get_break_statement();
            if(res.is_object()) { return res; }
            res = get_continue_statement();
            if(res.is_object()) { return res; }
            res = get_expression_statement();
            if(res.is_object()) { return res; }
            throw compilation_error{
                get_token(0).line(),
                get_token(0).col(),
                "invalid statement"
            };
        }

        json get_func_def_statement() {
            json res{};
            if(
                get_token(0).lexem() == L"function" &&
                get_token(1).is_id() &&
                get_token(2).type_is(token::type::LPAREN)
            ) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                res["type"] = "statement";
                res["subtype"] = "function_definition";
                res["content"]["function_name"] = get_token(1).lexem();
                increment_pos(2);
                res["content"]["arg_names"] = get_function_def_args_list()["content"];
                res["content"]["function_body"] = get_statement();
            }
            return res;
        }

        json get_function_def_args_list() {
            json res{};
            if(get_token(0).type_is(token::type::LPAREN)) {
                check_eof();
                increment_pos();
                res["type"] = "function_arguments_names";
                res["content"].become_array();
                while(get_token(0).type_is(token::type::IDENTIFIER)) {
                    res["content"].push_back(get_token(0).lexem());
                    increment_pos();
                    check_eof();
                    if(!get_token(0).type_is(token::type::COMMA)) {
                        break;
                    }
                    increment_pos();
                    check_eof();
                }
                check_eof();
                if(!get_token(0).type_is(token::type::RPAREN)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid function arguments list"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_cell_def_statement() {
            json res{};
            if(
                get_token(0).is_id() && get_token(1).type_is(token::type::LPAREN)
            ) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                res["type"] = "statement";
                res["subtype"] = "cell_definition";
                res["content"]["cell_name"] = get_token(0).lexem();
                increment_pos();
                res["content"]["arg_names"] = get_cell_def_args_list()["content"];
                res["content"]["cell_body"] = get_cell_def_body();
            }
            return res;
        }

        json get_cell_def_args_list() {
            json res{};
            if(get_token(0).type_is(token::type::LPAREN)) {
                increment_pos();
                res["type"] = "cell_arguments_names";
                res["content"].become_array();
                while(get_token(0).type_is(token::type::IDENTIFIER)) {
                    res["content"].push_back(get_token(0).lexem());
                    increment_pos();
                    if(!get_token(0).type_is(token::type::COMMA)) {
                        break;
                    }
                    increment_pos();
                }
                check_eof();
                if(!get_token(0).type_is(token::type::RPAREN)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid cell arguments list"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_cell_def_body() {
            return get_statement();
        }

        json get_cell_instantiation_statement() {
            json res{};
            if(
                get_token(0).is_id() &&
                get_token(1).is_id() &&
                get_token(2).type_is(token::type::LPAREN)
            ) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                auto ct{get_token(0).lexem()};
                auto cn{get_token(1).lexem()};
                res["type"] = "statement";
                res["subtype"] = "cell_inst";
                res["content"]["cell_flags"].push_back("regular");
                res["content"]["cell_type"] = ct;
                res["content"]["cell_name"] = cn;
                increment_pos(2);
                res["content"]["args"] =  get_cell_instance_args_list();
                check_eof();
                if(get_token(0).type_is(token::type::CHAR_LITERAL)) {
                    res["content"]["cell_flags"].push_back("output");
                    res["content"]["output_name"] = get_token(0).lexem();
                    increment_pos();
                }
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            } else if(
                get_token(0).is_id() && get_token(0).lexem() == L"extern" &&
                get_token(1).type_is(token::type::CHAR_LITERAL) &&
                get_token(2).is_id()
            ) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                auto rn{get_token(1).lexem()};
                auto cn{get_token(2).lexem()};
                res["type"] = "statement";
                res["subtype"] = "cell_inst";
                res["content"]["cell_flags"].push_back("extern");
                res["content"]["remote_name"] = rn;
                res["content"]["cell_name"] = cn;
                increment_pos(3);
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            } else if(get_token(0).type_is(token::type::CHAR_LITERAL) && get_token(1).is_id()) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                auto in{get_token(0).lexem()};
                auto cn{get_token(1).lexem()};
                res["type"] = "statement";
                res["subtype"] = "cell_inst";
                res["content"]["cell_flags"].push_back("input");
                res["content"]["input_name"] = in;
                res["content"]["cell_name"] = cn;
                increment_pos(2);
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            }
            return res;
        }

        void check_eof(int indx = 0) const {
            if(get_token(indx).is_eof()) {
                throw compilation_error{get_token(indx).line(), get_token(indx).col(), "unexpected end"};
            }
        }

        json get_cell_instance_args_list() {
            json res{};
            check_eof();
            if(get_token(0).type_is(token::type::LPAREN)) {
                res["type"] = "cell_actual_arguments";
                increment_pos();
                check_eof();
                while(get_token(0).type_is_not(token::type::RPAREN)) {
                    res["content"].push_back(get_expr());
                    if(!get_token(0).type_is(token::type::COMMA)) {
                        break;
                    }
                    increment_pos();
                }
                check_eof();
                if(!get_token(0).type_is(token::type::RPAREN)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid cell arguments list"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_compound_statement() {
            json res{};
            if(get_token(0).type_is(token::type::LCURLY)) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                res["type"] = "statement";
                res["subtype"] = "compound";
                increment_pos();
                check_eof();
                if(get_token(0).type_is(token::type::RCURLY)) {
                    res["content"].become_array();
                    increment_pos();
                } else {
                    bool closed{false};
                    for(auto sbst{get_statement()}; sbst.is_object(); sbst = get_statement()) {
                        res["content"].push_back(std::move(sbst));
                        check_eof();
                        if(get_token(0).type_is(token::type::RCURLY)) {
                            increment_pos();
                            closed = true;
                            break;
                        }
                    }
                    if(!closed && get_token(0).type_is(token::type::RCURLY)) {
                        increment_pos();
                    }
                }
            }
            return res;
        }

        json get_try_block_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"try") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                increment_pos();
                json trystat{get_statement()};
                if(trystat.is_object()) {
                    check_eof();
                    if(get_token(0).is_id() && get_token(0).lexem() == L"catch") {
                        increment_pos();
                        check_eof();
                        if(get_token(0).lexem() == L"(") {
                            increment_pos();
                            check_eof();
                            json catchexpr{};
                            if(get_token(0).is_id()) {
                                catchexpr = get_expr();
                                if(
                                    catchexpr.is_object() &&
                                    catchexpr["subtype"].as_string() == "identifier"
                                ) {
                                    check_eof();
                                    if(get_token(0).lexem() == L")") {
                                        increment_pos();
                                    } else {
                                        throw compilation_error{
                                            get_token(0).line(),
                                            get_token(0).col(),
                                            "invalid \"try\": \")\" expected"
                                        };
                                    }
                                } else {
                                    throw compilation_error{
                                        get_token(0).line(),
                                        get_token(0).col(),
                                        "invalid \"try\": invalid catch expression, identifier expected"
                                    };
                                }
                            } else if(get_token(0).lexem() == L")") {
                                increment_pos();
                            } else {
                                throw compilation_error{
                                    get_token(0).line(),
                                    get_token(0).col(),
                                    "invalid \"try\": identifier expected"
                                };
                            }
                            json catchstat{get_statement()};
                            if(catchstat.is_object()) {
                                res["type"] = "statement";
                                res["subtype"] = "try";
                                res["content"]["try_statement"] = std::move(trystat);
                                res["content"]["catch_expression"] = std::move(catchexpr);
                                res["content"]["catch_statement"] = std::move(catchstat);
                            } else {
                                throw compilation_error{
                                    get_token(0).line(),
                                    get_token(0).col(),
                                    "invalid \"try\": catch statement expected"
                                };
                            }
                        } else {
                            throw compilation_error{
                                get_token(0).line(),
                                get_token(0).col(),
                                "invalid \"try\": \"(\" expected"
                            };
                        }
                    } else {
                        throw compilation_error{
                            get_token(0).line(),
                            get_token(0).col(),
                            "invalid \"try\": \"catch\" expected"
                        };
                    }
                } else {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid \"try\": expected statement"
                    };
                }
            }
            return res;
        }

        json get_if_else_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"if") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                increment_pos();
                json ifexpr{get_expr()};
                if(ifexpr.is_object()) {
                    json ifstat{get_statement()};
                    json elsestat{};
                    if(ifstat.is_object()) {
                        check_eof();
                        if(get_token(0).is_id() && get_token(0).lexem() == L"else") {
                            increment_pos();
                            elsestat = get_statement();
                        }
                        res["type"] = "statement";
                        res["subtype"] = "if";
                        res["content"]["cond"] = ifexpr;
                        res["content"]["then_statement"] = ifstat;
                        res["content"]["else_statement"] = elsestat;
                    } else {
                        throw compilation_error{
                            get_token(0).line(),
                            get_token(0).col(),
                            "invalid \"if\": true-branch statement expected"
                        };
                    }
                } else {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid \"if\": condition expected"
                    };
                }
            }
            return res;
        }

        json get_while_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"while") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                loops_nesting_.push_front("while");
                teal::shut_on_destroy pop_nest{
                    [&]() { loops_nesting_.pop_front(); }
                };
                increment_pos();
                check_eof();
                if(get_token(0).type_is(token::type::LPAREN)) {
                    increment_pos();
                    json expr{get_expr()};
                    if(expr.is_object()) {
                        check_eof();
                        if(get_token(0).type_is(token::type::RPAREN)) {
                            increment_pos();
                            auto stmt{get_statement()};
                            if(stmt.is_object()) {
                                res["type"] = "statement";
                                res["subtype"] = "while";
                                res["content"]["cond"] = expr;
                                res["content"]["statement"] = stmt;
                            } else {
                                throw compilation_error{
                                    get_token(0).line(),
                                    get_token(0).col(),
                                    "invalid \"while\": statement expected"
                                };
                            }
                        } else {
                            throw compilation_error{
                                get_token(0).line(),
                                get_token(0).col(),
                                "invalid \"while\": condition expected"
                            };
                        }
                    } else {
                        throw compilation_error{
                            get_token(0).line(),
                            get_token(0).col(),
                            "invalid \"while\": condition expected"
                        };
                    }
                } else {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid \"while\": condition expected"
                    };
                }
            }
            return res;
        }

        json get_for_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"for") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                loops_nesting_.push_front("for");
                teal::shut_on_destroy pop_nest{[&]() {
                    loops_nesting_.pop_front();
                }};
                increment_pos();
                check_eof();
                if(get_token(0).type_is(token::type::LPAREN)) {
                    increment_pos();
                    json init_expr{};
                    check_eof();
                    if(get_token(0).type_is(token::type::SEMICOLON)) {
                        init_expr["type"] = "expression";
                        init_expr["subtype"] = "literal";
                        init_expr["literal"] = "bool";
                        init_expr["content"] = false;
                        init_expr["loc"]["line"] = get_token(0).line();
                        init_expr["loc"]["col"] = get_token(0).col();
                    } else {
                        init_expr = get_expr();
                    }
                    if(init_expr.is_object()) {
                        check_eof();
                        if(get_token(0).type_is(token::type::SEMICOLON)) {
                            increment_pos();
                            json cond_expr{};
                            check_eof();
                            if(get_token(0).type_is(token::type::SEMICOLON)) {
                                cond_expr["type"] = "expression";
                                cond_expr["subtype"] = "literal";
                                cond_expr["literal"] = "bool";
                                cond_expr["content"] = true;
                                cond_expr["loc"]["line"] = get_token(0).line();
                                cond_expr["loc"]["col"] = get_token(0).col();
                            } else {
                                cond_expr = get_expr();
                            }
                            if(cond_expr.is_object()) {
                                check_eof();
                                if(get_token(0).type_is(token::type::SEMICOLON)) {
                                    increment_pos();
                                    json incr_expr{};
                                    check_eof();
                                    if(get_token(0).type_is(token::type::SEMICOLON)) {
                                        incr_expr["type"] = "expression";
                                        incr_expr["subtype"] = "literal";
                                        incr_expr["literal"] = "bool";
                                        incr_expr["content"] = false;
                                        incr_expr["loc"]["line"] = get_token(0).line();
                                        incr_expr["loc"]["col"] = get_token(0).col();
                                    } else {
                                        incr_expr = get_expr();
                                    }
                                    if(incr_expr.is_object()) {
                                        check_eof();
                                        if(get_token(0).type_is(token::type::RPAREN)) {
                                            increment_pos();
                                            auto stmt{get_statement()};
                                            if(stmt.is_object()) {
                                                res["type"] = "statement";
                                                res["subtype"] = "for";
                                                res["content"]["init"] = init_expr;
                                                res["content"]["cond"] = cond_expr;
                                                res["content"]["incr"] = incr_expr;
                                                res["content"]["statement"] = stmt;
                                            } else {
                                                throw compilation_error{
                                                    get_token(0).line(),
                                                    get_token(0).col(),
                                                    "invalid \"for\": statement expected"
                                                };
                                            }
                                        } else {
                                            throw compilation_error{
                                                get_token(0).line(),
                                                get_token(0).col(),
                                                "invalid \"for\": ')' expected"
                                            };
                                        }
                                    } else {
                                        throw compilation_error{
                                            get_token(0).line(),
                                            get_token(0).col(),
                                            "invalid \"for\": expression expected"
                                        };
                                    }
                                } else {
                                    throw compilation_error{
                                        get_token(0).line(),
                                        get_token(0).col(),
                                        "invalid \"for\": ';' expected"
                                    };
                                }
                            } else {
                                throw compilation_error{
                                    get_token(0).line(),
                                    get_token(0).col(),
                                    "invalid \"for\": expression expected"
                                };
                            }
                        } else {
                            throw compilation_error{
                                get_token(0).line(),
                                get_token(0).col(),
                                "invalid \"for\": ';' expected"
                            };
                        }
                    } else {
                        throw compilation_error{
                            get_token(0).line(),
                            get_token(0).col(),
                            "invalid \"for\": expression expected"
                        };
                    }
                } else {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid \"for\": '(' expected"
                    };
                }
            }
            return res;
        }

        json get_return_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"return") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                increment_pos();
                res["type"] = "statement";
                res["subtype"] = "return";
                res["content"] = get_expr();
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_throw_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"throw") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                increment_pos();
                res["type"] = "statement";
                res["subtype"] = "throw";
                json ct{get_expr()};
                if(ct.is_null()) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "invalid expression to be thrown"
                    };
                }
                res["content"] = ct;
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_continue_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"continue") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                if(loops_nesting_.empty()) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\"continue\" outside loop"
                    };
                }
                increment_pos();
                res["type"] = "statement";
                res["subtype"] = "continue";
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_break_statement() {
            json res{};
            if(get_token(0).is_id() && get_token(0).lexem() == L"break") {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                if(loops_nesting_.empty()) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\"break\" outside loop"
                    };
                }
                increment_pos();
                res["type"] = "statement";
                res["subtype"] = "break";
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            }
            return res;
        }

        json get_expression_statement() {
            json res{};
            std::int64_t l{get_token(0).line()};
            std::int64_t c{get_token(0).col()};
            auto expr{get_expr()};
            if(expr.is_object()) {
                res["loc"]["line"] = l;
                res["loc"]["col"] = c;
                res["type"] = "statement";
                res["subtype"] = "expression";
                res["content"] = expr;
                check_eof();
                if(get_token(0).type_is_not(token::type::SEMICOLON)) {
                    throw compilation_error{
                        get_token(0).line(),
                        get_token(0).col(),
                        "\";\" expected"
                    };
                }
                increment_pos();
            } else {
                if(get_token(0).type_is(token::type::SEMICOLON)) {
                    increment_pos();
                }
            }
            return res;
        }

        json get_empty_statement() {
            json res{};
            if(get_token(0).type_is(token::type::SEMICOLON)) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                res["type"] = "statement";
                res["subtype"] = "empty";
                increment_pos();
            }
            return res;
        }

        json get_expr() {
            return get_prio_17();
        }

        json get_prio_17() {
            return get_prio_16();
        }

        json get_prio_16() {
            json res{get_prio_15()};
            token const &tk{get_token(0)};
            if(
                tk.type_is(token::type::ASSIGN) ||
                tk.type_is(token::type::ADDASSIGN) ||
                tk.type_is(token::type::SUBASSIGN) ||
                tk.type_is(token::type::MULASSIGN) ||
                tk.type_is(token::type::DIVASSIGN) ||
                tk.type_is(token::type::MODASSIGN) ||
                tk.type_is(token::type::LSHIFTASSIGN) ||
                tk.type_is(token::type::RSHIFTASSIGN) ||
                tk.type_is(token::type::BITANDASSIGN) ||
                tk.type_is(token::type::BITORASSIGN) ||
                tk.type_is(token::type::XORASSIGN)
            ) {
                json over_res{};
                over_res["loc"]["line"] = get_token(0).line();
                over_res["loc"]["col"] = get_token(0).col();
                increment_pos();
                over_res["type"] = "expression";
                over_res["subtype"] = "binop";
                over_res["content"]["operation"] = tk.tktype_str();
                over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                over_res["content"]["left"] = std::move(res);
                res = std::move(over_res);
                res["content"]["right"] = get_prio_16();
            } else if(tk.type_is(token::type::QUESTION)) {
                json over_res{};
                over_res["loc"]["line"] = get_token(0).line();
                over_res["loc"]["col"] = get_token(0).col();
                increment_pos();
                over_res["type"] = "expression";
                over_res["subtype"] = "ternop";
                over_res["content"]["operation"] = tk.tktype_str();
                over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                over_res["content"]["condition"] = std::move(res);
                over_res["content"]["true_expr"] = get_prio_16();
                token const &must_colon_tk{get_token(0)};
                if(must_colon_tk.type_is(token::type::COLON)) {
                    increment_pos();
                    over_res["content"]["false_expr"] = get_prio_16();
                    res = std::move(over_res);
                } else {
                    throw compilation_error{get_token(0).line(), get_token(0).col(),
                        std::string{"\":\" expected, got \""} + teal::str_util::to_utf8(get_token(0).lexem()) + "\""
                    };
                }
            }
            return res;
        }

        json get_prio_15() {
            json res{get_prio_14()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::OR)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    increment_pos();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    res["content"]["right"] = get_prio_14();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_14() {
            json res{get_prio_13()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::AND)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_13();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_13() {
            json res{get_prio_12()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::BITOR)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    increment_pos();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    res["content"]["right"] = get_prio_12();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_12() {
            json res{get_prio_11()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::XOR)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    increment_pos();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    res["content"]["right"] = get_prio_11();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_11() {
            json res{get_prio_10()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::BITAND)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    increment_pos();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    res["content"]["right"] = get_prio_10();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_10() {
            json res{get_prio_9()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::EQUAL) || tk.type_is(token::type::NOTEQUAL)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_9();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_9() {
            json res{get_prio_8()};
            while(true) {
                token const &tk{get_token(0)};
                if(
                    tk.type_is(token::type::LESS) ||
                    tk.type_is(token::type::LESSEQUAL) ||
                    tk.type_is(token::type::GREATER) ||
                    tk.type_is(token::type::GREATEREQUAL)
                ) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_8();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_8() {
            json res{get_prio_7()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::SPACESHIP)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_7();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_7() {
            json res{get_prio_6()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::LSHIFT) || tk.type_is(token::type::RSHIFT)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_6();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_6() {
            json res{get_prio_5()};
            while(true) {
                token const &tk{get_token(0)};
                if(tk.type_is(token::type::PLUS) || tk.type_is(token::type::MINUS)) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_5();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_5() {
            json res{get_prio_4()};
            while(true) {
                token const &tk{get_token(0)};
                if(
                    tk.type_is(token::type::STAR) ||
                    tk.type_is(token::type::SLASH) ||
                    tk.type_is(token::type::MOD)
                ) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                    res["content"]["right"] = get_prio_4();
                } else {
                    break;
                }
            }
            return res;
        }

        json get_prio_4() {
            return get_prio_3();
        }

        json get_prio_3() {
            token const &tk{get_token(0)};
            json res{};
            if(
                tk.type_is(token::type::MINUS) ||
                tk.type_is(token::type::PLUS) ||
                tk.type_is(token::type::NOT) ||
                tk.type_is(token::type::BITNOT) ||
                tk.type_is(token::type::INCREMENT) ||
                tk.type_is(token::type::DECREMENT)
            ) {
                res["loc"]["line"] = get_token(0).line();
                res["loc"]["col"] = get_token(0).col();
                res["type"] = "expression";
                res["subtype"] = "prefix";
                res["content"]["operation"] = tk.tktype_str();
                res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                increment_pos();
                res["content"]["operand"] = get_prio_3();
            } else if(
                tk.type_is(token::type::LPAREN) &&
                get_token(1).is_id() && valbox::is_type(str_util::to_utf8(get_token(1).lexem())) &&
                get_token(2).type_is(token::type::RPAREN)
            ) {
                res["loc"]["line"] = get_token(1).line();
                res["loc"]["col"] = get_token(1).col();
                res["type"] = "expression";
                res["subtype"] = "prefix";
                res["content"]["operation"] = get_token(1).lexem();
                res["content"]["oper_enum"] = static_cast<int>(token::type::TYPECAST);
                increment_pos(3);
                res["content"]["operand"] = get_prio_3();
            } else {
                return get_prio_2();
            }
            return res;
        }

        json get_prio_2() {
            json res{get_prio_1()};
            bool cont{true};
            while(cont) {
                cont = false;

                token const &tk{get_token(0)};
                if(
                    tk.type_is(token::type::INCREMENT) ||
                    tk.type_is(token::type::DECREMENT)
                ) {
                    json over_res{};
                    over_res["loc"]["line"] = get_token(0).line();
                    over_res["loc"]["col"] = get_token(0).col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "postfix";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["operand"] = std::move(res);
                    res = std::move(over_res);
                    increment_pos();
                } else if(tk.type_is(token::type::LPAREN)) {
                    cont = true;
                    json args{};
                    increment_pos();
                    if(get_token(0).type_is(token::type::RPAREN)) {
                        increment_pos();
                    } else {
                        while(true) {
                            json exptr{get_expr()};
                            if(exptr.is_object()) {
                                args.push_back(std::move(exptr));
                                if(get_token(0).type_is(token::type::RPAREN)) {
                                    increment_pos();
                                    break;
                                } else if(get_token(0).type_is_not(token::type::COMMA)) {
                                    throw compilation_error{get_token(0).line(), get_token(0).col(), "invalid function arguments"};
                                }
                                increment_pos();
                            } else {
                                if(get_token(0).type_is_not(token::type::RPAREN)) {
                                    throw compilation_error{get_token(0).line(), get_token(0).col(), "invalid function call"};
                                }
                                increment_pos();
                            }
                        }
                    }
                    json over_res{};
                    over_res["loc"]["line"] = tk.line();
                    over_res["loc"]["col"] = tk.col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "func_call";
                    over_res["content"]["func"] = res;
                    over_res["content"]["args"] = args;
                    res = std::move(over_res);
                } else if(tk.type_is(token::type::LBRACKET)) {
                    cont = true;
                    token const &tk{get_token(0)};
                    increment_pos();
                    json indx_expr{get_expr()};
                    if(indx_expr.is_object()) {
                        if(get_token(0).type_is(token::type::RBRACKET)) {
                            increment_pos();
                        } else {
                            throw compilation_error{get_token(0).line(), get_token(0).col(), "invalid indirection"};
                        }
                    } else {
                        throw compilation_error{get_token(0).line(), get_token(0).col(), "invalid indirection"};
                    }
                    json over_res{};
                    over_res["loc"]["line"] = tk.line();
                    over_res["loc"]["col"] = tk.col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    over_res["content"]["right"] = std::move(indx_expr);
                    res = std::move(over_res);
                } else if(
                    tk.type_is(token::type::DOT)
                ) {
                    cont = true;
                    token const &tk{get_token(0)};
                    increment_pos();
                    json over_res{};
                    over_res["loc"]["line"] = tk.line();
                    over_res["loc"]["col"] = tk.col();
                    over_res["type"] = "expression";
                    over_res["subtype"] = "binop";
                    over_res["content"]["operation"] = tk.tktype_str();
                    over_res["content"]["oper_enum"] = static_cast<int>(tk.tktype());
                    over_res["content"]["left"] = std::move(res);
                    res = std::move(over_res);
                    res["content"]["right"] = get_prio_1();
                }
            }
            return res;
        }

        json get_prio_1() {
            return get_primary();
        }

        json get_primary() {
            token const &tk{get_token(0)};
            if(tk.type_is(token::type::FP_LITERAL) || tk.type_is(token::type::FP_EXP_LITERAL)) {
                json res{};
                increment_pos();
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "flt";
                res["content"] = std::stold(teal::str_util::to_utf8(tk.lexem()));
                return res;
            } else if(tk.type_is(token::type::INT_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "int";
                std::string msg{};
                bool bam{false};
                try {
                    res["content"] = teal::str_util::atoi(teal::str_util::to_utf8(tk.lexem()), 10, true);
                } catch (std::exception const &e) {
                    msg = e.what();
                    bam = true;
                }
                if(bam) {
                    throw compilation_error{tk.line(), tk.col(), msg};
                }
                return res;
            } else if(tk.type_is(token::type::HEX_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "hex";
                std::string msg{};
                bool bam{false};
                try {
                    res["content"] = teal::str_util::atoui(teal::str_util::to_utf8(tk.lexem()), 16, true);
                } catch (std::exception const &e) {
                    msg = e.what();
                    bam = true;
                }
                if(bam) {
                    throw compilation_error{tk.line(), tk.col(), msg};
                }
                return res;
            } else if(tk.type_is(token::type::OCT_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "oct";
                std::string msg{};
                bool bam{false};
                try {
                    res["content"] = teal::str_util::atoui(teal::str_util::to_utf8(tk.lexem()), 8, true);
                } catch (std::exception const &e) {
                    msg = e.what();
                    bam = true;
                }
                if(bam) {
                    throw compilation_error{tk.line(), tk.col(), msg};
                }
                return res;
            } else if(tk.type_is(token::type::BIN_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "bin";
                std::string msg{};
                bool bam{false};
                try {
                    res["content"] = teal::str_util::atoui(teal::str_util::to_utf8(tk.lexem()), 2, true);
                } catch (std::exception const &e) {
                    msg = e.what();
                    bam = true;
                }
                if(bam) {
                    throw compilation_error{tk.line(), tk.col(), msg};
                }
                return res;
            } else if(tk.type_is(token::type::STRING_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "str";
                res["content"] = teal::str_util::to_utf8(tk.lexem());
                return res;
            } else if(tk.type_is(token::type::CHAR_LITERAL)) {
                increment_pos();
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "chr";
                res["content"] = teal::str_util::to_utf8(tk.lexem());
                return res;
            } else if(tk.type_is(token::type::IDENTIFIER)) {
                if(!expression_part(tk.lexem())) {
                    throw compilation_error{
                        tk.line(),
                        tk.col(),
                        std::string{"invalid expression: \""} +
                            teal::str_util::to_utf8(tk.lexem()) +
                            "\" cannot be a part of expression"
                    };
                }

                increment_pos();
                if(tk.lexem() == L"true") {
                    json res{};
                    res["loc"]["line"] = tk.line();
                    res["loc"]["col"] = tk.col();
                    res["type"] = "expression";
                    res["subtype"] = "literal";
                    res["literal"] = "bool";
                    res["content"] = true;
                    return res;
                } else if(tk.lexem() == L"false") {
                    json res{};
                    res["loc"]["line"] = tk.line();
                    res["loc"]["col"] = tk.col();
                    res["type"] = "expression";
                    res["subtype"] = "literal";
                    res["literal"] = "bool";
                    res["content"] = false;
                    return res;
                } else if(tk.lexem() == L"undefined") {
                    json res{};
                    res["loc"]["line"] = tk.line();
                    res["loc"]["col"] = tk.col();
                    res["type"] = "expression";
                    res["subtype"] = "literal";
                    res["literal"] = "undefined";
                    res["content"].clear();
                    return res;
                } else {
                    json res{};
                    res["loc"]["line"] = tk.line();
                    res["loc"]["col"] = tk.col();
                    res["type"] = "expression";
                    res["subtype"] = "identifier";
                    res["content"] = tk.lexem();
                    return res;
                }
            } else if(tk.type_is(token::type::LPAREN)) {
                increment_pos();
                json res{get_expr()};
                if(get_token(0).type_is_not(token::type::RPAREN)) {
                    throw compilation_error{get_token(0).line(), get_token(0).col(), "expected parentesis"};
                }
                increment_pos();
                return res;
            } else if(tk.type_is(token::type::LBRACKET)) {
                json res{};
                res["loc"]["line"] = tk.line();
                res["loc"]["col"] = tk.col();
                res["type"] = "expression";
                res["subtype"] = "literal";
                res["literal"] = "array";
                res["content"].become_array();
                increment_pos();
                while(get_token(0).type_is_not(token::type::RBRACKET)) {
                    json itm{get_expr()};
                    if(!itm.is_object()) {
                        throw compilation_error{get_token(0).line(), get_token(0).col(), "expression expected in array initialization sequence"};
                    }
                    res["content"].push_back(itm);
                    if(get_token(0).type_is(token::type::COMMA)) {
                        increment_pos();
                    }
                }
                if(get_token(0).type_is_not(token::type::RBRACKET)) {
                    throw compilation_error{get_token(0).line(), get_token(0).col(), "expected parentesis"};
                }
                increment_pos();
                return res;
            }
            return {};
        }

        token const &get_token(std::int64_t offs) const {
            std::size_t indx{(std::size_t)(tokens_pos_ + offs)};
            if(indx < tokens_.size()) {
                return tokens_[indx];
            }
            return eof_;
        }

        void increment_pos(std::int64_t n = 1) {
            tokens_pos_ += n;
        }

    private:
        static inline token const eof_{};
        std::int64_t tokens_pos_{-1};
        std::vector<token> tokens_{};
    };

}
