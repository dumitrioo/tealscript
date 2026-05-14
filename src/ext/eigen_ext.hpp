#pragma once

#include "../inc/commondefs.hpp"
#include "../inc/eigen/Eigen/Core"
#include "../inc/eigen/Eigen/Dense"
#include "../inc/eigen/Eigen/Eigen"

#include "../tealscript_value.hpp"
#include "../tealscript_util.hpp"
#include "../tealscript_interfaces.hpp"

namespace teal {

    namespace detail {

        static std::optional<Eigen::MatrixXd> mb_construct_from(valbox const &vb) {
            if(vb.is_mat4_ref()) {
                Eigen::MatrixXd res;
                res.resize(4, 4);
                for(int r{}; r < 4; ++r) {
                    for(int c{}; c < 4; ++c) {
                        res(r, c) = vb.as_mat4()[r][c];
                    }
                }
                return res;
            }
            return {};
        }

    }

    class eigen_ext: public extension_interface {
    public:
        eigen_ext() = default;
        ~eigen_ext() {
            unregister_runtime();
        }
        eigen_ext(eigen_ext const &) = delete;
        eigen_ext &operator=(eigen_ext const &) = delete;
        eigen_ext(eigen_ext &&) = delete;
        eigen_ext &operator=(eigen_ext &&) = delete;

        void register_runtime(runtime_interface *rt) override {
            std::unique_lock l{rt_mtp_};
            if(rt_ != nullptr) { return; }
            rt_ = rt;
            if(rt_ == nullptr) { return; }

            rt->add_function("matrix", TEALFUN(args) {
                Eigen::MatrixXd res;
                if(args.size() > 0) {
                    if(args.size() == 1 && args[0].is_array_ref()) {
                        for(auto &&v: args[0].as_array()) { res << v.cast_to_double(); }
                    } else if(args.size() == 2 && (args[0].is_any_int_number()) && (args[1].is_any_int_number())) {
                        res.resize(args[0].cast_to_int(), args[1].cast_to_int());
                    } else if(
                        args.size() == 3 &&
                        args[0].is_numeric() &&
                        args[1].is_numeric() &&
                        args[2].is_mat4_ref()
                    ) {
                        res.resize(args[0].cast_to_int(), args[1].cast_to_int());
                        for(int r{}; r < args[0].cast_to_int(); ++r) {
                            for(int c{}; c < args[1].cast_to_int(); ++c) {
                                res(r, c) = args[2].as_mat4()[r][c];
                            }
                        }
                    } else if(
                        args.size() == 3 &&
                        args[0].is_numeric() &&
                        args[1].is_numeric() &&
                        args[2].is_array_ref()
                    ) {
                        res.resize(args[0].cast_to_int(), args[1].cast_to_int());
                        for(int r{}; r < args[0].cast_to_int(); ++r) {
                            for(int c{}; c < args[1].cast_to_int(); ++c) {
                                int ai{r * args[1].cast_to_int() + c};
                                if((int)args[2].as_array().size() > ai) {
                                    res(r, c) = args[2].as_array().at(ai).cast_to_double();
                                }
                            }
                        }
                    } else {
                        throw std::runtime_error{"invalid matrix initialization"};
                    }
                }
                return teal::valbox{std::move(res), "matrix"};
            });

            rt->add_object_binary_operation("matrix", "=",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix") {
                        if(r.class_name() == "matrix") {
                            l.as_class<Eigen::MatrixXd>() = r.as_class<Eigen::MatrixXd>();
                        } else {
                            if(std::optional<Eigen::MatrixXd> omr{detail::mb_construct_from(r)}) {
                                l.as_class<Eigen::MatrixXd>() = *omr;
                            }
                        }
                    } else {
                        l.assign(r);
                    }
                    return l;
                }
            );

            rt->add_object_binary_operation("matrix", "*",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix" && r.class_name() == "matrix") {
                        Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() * r.as_class<Eigen::MatrixXd>()};
                        return teal::valbox{std::move(mr), "matrix"};
                    } else if(l.class_name() == "matrix") {
                        if(r.is_numeric()) {
                            Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() * r.cast_to_double()};
                            return teal::valbox{mr, "matrix"};
                        } else if(auto ortsw{detail::mb_construct_from(r)}) {
                            Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() * *ortsw};
                            return teal::valbox{mr, "matrix"};
                        }
                    } else if(r.class_name() == "matrix") {
                        if(l.is_numeric()) {
                            Eigen::MatrixXd mr{l.cast_to_double() * r.as_class<Eigen::MatrixXd>()};
                            return teal::valbox{mr, "matrix"};
                        } else if(auto oltsw{detail::mb_construct_from(l)}) {
                            Eigen::MatrixXd mr{*oltsw * r.as_class<Eigen::MatrixXd>()};
                            return teal::valbox{mr, "matrix"};
                        }
                    }
                    throw std::runtime_error{"invalid operands"};
                }
            );

            rt->add_object_binary_operation("matrix", "/",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix" && r.is_numeric()) {
                        Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() / r.cast_to_double()};
                        return teal::valbox{mr, "matrix"};
                    }
                    throw std::runtime_error{"invalid operands"};
                }
            );

            rt->add_object_binary_operation("matrix", "-",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix" && r.class_name() == "matrix") {
                        Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() - r.as_class<Eigen::MatrixXd>()};
                        return teal::valbox{mr, "matrix"};
                    } else if(l.class_name() == "matrix") {
                        auto rm{detail::mb_construct_from(r)};
                        if(rm) {
                            Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() - *rm};
                            return teal::valbox{mr, "matrix"};
                        }
                    } else if(r.class_name() == "matrix") {
                        auto lm{detail::mb_construct_from(l)};
                        if(lm) {
                            Eigen::MatrixXd mr{*lm - l.as_class<Eigen::MatrixXd>()};
                            return teal::valbox{mr, "matrix"};
                        }
                    }
                    throw std::runtime_error{"invalid operands"};
                }
            );

            rt->add_object_unary_operation("matrix", "-",
                [](valbox &v) -> valbox {
                    if(v.class_name() == "matrix") {
                        Eigen::MatrixXd mr{-v.as_class<Eigen::MatrixXd>()};
                        return teal::valbox{mr, "matrix"};
                    }
                    throw std::runtime_error{"invalid operands"};
                }
            );

            rt->add_object_binary_operation("matrix", "+",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix" && r.class_name() == "matrix") {
                        Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() + r.as_class<Eigen::MatrixXd>()};
                        return teal::valbox{mr, "matrix"};
                    } else if(l.class_name() == "matrix") {
                        auto rm{detail::mb_construct_from(r)};
                        if(rm) {
                            Eigen::MatrixXd mr{l.as_class<Eigen::MatrixXd>() + *rm};
                            return teal::valbox{mr, "matrix"};
                        }
                    } else if(r.class_name() == "matrix") {
                        auto lm{detail::mb_construct_from(l)};
                        if(lm) {
                            Eigen::MatrixXd mr{*lm + l.as_class<Eigen::MatrixXd>()};
                            return teal::valbox{mr, "matrix"};
                        }
                    }
                    throw std::runtime_error{"invalid operands"};
                }
            );

            rt->add_object_unary_operation("matrix", "(bool)",
                [](valbox &v) -> valbox {
                    if(v.class_name() == "matrix") {
                        bool res{false};
                        for(int r{}; r < v.as_class<Eigen::MatrixXd>().rows(); ++r) {
                            for(int c{}; c < v.as_class<Eigen::MatrixXd>().cols(); ++c) {
                                if(v.as_class<Eigen::MatrixXd>()(r, c) != 0.0) {
                                    res = true;
                                    break;
                                }
                            }
                        }
                        return res;
                    }
                    throw std::runtime_error{"invalid operand"};
                }
            );

            rt->add_object_unary_operation("matrix", "+",
                [](valbox &v) -> valbox {
                    if(v.class_name() == "matrix") {
                        return teal::valbox{v.as_class<Eigen::MatrixXd>(), "matrix"};
                    }
                    throw std::runtime_error{"invalid operand"};
                }
            );

            rt->add_object_binary_operation("matrix", "==",
                [](valbox &l, valbox &r) -> valbox {
                    if(l.class_name() == "matrix" && r.class_name() == "matrix") {
                        return l.as_class<Eigen::MatrixXd>() == r.as_class<Eigen::MatrixXd>();
                    } else {
                        if(l.class_name() == "matrix") {
                            if(auto ortsw{detail::mb_construct_from(r)}) {
                                return l.as_class<Eigen::MatrixXd>() == *ortsw;
                            }
                        } else if(r.class_name() == "matrix") {
                            if(auto oltsw{detail::mb_construct_from(l)}) {
                                return *oltsw == r.as_class<Eigen::MatrixXd>();
                            }
                        }
                    }
                    return false;
                }
            );

            rt->add_object_serializer("matrix",
                [](valbox const &v) -> std::optional<std::string> {
                    if(v.is_class_ref() && v.class_name() == "matrix") {
                        serializer ser{};
                        ser << "matrix" << v.as_class<Eigen::MatrixXd>().rows() << v.as_class<Eigen::MatrixXd>().cols();
                        for(int r{}; r < v.as_class<Eigen::MatrixXd>().rows(); ++r) {
                            for(int c{}; c < v.as_class<Eigen::MatrixXd>().cols(); ++c) {
                                ser << v.as_class<Eigen::MatrixXd>()(r, c);
                            }
                        }
                        return data_to_base64_str(ser.data_vec());
                    }
                    throw std::runtime_error{"invalid operand"};
                }
            );

            rt->add_object_deserializer("matrix",
                [](std::string const &class_name, std::string const &serial_form) -> valbox {
                    if(class_name == "matrix") {
                        std::vector<std::uint8_t> vd{base64_str_to_data(serial_form)};
                        serial_reader sr{vd.data(), vd.size()};
                        auto it{sr.begin()};
                        if(it->as_string() == "matrix") {
                            int numr{(int)(++it)->as_number()};
                            int numc{(int)(++it)->as_number()};
                            Eigen::MatrixXd res;
                            res.resize(numr, numc);
                            for(int r{}; r < numr; ++r) {
                                for(int c{}; c < numc; ++c) {
                                    res(r, c) = (++it)->as_fpnum<double>();
                                }
                            }
                            return teal::valbox{res, "matrix"};
                        }
                    }
                    throw std::runtime_error{"invalid operand"};
                }
            );

            rt->add_object_stringifier("matrix",
                [](valbox const &v) -> valbox {
                    if(v.class_name() == "matrix") {
                        auto m{v.as_class<Eigen::MatrixXd>()};
                        std::stringstream ss{};
                        ss << "eigen_mat{";
                        for(int r{}; r < v.as_class<Eigen::MatrixXd>().rows(); ++r) {
                            ss << "{";
                            for(int c{}; c < v.as_class<Eigen::MatrixXd>().cols(); ++c) {
                                ss << v.as_class<Eigen::MatrixXd>()(r, c);
                                if(c + 1 < v.as_class<Eigen::MatrixXd>().cols()) ss << ",";
                            }
                            ss << "}";
                            if(r + 1 < v.as_class<Eigen::MatrixXd>().rows()) ss << ",";
                        }
                        ss << "}";
                        return ss.str();
                    }
                    throw std::runtime_error{"invalid operand"};
                }
            );

            rt->add_function("zero_matrix", TEALFUN(args) {
                Eigen::MatrixXd res;
                if(args.size() > 0) {
                    if(args.size() == 1) {
                        res = Eigen::MatrixXd::Zero(args[0].cast_to_int(), args[0].cast_to_int());
                    } else if(args.size() == 2 ) {
                        res = Eigen::MatrixXd::Zero(args[0].cast_to_int(), args[1].cast_to_int());
                    } else {
                        throw std::runtime_error{"invalid matrix initialization arguments"};
                    }
                }
                return teal::valbox{std::move(res), "matrix"};
            });

            rt->add_function("identity_matrix", TEALFUN(args) {
                Eigen::MatrixXd res;
                if(args.size() > 0) {
                    if(args.size() == 1) {
                        res = Eigen::MatrixXd::Identity(args[0].cast_to_int(), args[0].cast_to_int());
                    } else if(args.size() == 2 ) {
                        res = Eigen::MatrixXd::Identity(args[0].cast_to_int(), args[1].cast_to_int());
                    } else {
                        throw std::runtime_error{"invalid matrix initialization arguments"};
                    }
                }
                return teal::valbox{std::move(res), "matrix"};
            });

            rt->add_method("matrix", "at", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args.size() == 2) {
                    return teal::valbox{
                        &TEALTHIS(args, Eigen::MatrixXd)(args[1].cast_to_int(), 0),
                        valbox::type::DOUBLE
                    };
                } else if(args.size() == 3) {
                    return teal::valbox{
                        &TEALTHIS(args, Eigen::MatrixXd)(args[1].cast_to_int(), args[2].cast_to_int()),
                        valbox::type::DOUBLE
                    };
                }
                return teal::valbox{valbox_no_initialize::dont_do_it};
            });

            rt->add_method("matrix", "resize", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args.size() == 2) {
                    TEALTHIS(args, Eigen::MatrixXd).resize(args[1].cast_to_int(), 1);
                    return args[0];
                } else if(args.size() == 3) {
                    TEALTHIS(args, Eigen::MatrixXd).resize(args[1].cast_to_int(), args[2].cast_to_int());
                    return args[0];
                }
                return teal::valbox{valbox_no_initialize::dont_do_it};
            });

            rt->add_method("matrix", "inversed", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                Eigen::MatrixXd res{TEALTHIS(args, Eigen::MatrixXd)};
                res = res.inverse();
                return teal::valbox{res, "matrix"};
            });

            rt->add_method("matrix", "inverse", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                TEALTHIS(args, Eigen::MatrixXd) = TEALTHIS(args, Eigen::MatrixXd).inverse();
                return args[0];
            });

            rt->add_method("matrix", "transposed", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                Eigen::MatrixXd res{TEALTHIS(args, Eigen::MatrixXd).transpose()};
                return teal::valbox{res, "matrix"};
            });

            rt->add_method("matrix", "transpose", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                TEALTHIS(args, Eigen::MatrixXd).transposeInPlace();
                return args[0];
            });

            rt->add_method("matrix", "rows", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return TEALTHIS(args, Eigen::MatrixXd).rows();
            });

            rt->add_method("matrix", "cols", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return TEALTHIS(args, Eigen::MatrixXd).cols();
            });

            rt->add_method("matrix", "max_coeff", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return TEALTHIS(args, Eigen::MatrixXd).maxCoeff();
            });


            rt->add_function("solveRiccatiArimotoPotter", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 4)

                const Eigen::MatrixXd &A{TEALCLASSARG(args, 0, Eigen::MatrixXd)};
                const Eigen::MatrixXd &B{TEALCLASSARG(args, 1, Eigen::MatrixXd)};
                const Eigen::MatrixXd &Q{TEALCLASSARG(args, 2, Eigen::MatrixXd)};
                const Eigen::MatrixXd &R{TEALCLASSARG(args, 3, Eigen::MatrixXd)};

                Eigen::MatrixXd P;

                const uint dim_x = A.rows();
                const uint dim_u = B.cols();

                // set Hamilton matrix
                Eigen::MatrixXd Ham = Eigen::MatrixXd::Zero(2 * dim_x, 2 * dim_x);
                Ham << A, -B * R.inverse() * B.transpose(), -Q, -A.transpose();

                // calc eigenvalues and eigenvectors
                Eigen::EigenSolver<Eigen::MatrixXd> Eigs(Ham);

                // check eigen values
                // std::cout << "eigen values：\n" << Eigs.eigenvalues() << std::endl;
                // std::cout << "eigen vectors：\n" << Eigs.eigenvectors() << std::endl;

                // extract stable eigenvectors into 'eigvec'
                Eigen::MatrixXcd eigvec = Eigen::MatrixXcd::Zero(2 * dim_x, dim_x);
                int j = 0;
                for (int i = 0; i < 2 * dim_x; ++i) {
                    if (Eigs.eigenvalues()[i].real() < 0.) {
                        eigvec.col(j) = Eigs.eigenvectors().block(0, i, 2 * dim_x, 1);
                        ++j;
                    }
                }

                // calc P with stable eigen vector matrix
                Eigen::MatrixXcd Vs_1, Vs_2;
                Vs_1 = eigvec.block(0, 0, dim_x, dim_x);
                Vs_2 = eigvec.block(dim_x, 0, dim_x, dim_x);
                P = (Vs_2 * Vs_1.inverse()).real();

                return teal::valbox{P, "matrix"};
            });


        }

        void unregister_runtime() override {
            std::unique_lock l{rt_mtp_};
            if(rt_ == nullptr) {
                return;
            }

            // rt_->remove_object_services("timestamp");
            // rt_->remove_function("timestamp");
            // rt_->remove_function("gmtimestamp");
            // rt_->remove_function("steady_clock");
            // rt_->remove_function("time");
            // rt_->remove_function("gmtime");
            // rt_->remove_method("timestamp", "year");
            // rt_->remove_method("timestamp", "month");
            // rt_->remove_method("timestamp", "day");
            // rt_->remove_method("timestamp", "weekday");
            // rt_->remove_method("timestamp", "hour");
            // rt_->remove_method("timestamp", "min");
            // rt_->remove_method("timestamp", "sec");
            // rt_->remove_method("timestamp", "seconds");
            // rt_->remove_method("timestamp", "fseconds");
            // rt_->remove_method("timestamp", "as_iso_8601");
            // rt_->remove_method("timestamp", "as_gmt_iso_8601");
            // rt_->remove_method("timestamp", "from_string");

            rt_ = nullptr;
        }

    private:
        shared_mutex rt_mtp_{};
        runtime_interface *rt_{nullptr};
    };

}
