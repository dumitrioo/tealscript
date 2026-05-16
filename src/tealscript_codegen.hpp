#pragma once

#include "inc/commondefs.hpp"
#include "inc/str_util.hpp"
#include "inc/net/net_utils.hpp"
#include "inc/net/url.hpp"
#include "inc/json.hpp"

#include "tealscript_util.hpp"
#include "tealscript_token.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_statement.hpp"
#include "tealscript_cells.hpp"

namespace teal {

    namespace detail {

        static bool is_ident_start(std::int64_t c) {
            return teal::str_util::fltr<std::wstring>::isalpha(c) || c == '_' || c == '$';
        }

        static bool is_ident_tail(std::int64_t c) {
            return is_ident_start(c) || teal::str_util::fltr<std::wstring>::isdigit(c);
        }

        static bool is_valid_dentifier(std::string const &id) {
            if(id.empty()) { return false; }
            if(!is_ident_start(id[0])) { return false; }
            for(size_t i{1}; i < id.size(); ++i) { if(!is_ident_tail(id[i])) { return false; } }
            return true;
        }

    }

    class code_generator {
    public:
        void chop(
            json const &ast
            , str_map_t<std::shared_ptr<input_cell>> &input_cells
            , str_map_t<std::string> &input_names_to_instances_mapping
            , str_map_t<worker_cell_definition_info> &worker_cells_templates
            , str_map_t<std::shared_ptr<worker_cell_instance>> &worker_cells
            , str_map_t<statement_ptr> &worker_bodies
            , str_map_t<function_definition> &user_functions
            , str_map_t<valbox> const &global_functions_dictionary
            , str_map_t<std::shared_ptr<extern_cell>> &extern_cells
        ) {
            for(std::size_t i = 0; i < ast.size(); ++i) {
                json const &cur{ast[i]};
                if(cur["subtype"].as_string() == "cell_definition") {
                    json const &cur_cnt{cur["content"]};
                    if(worker_cells_templates.find(cur_cnt["cell_name"].as_string()) != worker_cells_templates.end()) {
                        throw compilation_error{
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number(),
                            cur_cnt["cell_name"].as_string() + ": duplicated cell template identifier"
                        };
                    }
                    statement_ptr cb{chop_statement(cur_cnt["cell_body"])};
                    worker_cell_definition_info wc{};
                    wc.set_loc(cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number());
                    for(std::size_t ai = 0; ai < cur_cnt["arg_names"].size(); ++ai) {
                        wc.set_arg_name(ai, cur_cnt["arg_names"][ai].as_string());
                    }
                    wc.set_type_name(cur_cnt["cell_name"].as_string());
                    cb->skip_frame_creation();
                    worker_bodies[wc.type_name()] = cb;
                    worker_cells_templates[wc.type_name()] = wc;
                } else if(cur["subtype"].as_string() == "cell_inst") {
                    json const &cur_cnt{cur["content"]};
                    if(array_contains_str(cur_cnt["cell_flags"], "input")) {
                        std::string cnm{cur_cnt["cell_name"].as_string()};
                        std::string inm{cur_cnt["input_name"].as_string()};
                        if(
                            worker_cells.find(cnm) != worker_cells.end() ||
                            input_cells.find(cnm) != input_cells.end() ||
                            extern_cells.find(cnm) != extern_cells.end()
                        ) {
                            throw compilation_error{
                                cur["loc"]["line"].try_as_number(),
                                cur["loc"]["col"].try_as_number(),
                                cnm + ": duplicated cell identifier"
                            };
                        }
                        std::shared_ptr<input_cell> ic_ptr{std::make_shared<input_cell>()};
                        input_cells[cnm] = ic_ptr;
                        ic_ptr->set_input_name(inm);
                        ic_ptr->set_inst_name(cnm);
                        input_names_to_instances_mapping[inm] = cnm;
                        ic_ptr->set_loc(cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number());
                    } else if(array_contains_str(cur_cnt["cell_flags"], "extern")) {
                        std::string cnm{cur_cnt["cell_name"].as_string()};
                        std::string rnm{cur_cnt["remote_name"].as_string()};
                        url u{rnm};
                        if(!u.valid()) {
                            throw compilation_error{cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number(),
                                rnm + ": external name should be a valid URL"};
                        }
                        std::string p{u.path()};
                        while(!p.empty() && p[0] == '/') { p = p.substr(1); }
                        if(!detail::is_valid_dentifier(p)) {
                            throw compilation_error{cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number(),
                                                    rnm + ": path must be a valid identifier after \"/\""};
                        }
                        if(u.scheme() != "tealscript") {
                            throw compilation_error{cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number(),
                                                    u.scheme() + ": URL scheme must be \"tealscript\""};
                        }
                        std::string remote_host{teal::net::ntop(teal::net::resolve(u.host()))};
                        if(remote_host.empty() || !u.port()) {
                            throw compilation_error{cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number(),
                                rnm + ": host and port must present in URL"};
                        }                        
                        if(
                            worker_cells.find(cnm) != worker_cells.end() ||
                            input_cells.find(cnm) != input_cells.end() ||
                            extern_cells.find(cnm) != extern_cells.end()
                        ) {
                            throw compilation_error{
                                cur["loc"]["line"].try_as_number(),
                                cur["loc"]["col"].try_as_number(),
                                cnm + ": duplicated cell identifier"
                            };
                        }
                        std::shared_ptr<extern_cell> ec_ptr{std::make_shared<extern_cell>()};
                        extern_cells[cnm] = ec_ptr;
                        ec_ptr->set_remote_var_name(p);
                        ec_ptr->set_remote_host(remote_host);
                        ec_ptr->set_url(u);
                        ec_ptr->set_inst_name(cnm);
                        ec_ptr->set_loc(
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number()
                        );
                    } else if(array_contains_str(cur_cnt["cell_flags"], "regular")) {
                        std::string cnm{cur_cnt["cell_name"].as_string()};
                        if(
                            worker_cells.find(cnm) != worker_cells.end() ||
                            input_cells.find(cnm) != input_cells.end() ||
                            extern_cells.find(cnm) != extern_cells.end()
                        ) {
                            throw compilation_error{
                                cur["loc"]["line"].try_as_number(),
                                cur["loc"]["col"].try_as_number(),
                                cnm + ": duplicated cell identifier"
                            };
                        }
                        std::shared_ptr<worker_cell_instance> wc_ptr{
                            std::make_shared<worker_cell_instance>()
                        };
                        worker_cells[cnm] = wc_ptr;
                        wc_ptr->set_loc(
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number()
                        );
                        wc_ptr->set_inst_name(cnm);
                        wc_ptr->set_type_name(cur_cnt["cell_type"].as_string());
                        if(cur_cnt["args"].key_exists("content")) {
                            for(std::size_t ai = 0; ai < cur_cnt["args"]["content"].size(); ++ai) {
                                json const &arg_cnt{cur_cnt["args"]["content"][ai]};
                                if(arg_cnt["subtype"].as_string() == "identifier") {
                                    wc_ptr->set_act_arg_source(ai, arg_cnt["content"].as_string());
                                } else {
                                    wc_ptr->set_act_arg_expr(ai, chop_expression(arg_cnt));
                                }
                            }
                        }
                        if(array_contains_str(cur_cnt["cell_flags"], "output")) {
                            wc_ptr->set_output_name(cur_cnt["output_name"].as_string());
                        }
                    } else {
                        throw compilation_error{
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number(),
                            "invalid cell instantiation"
                        };
                    }
                } else if(cur["subtype"].as_string() == "function_definition") {
                    json const &cur_cnt{cur["content"]};
                    std::string func_name{cur_cnt["function_name"].as_string()};
                    if(is_keyword(str_util::from_utf8(func_name))) {
                        throw compilation_error{
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number(),
                            std::string{"name \""} + func_name + "\" is a keyword"
                        };
                    }
                    if(
                        user_functions.find(func_name) != user_functions.end() ||
                        global_functions_dictionary.find(func_name) != global_functions_dictionary.end()
                    ) {
                        throw compilation_error{
                            cur["loc"]["line"].try_as_number(),
                            cur["loc"]["col"].try_as_number(),
                            func_name + ": symbol already exists"
                        };
                    }
                    statement_ptr cb{chop_statement(cur_cnt["function_body"])};
                    function_definition fn{};
                    fn.set_loc(cur["loc"]["line"].try_as_number(), cur["loc"]["col"].try_as_number());
                    fn.set_name(func_name);
                    fn.set_body(cb);
                    for(std::size_t ai = 0; ai < cur_cnt["arg_names"].size(); ++ai) {
                        fn.set_arg_name(ai, cur_cnt["arg_names"][ai].as_string());
                    }
                    user_functions[func_name] = std::move(fn);
                }
            }
        }

    private:
        static bool array_contains_str(json const &arr, std::string const &s) {
            if(!arr.is_array()) {
                return false;
            }
            for(std::size_t i{}; i < arr.size(); ++i) {
                if(arr[i].as_string() == s) {
                    return true;
                }
            }
            return false;
        }

        statement_ptr chop_statement(json const &ast) {
            statement_ptr res{};
            if(
                ast.is_object() &&
                ast.key_exists("type") &&
                ast["type"].as_string() == "statement"
            ) {
                if(ast["subtype"].as_string() == "compound") {
                    res = chop_compound_statement(ast);
                } else if(ast["subtype"].as_string() == "empty") {
                    res = std::make_shared<statement_empty>();
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "expression") {
                    res = chop_expression_statement(ast);
                } else if(ast["subtype"].as_string() == "while") {
                    res = std::make_shared<statement_while>(
                        chop_expression(ast["content"]["cond"]),
                        chop_statement(ast["content"]["statement"])
                    );
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "for") {
                    res = std::make_shared<statement_for>(
                        chop_expression(ast["content"]["init"]),
                        chop_expression(ast["content"]["cond"]),
                        chop_expression(ast["content"]["incr"]),
                        chop_statement(ast["content"]["statement"])
                    );
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "if") {
                    res = std::make_shared<statement_if_else>(
                        chop_expression(ast["content"]["cond"]),
                        chop_statement(ast["content"]["then_statement"]),
                        chop_statement(ast["content"]["else_statement"])
                    );
                } else if(ast["subtype"].as_string() == "throw") {
                    res = std::make_shared<statement_throw>(
                        chop_expression(ast["content"])
                    );
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "try") {
                    res = std::make_shared<statement_try_catch>(
                        chop_statement(ast["content"]["try_statement"]),
                        chop_expression(ast["content"]["catch_expression"]),
                        chop_statement(ast["content"]["catch_statement"])
                    );
                } else if(ast["subtype"].as_string() == "return") {
                    res = std::make_shared<statement_return>(
                        chop_expression(ast["content"])
                    );
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "break") {
                    res = std::make_shared<statement_break>();
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                } else if(ast["subtype"].as_string() == "continue") {
                    res = std::make_shared<statement_continue>();
                    res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
                }
            }
            return res;
        }

        statement_ptr chop_compound_statement(json const &ast) {
            statement_ptr res{std::make_shared<statement_compound>()};
            res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
            for(std::size_t i = 0; i < ast["content"].size(); ++i) {
                static_cast<statement_compound *>(res.get())->push_back(
                    chop_statement(ast["content"][i])
                );
            }
            if(static_cast<statement_compound *>(res.get())->num_substatements() == 0) {
                return std::make_shared<statement_empty>();
            } else if(static_cast<statement_compound *>(res.get())->num_substatements() == 1) {
                return static_cast<statement_compound *>(res.get())->get_tatement_at(0);
            }
            return res;
        }

        statement_ptr chop_expression_statement(json const &ast) {
            auto res {std::make_shared<statement_expr>(chop_expression(ast["content"]))};
            res->set_loc(ast["loc"]["line"].try_as_number(), ast["loc"]["col"].try_as_number());
            return res;
        }

        expr_ptr chop_expression(json const &ast) {
            static std::unordered_set<std::string> const hobi{"hex", "oct", "bin", "int"};

            struct frame {
                frame(
                    json const *ast_arg = nullptr,
                    expr_ptr res_arg = {},
                    expr_ptr buf1_arg = {},
                    expr_ptr buf2_arg = {},
                    size_t phase_arg = 0
                ):
                    ast{ast_arg},
                    res{res_arg},
                    buf1{buf1_arg},
                    buf2{buf2_arg},
                    phase{phase_arg}
                {}
                json const *ast{nullptr};
                expr_ptr res{};
                expr_ptr buf1{};
                expr_ptr buf2{};
                size_t phase{0};
            };
            frame stack_res{};
            std::deque<frame> stack{};
            stack.emplace_back(&ast);

            while(!stack.empty()) {
                if(stack.back().ast->is_null()) {
                    stack.back().res = std::make_shared<void_expression>();
                    stack_res = std::move(stack.back());
                    stack.pop_back();
                    continue;
                }
                if((*stack.back().ast)["subtype"].as_string() == "binop") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if(stack.back().phase == 0) {
                        if(cnt["left"].is_null() || cnt["right"].is_null()) {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                std::string{"invalid expression"}
                            };
                        }
                        stack.back().phase = 1;
                        stack.emplace_back(&cnt["left"]);
                        continue;
                    } else if(stack.back().phase == 1) {
                        stack.back().buf1 = stack_res.res;
                        stack.back().phase = 2;
                        stack.emplace_back(&cnt["right"]);
                        continue;
                    } else if(stack.back().phase == 2) {
                        stack.back().res = std::make_shared<binop_expression>(
                            static_cast<token::type>(cnt["oper_enum"].as_int()),
                            stack.back().buf1,
                            stack_res.res
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                } else if((*stack.back().ast)["subtype"].as_string() == "ternop") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if(stack.back().phase == 0) {
                        if(
                            cnt["condition"].is_null() ||
                            cnt["true_expr"].is_null() ||
                            cnt["false_expr"].is_null() ||
                            static_cast<token::type>(cnt["oper_enum"].as_int(-1)) != token::type::QUESTION
                        ) {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                std::string{"invalid expression"}
                            };
                        }
                        stack.back().phase = 1;
                        stack.emplace_back(&cnt["condition"]);
                        continue;
                    } else if(stack.back().phase == 1) {
                        stack.back().buf1 = stack_res.res;
                        stack.back().phase = 2;
                        stack.emplace_back(&cnt["true_expr"]);
                        continue;
                    } else if(stack.back().phase == 2) {
                        stack.back().buf2 = stack_res.res;
                        stack.back().phase = 3;
                        stack.emplace_back(&cnt["false_expr"]);
                        continue;
                    } else if(stack.back().phase == 3) {
                        stack.back().res = std::make_shared<ternop_expression>(
                            stack.back().buf1,
                            stack.back().buf2,
                            stack_res.res
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                } else if((*stack.back().ast)["subtype"].as_string() == "prefix") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if(stack.back().phase == 0) {
                        if(cnt["operand"].is_null() || cnt["oper_enum"].is_null()) {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                std::string{"invalid expression"}
                            };
                        }
                        stack.back().phase = 1;
                        stack.emplace_back(&cnt["operand"]);
                        continue;
                    } else if(stack.back().phase == 1) {
                        if(static_cast<token::type>(cnt["oper_enum"].as_int()) == token::type::TYPECAST) {
                            static const std::set<valbox::type> forbidden_casts{
                                valbox::type::POINTER,
                                valbox::type::CLASS,
                                valbox::type::FUNC,
                                valbox::type::VALBOX,
                            };
                            if(forbidden_casts.find(valbox::str_to_type(cnt["operation"].as_string())) != forbidden_casts.end()) {
                                throw compilation_error{
                                    (*stack.back().ast)["loc"]["line"].try_as_number(),
                                    (*stack.back().ast)["loc"]["col"].try_as_number(),
                                    cnt["operation"].as_string() + ": invalid type to convert to"
                                };
                            }
                            stack.back().res = std::make_shared<prefix_unop_expression>(
                                static_cast<token::type>(cnt["oper_enum"].as_int()),
                                stack_res.res,
                                cnt["operation"].as_string()
                            );
                        } else {
                            stack.back().res = std::make_shared<prefix_unop_expression>(
                                static_cast<token::type>(cnt["oper_enum"].as_int()),
                                stack_res.res
                            );
                        }
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                } else if((*stack.back().ast)["subtype"].as_string() == "postfix") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if(stack.back().phase == 0) {
                        if(cnt["operand"].is_null() || cnt["oper_enum"].is_null()) {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                std::string{"invalid expression"}
                            };
                        }
                        stack.back().phase = 1;
                        stack.emplace_back(&cnt["operand"]);
                        continue;
                    } else if(stack.back().phase == 1) {
                        stack.back().res = std::make_shared<postfix_unop_expression>(
                            stack_res.res,
                            static_cast<token::type>(cnt["oper_enum"].as_int())
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                } else if((*stack.back().ast)["subtype"].as_string() == "identifier") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    stack.back().res = std::make_shared<sym_expression>(cnt.as_string());
                    stack.back().res->set_loc(
                        (*stack.back().ast)["loc"]["line"].try_as_number(),
                        (*stack.back().ast)["loc"]["col"].try_as_number()
                    );
                    stack_res = std::move(stack.back());
                    stack.pop_back();
                } else if((*stack.back().ast)["subtype"].as_string() == "literal") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if((*stack.back().ast)["literal"].as_string() == "flt") {
                        stack.back().res = std::make_shared<primary_expression>(cnt.as_double());
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if(hobi.find((*stack.back().ast)["literal"].as_string()) != hobi.end()) {
                        stack.back().res = std::make_shared<primary_expression>(cnt.as_number());
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if((*stack.back().ast)["literal"].as_string() == "bool") {
                        stack.back().res = std::make_shared<primary_expression>(cnt.as_boolean());
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if((*stack.back().ast)["literal"].as_string() == "undefined") {
                        stack.back().res = std::make_shared<primary_expression>(
                            valbox{valbox_no_initialize::dont_do_it}
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if((*stack.back().ast)["literal"].as_string() == "str") {
                        stack.back().res = std::make_shared<primary_expression>(cnt.as_string());
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if((*stack.back().ast)["literal"].as_string() == "array") {
                        stack.back().res = std::make_shared<func_call_expression>(
                            std::make_shared<sym_expression>("array"),
                            func_call_args(cnt)
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    } else if((*stack.back().ast)["literal"].as_string() == "chr") {
                        std::wstring chr_str{teal::str_util::from_utf8(cnt.as_string())};
                        if(chr_str.size() == 1) {
                            if(chr_str[0] < 256) {
                                stack.back().res = std::make_shared<primary_expression>((char)chr_str[0]);
                            } else {
                                stack.back().res = std::make_shared<primary_expression>(chr_str[0]);
                            }
                        } else if(chr_str.size() > 1) {
                            uint32_t c{};
                            int pos{(int)(chr_str.size() - 1)};
                            for(size_t i{0}; i < 4 && pos >= 0; ++i) {
                                int cc{(std::uint8_t)chr_str[pos]};
                                c |= cc << (i * 8);
                                --pos;
                            }
                            stack.back().res = std::make_shared<primary_expression>((wchar_t)c);
                        } else {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                "invalid character"
                            };
                        }
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                } else if((*stack.back().ast)["subtype"].as_string() == "func_call") {
                    json const &cnt{(*stack.back().ast)["content"]};
                    if(stack.back().phase == 0) {
                        if(cnt["func"].is_null()) {
                            throw compilation_error{
                                (*stack.back().ast)["loc"]["line"].try_as_number(),
                                (*stack.back().ast)["loc"]["col"].try_as_number(),
                                std::string{"invalid expression"}
                            };
                        }
                        stack.back().phase = 1;
                        stack.emplace_back(&cnt["func"]);
                        continue;
                    } else if(stack.back().phase == 1) {
                        stack.back().res = std::make_shared<func_call_expression>(
                            stack_res.res,
                            func_call_args(cnt["args"])
                        );
                        stack.back().res->set_loc(
                            (*stack.back().ast)["loc"]["line"].try_as_number(),
                            (*stack.back().ast)["loc"]["col"].try_as_number()
                        );
                        stack_res = std::move(stack.back());
                        stack.pop_back();
                    }
                }
            }
            return stack_res.res;
        }

        std::vector<expr_ptr> func_call_args(json const &ast) {
            std::vector<expr_ptr> res{};
            for(std::size_t i = 0; i < ast.size(); ++i) {
                res.push_back(chop_expression(ast[i]));
            }
            return res;
        }
    };

}
