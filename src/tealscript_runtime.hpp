#pragma once

#include "inc/commondefs.hpp"
#include "inc/command_queue.hpp"
#include "inc/file_util.hpp"
#include "inc/str_util.hpp"
#include "inc/containers/concurrentqueue.h"
#include "inc/net/url.hpp"
#include "inc/net/net_utils.hpp"
#include "inc/sys_util.hpp"
#include "inc/base16.hpp"
#include "inc/base64.hpp"
#include "inc/base85.hpp"
#include "inc/so.hpp"
#include "inc/binned_allocator.hpp"
#include "tealscript_util.hpp"
#include "tealscript_token.hpp"
#include "tealscript_lexer.hpp"
#include "tealscript_parser.hpp"
#include "tealscript_expr.hpp"
#include "tealscript_statement.hpp"
#include "tealscript_cells.hpp"
#include "tealscript_codegen.hpp"
#include "tealscript_exec_ctx.hpp"
#include "tealscript_interfaces.hpp"
#include "tealscript_net.hpp"
#include "tealscript_console.hpp"

#include "ext/array_buffer_ext.hpp"
#include "ext/containers_ext.hpp"
#ifndef TEALSCRIPT_NO_EIGEN
#include "ext/eigen_ext.hpp"
#endif
#include "ext/file_ext.hpp"
#include "ext/crypto_ext.hpp"
#include "ext/cpu_ext.hpp"
#include "ext/rand_ext.hpp"
#include "ext/time_ext.hpp"
#include "ext/math_ext.hpp"
#include "ext/socket_ext.hpp"

#ifdef PLATFORM_WINDOWS
#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */
#endif

namespace teal {

    class runtime: public runtime_interface {
    public:
        runtime() {
            exctx_.set_runtime_interface(this);

            add_function("version_major", TEALFUN(/*args*/) { return version_major_; });
            add_function("version_minor", TEALFUN(/*args*/) { return version_minor_; });
            add_function("version_patch", TEALFUN(/*args*/) { return version_patch_; });
            add_function("version_string", TEALFUN(/*args*/) {
                return str_util::utoa<std::string>(version_major_) + "." +
                       str_util::utoa<std::string>(version_minor_) + "." +
                       str_util::utoa<std::string>(version_patch_);
            });

            // ordering
            add_var("LESS", -1);
            add_var("EQUAL", 0);
            add_var("GREATER", 1);

            add_var("EPERM", EPERM);     // 1   Operation not permitted
            add_var("ENOENT", ENOENT);   // 2   No such file or directory
            add_var("ESRCH", ESRCH);     // 3   No such process
            add_var("EINTR", EINTR);     // 4   Interrupted system call
            add_var("EIO", EIO);         // 5   I/O error
            add_var("ENXIO", ENXIO);     // 6   No such device or address
            add_var("E2BIG", E2BIG);     // 7   Argument list too long
            add_var("ENOEXEC", ENOEXEC); // 8   Exec format error
            add_var("EBADF", EBADF);     // 9   Bad file number
            add_var("ECHILD", ECHILD);   // 10  No child processes
            add_var("EAGAIN", EAGAIN);   // 11  Try again
            add_var("ENOMEM", ENOMEM);   // 12  Out of memory
            add_var("EACCES", EACCES);   // 13  Permission denied
            add_var("EFAULT", EFAULT);   // 14  Bad address
            add_var("ENOTBLK", ENOTBLK); // 15  Block device required
            add_var("EBUSY", EBUSY);     // 16  Device or resource busy
            add_var("EEXIST", EEXIST);   // 17  File exists
            add_var("EXDEV", EXDEV);     // 18  Cross-device link
            add_var("ENODEV", ENODEV);   // 19  No such device
            add_var("ENOTDIR", ENOTDIR); // 20  Not a directory
            add_var("EISDIR", EISDIR);   // 21  Is a directory
            add_var("EINVAL", EINVAL);   // 22  Invalid argument
            add_var("ENFILE", ENFILE);   // 23  File table overflow
            add_var("EMFILE", EMFILE);   // 24  Too many open files
            add_var("ENOTTY", ENOTTY);   // 25  Not a typewriter
            add_var("ETXTBSY", ETXTBSY); // 26  Text file busy
            add_var("EFBIG", EFBIG);     // 27  File too large
            add_var("ENOSPC", ENOSPC);   // 28  No space left on device
            add_var("ESPIPE", ESPIPE);   // 29  Illegal seek
            add_var("EROFS", EROFS);     // 30  Read-only file system
            add_var("EMLINK", EMLINK);   // 31  Too many links
            add_var("EPIPE", EPIPE);     // 32  Broken pipe
            add_var("EDOM", EDOM);       // 33  Math argument out of domain of func
            add_var("ERANGE", ERANGE);   // 34  Math result not representable

            add_function("last_error", TEALFUN() {
                return sys_util::last_error();
            });
            add_function("error_str", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return sys_util::error_str(args[0].cast_to_s64());
            });

            add_function("execute", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return std::system(args[0].cast_to_string().c_str());
            });
            add_function("load_from_file", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                auto fd{file_util::load_from_file(args[0].cast_to_string())};
                return std::string{fd.begin(), fd.end()};
            });
            add_function("save_to_file", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                return file_util::save_to_file(args[0].cast_to_string(), args[1].cast_to_string());
            });
            add_function("delete_filesystem_entry", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::delete_fs_entry(args[0].cast_to_string());
            });
            add_function("extract_file_dir", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::extract_file_dir(args[0].cast_to_string());
            });
            add_function("extract_file_name", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::extract_file_name(args[0].cast_to_string());
            });
            add_function("extract_file_ext", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::extract_file_ext(args[0].cast_to_string());
            });

            add_function("file_exists", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::file_exists(args[0].cast_to_string());
            });
            add_function("dir_exists", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return file_util::dir_exists(args[0].cast_to_string());
            });

            add_function("native_path_seperator", TEALFUN() {
                return file_util::native_path_separator<std::string>{}.sym();
            });

            add_function("native_path_seperator_str", TEALFUN() {
                return file_util::native_path_separator<std::string>{}.val();
            });

            add_function("temp_directory_path", TEALFUN() {
                return std::filesystem::temp_directory_path().string();
            });

            add_function("list_directory", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 2)
                bool recur{false};
                if(args.size() >= 2) {
                    recur = args[1].cast_to_bool();
                }
                valbox names{valbox_no_initialize::dont_do_it};
                names.become_array();
                file_util::for_dir_tree(
                    args[0].cast_to_string(),
                    [&](file_util::dir_entry const &de) {
                        names.as_array().push_back(de.full_path());
                        return true;
                    },
                    recur
                );
                return names;
            });

            add_function("errno", TEALFUN() { return sys_util::last_error(); });
            add_function("strerror", TEALFUN(args) {
                if(args.size() > 0) {
                    return std::string{strerror(args[0].cast_num_to_num<int>())};
                }
                return std::string{strerror(sys_util::last_error())};
            });

            array_buffer_ext_.register_runtime(this);
#ifndef TEALSCRIPT_NO_EIGEN
            eigen_ext_.register_runtime(this);
#endif
            math_ext_.register_runtime(this);
            time_ext_.register_runtime(this);
            crypt_.register_runtime(this);
            fpool_.register_runtime(this);
            perf_stat_.register_runtime(this);
            randlib_.register_runtime(this);
            sockext_.register_runtime(this);
            dict_ext_.register_runtime(this);

            add_var("console", valbox{&con_, "console"});
            add_method("console", "info", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1)
                std::vector<valbox> args1{args.begin() + 1, args.end()};
                TEALTHIS(args, console *)->info(args1);
                return {valbox_no_initialize::dont_do_it};
            });
            add_method("console", "log", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1) std::vector<valbox> args1{args.begin() + 1, args.end()}; TEALTHIS(args, console *)->log(args1); return {valbox_no_initialize::dont_do_it}; });
            add_method("console", "warn", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1) std::vector<valbox> args1{args.begin() + 1, args.end()}; TEALTHIS(args, console *)->warn(args1); return {valbox_no_initialize::dont_do_it}; });
            add_method("console", "debug", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1) std::vector<valbox> args1{args.begin() + 1, args.end()}; TEALTHIS(args, console *)->debug(args1); return {valbox_no_initialize::dont_do_it}; });
            add_method("console", "error", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1) std::vector<valbox> args1{args.begin() + 1, args.end()}; TEALTHIS(args, console *)->error(args1); return {valbox_no_initialize::dont_do_it}; });
            add_method("console", "print", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_GE(args, 1) std::vector<valbox> args1{args.begin() + 1, args.end()}; TEALTHIS(args, console *)->print(args1); return {valbox_no_initialize::dont_do_it}; });
            add_method("console", "flush", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) TEALTHIS(args, console *)->flush(); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "fixed", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) TEALTHIS(args, console *)->fixed(); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "scientific", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) TEALTHIS(args, console *)->scientific(); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "hexfloat", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) TEALTHIS(args, console *)->hexfloat(); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "defaultfloat", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) TEALTHIS(args, console *)->defaultfloat(); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "setprecision", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2) TEALTHIS(args, console *)->setprecision(args[1].cast_to_u64()); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "precision", TEALFUN(args) { return TEALTHIS(args, console *)->precision(); });
            add_method("console", "setw", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2) TEALTHIS(args, console *)->setw(args[1].cast_to_u64()); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "setfill", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2) TEALTHIS(args, console *)->setfill(args[1].cast_to_char()); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "fill", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return TEALTHIS(args, console *)->fill(); });
            add_method("console", "enable_colors", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2) TEALTHIS(args, console *)->enable_colors(args[1].cast_to_bool()); return valbox{valbox_no_initialize::dont_do_it}; });
            add_method("console", "colors_enabled", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return TEALTHIS(args, console *)->colors_enabled(); });
            add_method("console", "sync_stdio", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2) return TEALTHIS(args, console *)->setsync(args[1].cast_to_bool()); });
            add_function("print", TEALFUN(args) { con_.rawprint(args); return {valbox_no_initialize::dont_do_it}; });
            add_function("println", TEALFUN(args) { con_.println(args); return {valbox_no_initialize::dont_do_it}; });


            add_function("to_string", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                std::stringstream ss{};
                ss << args[0];
                return ss.str();
            });


            add_function("typeof", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                valbox::type t{args[0].deref().val_type()};
                return t == valbox::type::CLASS ? args[0].ref_class_name() : valbox::type_to_str(t);
            });


            add_function("sizeof", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return sizeof_func_(args[0]);
            });

            add_function("is_i64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_s64_ref(); });
            add_function("is_u64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_u64_ref(); });
            add_function("is_i32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_s32_ref(); });
            add_function("is_u32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_u32_ref(); });
            add_function("is_i16", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_s16_ref(); });
            add_function("is_u16", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_u16_ref(); });
            add_function("is_i8", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_s8_ref(); });
            add_function("is_u8", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_u8_ref(); });
            add_function("is_f32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_float_ref(); });
            add_function("is_f64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_double_ref(); });
            add_function("is_float", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_long_double_ref(); });
            add_function("is_char", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_char_ref(); });
            add_function("is_wchar", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_wchar_ref(); });
            add_function("is_bool", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_bool_ref(); });
            add_function("is_string", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_string_ref(); });
            add_function("is_wstring", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_wstring_ref(); });
            add_function("is_mat4", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_mat4_ref(); });
            add_function("is_vec4", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_vec4_ref(); });
            add_function("is_array", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_array_ref(); });
            add_function("is_object", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return args[0].is_object_ref(); });

            add_function("hton", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) { return args[0].as_char(); }
                if(args[0].is_bool_ref()) { return args[0].as_bool(); }
                if(args[0].is_s64_ref()) { return bit_util::swap_on_le<int64_t>{args[0].as_s64()}.val; }
                if(args[0].is_u64_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u64()}.val; }
                if(args[0].is_s32_ref()) { return bit_util::swap_on_le<int32_t>{args[0].as_s32()}.val; }
                if(args[0].is_u32_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u32()}.val; }
                if(args[0].is_s16_ref()) { return bit_util::swap_on_le<int16_t>{args[0].as_s16()}.val; }
                if(args[0].is_u16_ref()) { return bit_util::swap_on_le<uint16_t>{args[0].as_u16()}.val; }
                if(args[0].is_s8_ref()) { return bit_util::swap_on_le<int8_t>{args[0].as_s8()}.val; }
                if(args[0].is_u8_ref()) { return bit_util::swap_on_le<uint8_t>{args[0].as_u8()}.val; }
                if(args[0].is_float_ref()) { return bit_util::swap_on_le<float>{args[0].as_float()}.val; }
                if(args[0].is_double_ref()) { return bit_util::swap_on_le<double>{args[0].as_double()}.val; }
                if(args[0].is_long_double_ref()) { return bit_util::swap_on_le<long double>{args[0].as_long_double()}.val; }
                if(args[0].is_wchar_ref()) { return bit_util::swap_on_le<wchar_t>{args[0].as_wchar()}.val; }
                throw std::runtime_error{"invalid argument type"};
            });
            add_function("ntoh", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) { return args[0].as_char(); }
                if(args[0].is_bool_ref()) { return args[0].as_bool(); }
                if(args[0].is_s64_ref()) { return bit_util::swap_on_le<int64_t>{args[0].as_s64()}.val; }
                if(args[0].is_u64_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u64()}.val; }
                if(args[0].is_s32_ref()) { return bit_util::swap_on_le<int32_t>{args[0].as_s32()}.val; }
                if(args[0].is_u32_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u32()}.val; }
                if(args[0].is_s16_ref()) { return bit_util::swap_on_le<int16_t>{args[0].as_s16()}.val; }
                if(args[0].is_u16_ref()) { return bit_util::swap_on_le<uint16_t>{args[0].as_u16()}.val; }
                if(args[0].is_s8_ref()) { return bit_util::swap_on_le<int8_t>{args[0].as_s8()}.val; }
                if(args[0].is_u8_ref()) { return bit_util::swap_on_le<uint8_t>{args[0].as_u8()}.val; }
                if(args[0].is_float_ref()) { return bit_util::swap_on_le<float>{args[0].as_float()}.val; }
                if(args[0].is_double_ref()) { return bit_util::swap_on_le<double>{args[0].as_double()}.val; }
                if(args[0].is_long_double_ref()) { return bit_util::swap_on_le<long double>{args[0].as_long_double()}.val; }
                if(args[0].is_wchar_ref()) { return bit_util::swap_on_le<wchar_t>{args[0].as_wchar()}.val; }
                throw std::runtime_error{"invalid argument type"};
            });

            add_function("tole", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) { return args[0].as_char(); }
                if(args[0].is_bool_ref()) { return args[0].as_bool(); }
                if(args[0].is_s64_ref()) { return bit_util::swap_on_be<int64_t>{args[0].as_s64()}.val; }
                if(args[0].is_u64_ref()) { return bit_util::swap_on_be<uint64_t>{args[0].as_u64()}.val; }
                if(args[0].is_s32_ref()) { return bit_util::swap_on_be<int32_t>{args[0].as_s32()}.val; }
                if(args[0].is_u32_ref()) { return bit_util::swap_on_be<uint64_t>{args[0].as_u32()}.val; }
                if(args[0].is_s16_ref()) { return bit_util::swap_on_be<int16_t>{args[0].as_s16()}.val; }
                if(args[0].is_u16_ref()) { return bit_util::swap_on_be<uint16_t>{args[0].as_u16()}.val; }
                if(args[0].is_s8_ref()) { return bit_util::swap_on_be<int8_t>{args[0].as_s8()}.val; }
                if(args[0].is_u8_ref()) { return bit_util::swap_on_be<uint8_t>{args[0].as_u8()}.val; }
                if(args[0].is_float_ref()) { return bit_util::swap_on_be<float>{args[0].as_float()}.val; }
                if(args[0].is_double_ref()) { return bit_util::swap_on_be<double>{args[0].as_double()}.val; }
                if(args[0].is_long_double_ref()) { return bit_util::swap_on_be<long double>{args[0].as_long_double()}.val; }
                if(args[0].is_wchar_ref()) { return bit_util::swap_on_be<wchar_t>{args[0].as_wchar()}.val; }
                throw std::runtime_error{"invalid argument type"};
            });
            add_function("tobe", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) { return args[0].as_char(); }
                if(args[0].is_bool_ref()) { return args[0].as_bool(); }
                if(args[0].is_s64_ref()) { return bit_util::swap_on_le<int64_t>{args[0].as_s64()}.val; }
                if(args[0].is_u64_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u64()}.val; }
                if(args[0].is_s32_ref()) { return bit_util::swap_on_le<int32_t>{args[0].as_s32()}.val; }
                if(args[0].is_u32_ref()) { return bit_util::swap_on_le<uint64_t>{args[0].as_u32()}.val; }
                if(args[0].is_s16_ref()) { return bit_util::swap_on_le<int16_t>{args[0].as_s16()}.val; }
                if(args[0].is_u16_ref()) { return bit_util::swap_on_le<uint16_t>{args[0].as_u16()}.val; }
                if(args[0].is_s8_ref()) { return bit_util::swap_on_le<int8_t>{args[0].as_s8()}.val; }
                if(args[0].is_u8_ref()) { return bit_util::swap_on_le<uint8_t>{args[0].as_u8()}.val; }
                if(args[0].is_float_ref()) { return bit_util::swap_on_le<float>{args[0].as_float()}.val; }
                if(args[0].is_double_ref()) { return bit_util::swap_on_le<double>{args[0].as_double()}.val; }
                if(args[0].is_long_double_ref()) { return bit_util::swap_on_le<long double>{args[0].as_long_double()}.val; }
                if(args[0].is_wchar_ref()) { return bit_util::swap_on_le<wchar_t>{args[0].as_wchar()}.val; }
                throw std::runtime_error{"invalid argument type"};
            });

            add_function("i64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::int64_t)0; } return args[0].cast_to_s64(); });
            add_function("u64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::uint64_t)0; } return args[0].cast_to_u64(); });
            add_function("i32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::int32_t)0; } return args[0].cast_to_s32(); });
            add_function("u32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::uint32_t)0; } return args[0].cast_to_u32(); });
            add_function("i16", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::int16_t)0; } return args[0].cast_to_s16(); });
            add_function("u16", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::uint16_t)0; } return args[0].cast_to_u16(); });
            add_function("i8", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::int8_t)0; } return args[0].cast_to_s8(); });
            add_function("u8", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (std::uint8_t)0; } return args[0].cast_to_u8(); });
            add_function("f32", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (float)0; } return args[0].cast_to_float(); });
            add_function("f64", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (double)0; } return args[0].cast_to_double(); });
            add_function("float", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return (long double)0; } return args[0].cast_num_to_num<long double>(); });
            add_function("char", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return char{}; } return args[0].cast_to_char(); });
            add_function("wchar", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return wchar_t{}; } return args[0].cast_to_wchar(); });
            add_function("bool", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return false; } return args[0].cast_to_bool(); });
            add_function("string", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return std::string{}; } return args[0].cast_to_string(); });
            add_function("wstring", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_LE(args, 1) if(args.empty()) { return std::wstring{}; } return args[0].cast_to_wstring(); });
            add_function("vec4", TEALFUN(args) {
                valbox res{valbox::vec4_t{0, 0, 0, 1}};
                if(args.size() == 1) {
                    if(args[0].is_array_ref() || args[0].is_object_ref()) {
                        res = vec4_from_json<double>(args[0].to_json());
                    } else if(args[0].is_string_ref() || args[0].is_wstring_ref()) {
                        if(args[0].is_string_ref()) {
                            res = vec4_from_str<double>(args[0].as_string());
                        } else {
                            res = vec4_from_str<double>(args[0].as_wstring());
                        }
                    } else {
                        res.as_vec4()[0] = args[0].cast_to_long_double();
                    }
                } else {
                    for(std::size_t i = 0; i < 4 && i < args.size(); ++i) {
                        res.as_vec4()[i] = args[i].cast_to_long_double();
                    }
                }
                return res;
            });
            add_function("mat4", TEALFUN(args) {
                valbox res{valbox::mat4_t::identity()};
                if(args.size() == 16) {
                    for(std::size_t i{0}; i < 16 && i < args.size(); ++i) {
                        res.as_mat4().at_flat_index(i) = args[i].cast_to_long_double();
                    }
                } else if(args.size() < 16) {
                    std::size_t row_no{0};
                    for(std::size_t i{0}; row_no < 4 && i < args.size(); ++i) {
                        if(args[i].is_array_ref()) {
                            for(std::size_t j{0}; j < 4 && j < args[i].as_array().size(); ++j) {
                                res.as_mat4()[row_no][j] = args[i].as_array()[j].cast_to_long_double();
                            }
                        } else if(args[i].is_vec4_ref()) {
                            for(std::size_t j{0}; j < 4; ++j) {
                                res.as_mat4()[row_no][j] = args[i].as_vec4()[j];
                            }
                        } else {
                            for(std::size_t j{0}; j < 4 && i < args.size(); ++j) {
                                if(args[i].is_any_fp_number() || args[i].is_any_int_number()) {
                                    res.as_mat4()[row_no][j] = args[i].cast_to_long_double();
                                    ++i;
                                } else {
                                    break;
                                }
                            }
                        }
                        ++row_no;
                    }
                }
                return res;
            });
            add_function("array", TEALFUN(args) {
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_array();
                if(args.size() == 1 && (args[0].is_array_ref())) {
                    res.assign(args[0]);
                } else if(args.size() == 1 && (args[0].is_string_ref() || args[0].is_wstring_ref())) {
                    res.construct(args[0].cast_to_array());
                } else {
                    for(std::size_t i = 0; i < args.size(); ++i) {
                        res.as_array().push_back(args[i]);
                    }
                }
                return res;
            });
            add_function("object", TEALFUN(args) {
                if(args.empty()) {
                    return valbox{valbox_no_initialize::dont_do_it}.become_object();
                }
                if(args[0].is_object_ref()) {
                    return args[0];
                }
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_object();
                if(args[0].is_string_ref()) {
                    res.from_json(json::deserialize(args[0].as_string()));
                    return res;
                } else if(args[0].is_wstring_ref()) {
                    return json::deserialize(args[0].as_wstring());
                } else if(args[0].is_any_int_number()) {
                    return json{args[0].cast_num_to_num<int64_t>()};
                } else if(args[0].is_any_fp_number()) {
                    return json{args[0].cast_num_to_num<long double>()};
                } else if(args[0].is_bool_ref()) {
                    return json{args[0].cast_num_to_num<long double>()};
                }
                return res;
            });
            add_function("deserialize", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                valbox res{valbox_no_initialize::dont_do_it};
                if(args[0].is_string_ref()) {
                    res.from_json(json::deserialize(args[0].as_string()));
                    return res;
                } else if(args[0].is_wstring_ref()) {
                    res.from_json(json::deserialize(args[0].as_wstring()));
                }
                return res;
            });
            add_function("serialize", TEALFUN(args) {
                valbox res{valbox_no_initialize::dont_do_it};
                if(args.size() == 1) {
                    res = args[0].to_json().serialize();
                } else if(args.size() == 2) {
                    res = args[0].to_json().serialize(args[1].cast_to_u64());
                }
                return res;
            });
            add_function("serialize5", TEALFUN(args) {
                valbox res{valbox_no_initialize::dont_do_it};
                if(args.size() == 1) {
                    res = args[0].to_json().serialize5();
                } else if(args.size() == 2) {
                    res = args[0].to_json().serialize5(args[1].cast_to_u64());
                }
                return res;
            });

            add_function("contains", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                auto a1r{args[0].deref()};
                if(a1r.is_object_ref()) {
                    return a1r.as_object().find(args[1].cast_to_string()) != a1r.as_object().end();
                } else if(a1r.is_array_ref()) {
                    auto idx{args[1].cast_to_u64()};
                    return idx < a1r.as_array().size();
                }
                return false;
            });

            add_function("key_exists", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                auto a1r{args[0].deref()};
                if(a1r.is_object_ref()) {
                    return a1r.as_object().find(args[1].cast_to_string()) != a1r.as_object().end();
                }
                throw std::runtime_error{"not object"};
            });
            add_function("string_field_exists", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                auto a1r{args[0].deref()};
                if(a1r.is_object_ref()) {
                    return a1r.as_object().find(args[1].cast_to_string()) != a1r.as_object().end() &&
                           a1r.as_object().at(args[1].cast_to_string()).is_string_ref();
                }
                throw std::runtime_error{"not object"};
            });

            add_function("key_at", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                auto a1r{args[0].deref()};
                if(a1r.is_object_ref()) {
                    return a1r.object_key_at(args[1].cast_to_u64());
                }
                throw std::runtime_error{"not object"};
            });

            add_function("value_at", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                auto a1r{args[0].deref()};
                if(a1r.is_object_ref()) {
                    return a1r.object_value_at(args[1].cast_to_u64());
                }
                throw std::runtime_error{"not object"};
            });

            add_function("resize", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                valbox &a1r{args[0].deref()};
                if(a1r.is_array_ref()) {
                    auto &vec{a1r.as_array()};
                    vec.resize(args[1].cast_to_u64());
                    return true;
                } else if(a1r.is_string_ref()) {
                    auto &str{a1r.as_string()};
                    str.resize(args[1].cast_to_u64());
                    return true;
                } else if(a1r.is_wstring_ref()) {
                    auto &str{a1r.as_wstring()};
                    str.resize(args[1].cast_to_u64());
                    return true;
                }
                return false;
            });
            add_function("push_back", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                valbox &a1r{args[0].deref()};
                if(a1r.is_undefined_ref()) {
                    a1r.become_array();
                }
                a1r.as_array().push_back(args[1]);
                return args[0];
            });

#if 1
            add_function("push_front", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                valbox &a1r{args[0].deref()};
                if(a1r.is_undefined_ref()) {
                    a1r.become_array();
                }
                a1r.as_array().insert(a1r.as_array().begin(), args[1]);
                return args[0];
            });
            add_function("pop_front", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                valbox &a1r{args[0].deref()};
                if(a1r.is_array_ref()) {
                    if(a1r.as_array().empty()) {
                        throw std::runtime_error{"array empty"};
                    }
                    valbox res{a1r.as_array().front()};
                    auto &vec{a1r.as_array()};
                    vec.erase(vec.begin());
                    return res;
                }
                throw std::runtime_error{"not array"};
            });
#endif

            add_function("pop_back", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                valbox &a1r{args[0].deref()};
                if(a1r.is_array_ref()) {
                    if(a1r.as_array().empty()) {
                        throw std::runtime_error{"array empty"};
                    }
                    valbox res{a1r.as_array().back()};
                    a1r.as_array().pop_back();
                    return res;
                }
                throw std::runtime_error{"not array"};
            });

            add_function("undefine", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                args[0].become_undefined();
                return args[0];
            });

            add_function("is_undefined", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return args[0].is_undefined_ref();
            });

            add_function("is_defined", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return !args[0].is_undefined_ref();
            });

            add_function("replace_substr", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 2, 3)
                if(args.size() == 2) {
                    return str_util::to_utf8(str_util::replace_substring<std::wstring>(
                        args[0].cast_to_wstring(), args[1].cast_to_wstring(), std::wstring{}
                    ));
                }
                if(args.size() == 3) {
                    return str_util::to_utf8(str_util::replace_substring<std::wstring>(
                        args[0].cast_to_wstring(), args[1].cast_to_wstring(), args[2].cast_to_wstring()
                    ));
                }
                return std::string{};
            });

            add_function("reserve", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                if(args[0].is_string_ref()) {
                    args[0].as_string().reserve(args[1].cast_to_size_t());
                    return true;
                } else if(args[0].is_wstring_ref()) {
                    args[0].as_wstring().reserve(args[1].cast_to_size_t());
                    return true;
                }
                return false;
            });

            add_function("substr", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args.size() == 1) {
                    return args[0].cast_to_string();
                } else if(args.size() == 2) {
                    std::string s{args[0].cast_to_string()};
                    std::size_t from{args[1].cast_to_u64()};
                    return s.substr(from);
                } else if(args.size() == 3) {
                    std::string s{args[0].cast_to_string()};
                    std::size_t from{args[1].cast_to_u64()};
                    std::size_t num{args[2].cast_to_u64()};
                    return s.substr(from, num);
                } else if(args.size() > 3) {
                    return args[0].cast_to_string();
                }
                return std::string{};
            });

            add_function("slice", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args[0].is_array_ref()) {
                    if(args.size() == 1) {
                        return args[0];
                    } else if(args.size() == 2) {
                        valbox res{valbox_no_initialize::dont_do_it};
                        res.become_array();
                        res = args[0].subarray(args[1].cast_to_u64());
                        return res;
                    } else if(args.size() == 3) {
                        valbox res{valbox_no_initialize::dont_do_it};
                        res.become_array();
                        res = args[0].subarray(args[1].cast_to_u64(), args[2].cast_to_u64());
                        return res;
                    }
                } else if(args[0].is_string_ref()) {
                    if(args.size() == 1) {
                        return args[0];
                    } else if(args.size() == 2) {
                        return args[0].as_string().substr(args[1].cast_to_u64());
                    } else if(args.size() == 3) {
                        return args[0].as_string().substr(args[1].cast_to_u64(), args[2].cast_to_u64());
                    } else if(args.size() > 3) {
                        return args[0].cast_to_string();
                    }
                } else if(args[0].is_wstring_ref()) {
                    if(args.size() == 1) {
                        return args[0];
                    } else if(args.size() == 2) {
                        return args[0].as_wstring().substr(args[1].cast_to_u64());
                    } else if(args.size() == 3) {
                        return args[0].as_wstring().substr(args[1].cast_to_u64(), args[2].cast_to_u64());
                    } else if(args.size() > 3) {
                        return args[0].cast_to_wstring();
                    }
                }
                return valbox{valbox_no_initialize::dont_do_it};
            });

            add_function("str_tok", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_array();
                if(args[0].is_string_ref()) {
                    std::vector<std::string> sv{str_util::str_tok(args[0].cast_to_string(), args[1].cast_to_string())};
                    for(auto &&s: sv) {
                        res.as_array().push_back(s);
                    }
                } else if(args[0].is_wstring_ref()) {
                    std::vector<std::wstring> sv{str_util::str_tok(args[0].cast_to_wstring(), args[1].cast_to_wstring())};
                    for(auto &&s: sv) {
                        res.as_array().push_back(s);
                    }
                }
                return res;
            });

            add_function("ltrim", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_string_ref()) {
                    return str_util::ltrim<std::string>(args[0].as_string());
                } else if(args[0].is_wstring_ref()) {
                    return str_util::ltrim<std::wstring>(args[0].as_wstring());
                } else {
                    return args[0];
                }
            });

            add_function("rtrim", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_string_ref()) {
                    return str_util::rtrim<std::string>(args[0].as_string());
                } else if(args[0].is_wstring_ref()) {
                    return str_util::rtrim<std::wstring>(args[0].as_wstring());
                } else {
                    return args[0];
                }
            });
            add_function("trim", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_string_ref()) {
                    return str_util::trim<std::string>(args[0].as_string());
                } else if(args[0].is_wstring_ref()) {
                    return str_util::trim<std::wstring>(args[0].as_wstring());
                } else {
                    return args[0];
                }
            });

            add_function("isspace", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isspace(args[0].cast_to_int()); });
            add_function("isalpha", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isalpha(args[0].cast_to_int()); });
            add_function("isdigit", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isdigit(args[0].cast_to_int()); });
            add_function("isalnum", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isalnum(args[0].cast_to_int()); });
            add_function("ispunct", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::ispunct(args[0].cast_to_int()); });
            add_function("iscntrl", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::iscntrl(args[0].cast_to_int()); });
            add_function("ishexdigit", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::ishexdigit(args[0].cast_to_int()); });
            add_function("isoctdigit", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isoctdigit(args[0].cast_to_int()); });
            add_function("isbindigit", TEALFUN(args) { TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1) return str_util::fltr<std::wstring>::isbindigit(args[0].cast_to_int()); });

            add_function("subarray", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args[0].is_array_ref()) {
                    if(args.size() == 1) {
                        return args[0].subarray();
                    } else if(args.size() == 2) {
                        return args[0].subarray(args[1].cast_num_to_num<std::size_t>());
                    } else if(args.size() == 3) {
                        return args[0].subarray(args[1].cast_num_to_num<std::size_t>(), args[2].cast_num_to_num<std::size_t>());
                    }
                }
                throw std::runtime_error{"the first argument must be array"};
            });

            add_function("hexdump", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 4)
                if(args.size() == 1) {
                    return str_util::hexdump(args[0].cast_to_byte_array());
                } else if(args.size() == 2) {
                    return str_util::hexdump(
                        args[0].cast_to_byte_array(),
                        args[1].cast_to_u64()
                    );
                } else if(args.size() == 3) {
                    return str_util::hexdump(
                        args[0].cast_to_byte_array(),
                        args[1].cast_to_u64(),
                        args[2].cast_to_string()
                    );
                } else if(args.size() == 4) {
                    return str_util::hexdump(
                        args[0].cast_to_byte_array(),
                        args[1].cast_to_u64(),
                        args[2].cast_to_string(),
                        args[3].cast_to_bool()
                     );
                }
                return std::string{};
            });

            add_function("data_to_base85_str", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_byte_array()};
                return data_to_base85_str(src);
            });

            add_function("base85_str_to_data", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_string()};
                auto d{base85_str_to_data(src)};
                return std::string{d.begin(), d.end()};
            });

            add_function("data_to_base64_str", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_byte_array()};
                return data_to_base64_str(src);
            });

            add_function("base64_str_to_data", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_string()};
                auto d{base64_str_to_data(src)};
                return std::string{d.begin(), d.end()};
            });

            add_function("data_to_hex_str", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_byte_array()};
                return data_to_hex_str(src);
            });

            add_function("hex_str_to_data", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 1)
                auto src{args[0].cast_to_string()};
                auto d{hex_str_to_data(src)};
                return std::string{d.begin(), d.end()};
            });

            add_function("getset", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                valbox res{valbox_no_initialize::dont_do_it};
                res.assign(args[0]);
                args[0].assign(args[1]);
                return res;
            });


            add_function("atoi", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 3)
                if(args.size() == 1) {
                    return str_util::atoi(args[0].cast_to_string());
                } else if(args.size() == 2) {
                    return str_util::atoi(args[0].cast_to_string(), args[1].cast_to_u64());
                } else if(args.size() == 3) {
                    return str_util::atoi(args[0].cast_to_string(), args[1].cast_to_u64(), args[2].cast_to_bool());
                }
                return std::int64_t{0};
            });

            add_function("atoui", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 2)
                if(args.size() == 1) {
                    return str_util::atoui(args[0].cast_to_string());
                } else if(args.size() == 2) {
                    return str_util::atoui(args[0].cast_to_string(), args[1].cast_to_u64());
                }
                return std::uint64_t{0};
            });

            add_function("atof", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                return str_util::atof(args[0].cast_to_string());
            });

            add_function("itoa", TEALFUN(args) {
                if(args.size() > 0) {
                    if(args.size() > 1) {
                        if(args.size() > 2) {
                            if(args.size() > 3) {
                                return str_util::itoa<std::string>(args[0].cast_to_s64(), args[1].cast_to_s64(), args[2].cast_to_s64(), args[3].cast_to_bool());
                            } else {
                                return str_util::itoa<std::string>(args[0].cast_to_s64(), args[1].cast_to_s64(), args[2].cast_to_s64());
                            }
                        } else {
                            return str_util::itoa<std::string>(args[0].cast_to_s64(), args[1].cast_to_s64());
                        }
                    } else {
                        return str_util::itoa<std::string>(args[0].cast_to_s64());
                    }
                }
                return std::string{};
            });

            add_function("utoa", TEALFUN(args) {
                if(args.size() > 0) {
                    if(args.size() > 1) {
                        if(args.size() > 2) {
                            if(args.size() > 3) {
                                return str_util::utoa<std::string>(args[0].cast_to_u64(), args[1].cast_to_s64(), args[2].cast_to_s64(), args[3].cast_to_bool());
                            } else {
                                return str_util::utoa<std::string>(args[0].cast_to_u64(), args[1].cast_to_s64(), args[2].cast_to_s64());
                            }
                        } else {
                            return str_util::utoa<std::string>(args[0].cast_to_u64(), args[1].cast_to_s64());
                        }
                    } else {
                        return str_util::utoa<std::string>(args[0].cast_to_u64());
                    }
                }
                return std::string{};
            });

            add_function("ftoa", TEALFUN(args) {
                if(args.size() > 0) {
                    if(args.size() > 1) {
                        return str_util::ftoa(args[0].cast_to_long_double(), args[1].cast_to_u64());
                    } else {
                        return str_util::ftoa(args[0].cast_to_long_double());
                    }
                }
                return std::string{"0.0"};
            });

            add_function("toupper", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) {
                    return str_util::toupper(args[0].as_char());
                } else {
                    return str_util::towupper(args[0].cast_to_u64());
                }
            });
            add_function("tolower", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) {
                    return str_util::tolower(args[0].as_char());
                } else {
                    return str_util::towlower(args[0].cast_to_u64());
                }
            });

            add_function("strtoupper", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_string_ref()) {
                    return str_util::fltr<std::string>::strtoupper(args[0].as_string());
                } else if(args[0].is_wstring_ref()) {
                    return str_util::fltr<std::wstring>::strtoupper(args[0].as_wstring());
                } else {
                    throw std::runtime_error{"invalid argument"};
                }
            });
            add_function("strtolower", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
                if(args[0].is_char_ref()) {
                    return str_util::tolower(args[0].as_char());
                } else {
                    return str_util::towlower(args[0].cast_to_u64());
                }
            });


            add_function("get_bit_field", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 3)
                if(args[0].is_u64_ref()) {
                    bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_s64_ref()) {
                    bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_u32_ref()) {
                    bit_util::bits<std::uint32_t> bf{args[0].cast_to_u32()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_s32_ref()) {
                    bit_util::bits<std::uint32_t> bf{args[0].cast_to_u32()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_u16_ref()) {
                    bit_util::bits<std::uint16_t> bf{args[0].cast_to_u16()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_s16_ref()) {
                    bit_util::bits<std::uint16_t> bf{args[0].cast_to_u16()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_u8_ref()) {
                    bit_util::bits<std::uint8_t> bf{args[0].cast_to_u8()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                } else if(args[0].is_s8_ref()) {
                    bit_util::bits<std::uint8_t> bf{args[0].cast_to_u8()};
                    return bf.get(args[1].cast_to_u64(), args[2].cast_to_u64());
                }
                return args[0];
            });

            add_function("get_bit", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                auto bitpos{args[1].cast_to_u64()};
                return bf.get(bitpos);
            });

            add_function("update_bit", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 3)
                bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                auto bitpos{args[1].cast_to_u64()};
                auto bitval{args[2].cast_to_u64()};
                if(bitval != 0) {
                    bf.set(bitpos);
                } else {
                    bf.clr(bitpos);
                }
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_same_type_as(args[0]);
                res.assign_preserving_type(bf.whole());
                return res;
            });
            add_function("set_bit", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                bf.set(args[1].cast_to_u64());
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_same_type_as(args[0]);
                res.assign_preserving_type(bf.whole());
                return res;
            });
            add_function("clear_bit", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
                bit_util::bits<std::uint64_t> bf{args[0].cast_to_u64()};
                bf.clr(args[1].cast_to_u64());
                valbox res{valbox_no_initialize::dont_do_it};
                res.become_same_type_as(args[0]);
                res.assign_preserving_type(bf.whole());
                return res;
            });


            add_function("sleep", TEALFUN(args) {
                long double time_to_sleep{args[0].cast_to_long_double()};
                if(time_to_sleep > 0) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds{static_cast<std::uint64_t>(time_to_sleep * 1000000000.0L)});
                }
                return time_to_sleep;
            });
            add_function("set_cycle_sleep_seconds", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1);
                uint64_t time_to_sleep{static_cast<uint64_t>(args[0].cast_to_long_double() * 1000000000.0L)};
                set_nanoseconds_of_sleeping_between_cycles(time_to_sleep);
                return time_to_sleep;
            });
            add_function("cycle_nanosleep", TEALFUN() {
                return sleep_between_cycles_nanoseconds();
            });
            add_function("set_inactive_sleep_seconds", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1);
                uint64_t time_to_sleep{static_cast<uint64_t>(args[0].cast_to_long_double() * 1000000000.0L)};
                set_sleep_inactive_thread_nanoseconds(time_to_sleep);
                return time_to_sleep;
            });
            add_function("inactive_nanosleep", TEALFUN() {
                return sleep_inactive_thread_nanoseconds();
            });
            add_function("exit", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 1, 2);
                if(programmatic_termination_enabled_ != 0) {
                    exit_status_ = args.size() == 2 ? args[1].cast_num_to_num<int>() : 0;
                    terminate();
                    execution_context *ctx{(execution_context *)args[0].as_ptr()};
                    ctx->request_return();
                }
                return programmatic_termination_enabled_ != 0;
            });

            add_function("assert", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_GE(args, 3);
                if(!args[2].cast_to_bool()) {
                    throw runtime_error{args[0].cast_to_s64(), args[0].cast_to_s64(), std::string{"assertion failed"}};
                }
                return true;
            });

            add_function("assign", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2);
                args[0].assign_preserving_type(args[1]);
                return args[0];
            });


            add_function("thread_id", TEALFUN() {
                std::stringstream ss{};
                ss << std::this_thread::get_id();
                std::uint64_t res{};
                ss >> res;
                return res;
            });
            add_function("hardware_concurrency", TEALFUN() {
                std::shared_lock l{threads_mtp_};
                auto ts{threads_.size()};
                return ts > 0 ? ts : 1;
            });

            add_function("enable_values_network_exposing", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_IN_RANGE(args, 0, 3);
                std::string bind_addr{"0.0.0.0"};
                std::uint16_t port{43987};
                long double stale_connections_removal_timeout{0};
                if(args.size() > 0) { bind_addr = args[0].cast_to_string(); }
                if(args.size() > 1) { port = args[1].cast_to_u16(); }
                if(args.size() > 2) { port = args[2].cast_to_long_double(); }
                start_net_server(teal::net::address_family::inet4, bind_addr, port, stale_connections_removal_timeout);
                return net_server_running();
            });

            add_function("disable_values_network_exposing", TEALFUN(args) {
                stop_net_server();
                return !net_server_running();
            });

            add_function("extern_update_nanointerval", TEALFUN(args) {
                return ext_cells_refresh_interval_nanos_;
            });

            add_function("set_extern_update_seconds", TEALFUN(args) {
                TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1);
                ext_cells_refresh_interval_nanos_ = args[0].cast_to_long_double() * 1'000'000'000;
                return ext_cells_refresh_interval_nanos_;
            });

            add_function("size", TEALFUN(args) {
                valbox &der{args[0].deref()};
                switch(der.val_type()) {
                    case valbox::type::STRING: return static_cast<uint64_t>(der.as_string().size());
                    case valbox::type::WSTRING: return static_cast<uint64_t>(der.as_wstring().size());
                    case valbox::type::ARRAY: return static_cast<uint64_t>(der.as_array().size());
                    case valbox::type::OBJECT: return static_cast<uint64_t>(der.as_object().size());
                    default: return static_cast<uint64_t>(1);
                }
            });

            add_function("empty", TEALFUN(args) {
                valbox arg0{args[0]};
                return arg0.is_undefined_ref() ||
                       (arg0.is_object_ref() && arg0.as_object().empty()) ||
                       (arg0.is_array_ref() && arg0.as_array().empty()) ||
                       (arg0.is_string_ref() && arg0.as_string().empty()) ||
                       (arg0.is_wstring_ref() && arg0.as_wstring().empty());
            });
            check_func_kw_ = true;
        }

        runtime(runtime const &) = delete;
        runtime(runtime &&) = delete;
        runtime &operator=(runtime const &) = delete;
        runtime &operator=(runtime &&) = delete;
        ~runtime() {
            terminate();
            stop_mt();
            stop_net_server();
            stop_extcell_processing();
            worker_cells_templates_.clear();
            global_constants_dictionary_.clear();
            global_functions_dictionary_.clear();
            global_methods_dictionary_.clear();
            input_cells_.clear();
            input_names_to_instances_mapping_.clear();
            outputs_.clear();
            worker_cells_templates_.clear();
            worker_cells_.clear();
            worker_bodies_.clear();
            user_functions_.clear();

            unload_extensions();
            sockext_.unregister_runtime();
            dict_ext_.unregister_runtime();
            crypt_.unregister_runtime();
            fpool_.unregister_runtime();
            perf_stat_.unregister_runtime();
#ifndef TEALSCRIPT_NO_EIGEN
            eigen_ext_.unregister_runtime();
#endif
            randlib_.unregister_runtime();
            time_ext_.unregister_runtime();
            math_ext_.unregister_runtime();
            array_buffer_ext_.unregister_runtime();
        }

        timespec_wrapper valbox_to_timestamp(std::vector<valbox> const &args) const {
            if(args.size() >= 1) {
                if(args[0].is_string_ref()) {
                    return timespec_wrapper{args[0].as_string()};
                } else if(args[0].is_wstring_ref()) {
                    return timespec_wrapper{args[0].as_wstring()};
                } else if(args[0].is_any_fp_number()) {
                    return timespec_wrapper{args[0].cast_to_long_double()};
                } else if(args[0].is_any_int_number()) {
                    return timespec_wrapper{args[0].cast_to_s64()}; // milliseconds
                }
            }
            return timespec_wrapper::now();
        }

        void load_file(std::string const &filename) {
            if(!load_library(filename)) {
                if(file_util::file_exists(filename)) {
                    std::string src{file_util::load_text_file(filename)};
                    load_source_string(src);
                }
            }
        }

        void load_string(std::string const &src) {
            load_source_string(src);
        }

        void loading_complete() {
            std::unique_lock l{workers_mtp_};
            for(auto &&wcp: worker_cells_) {
                std::shared_ptr<worker_cell_instance> curr_cell{wcp.second};
                auto type_it{worker_cells_templates_.find(curr_cell->type_name())};
                if(type_it != worker_cells_templates_.end()) {
                    if(curr_cell->actual_args_info().size() != static_cast<size_t>(type_it->second.num_args())) {
                        throw runtime_error{curr_cell->line(), curr_cell->col(), "actual arguments count for the cell mismatch"};
                    }
                    curr_cell->set_type_info(type_it->second.num_args(), type_it->second.arg_names());
                }
            }
        }

        bool check_func_kw_{false};

        void add_function(std::string const &func_name, std::function<valbox(std::vector<valbox> &)> f) override {
            if(!is_identifier(func_name)) {
                throw std::runtime_error{std::string{"invalid identifier: \""} + func_name + "\""};
            }
            if(check_func_kw_ && is_keyword(str_util::from_utf8(func_name))) {
                throw std::runtime_error{std::string{"name \""} + func_name + "\" is a keyword"};
            }
            if(global_functions_dictionary_.find(func_name) != global_functions_dictionary_.end()) {
                throw std::runtime_error{
                    std::string{"symbol already exists: \""} + func_name + "\""
                };
            }
            global_functions_dictionary_[func_name] = valbox{std::move(f), func_name, false};
        }

        void remove_function(std::string const &fname) override {
            auto it{global_functions_dictionary_.find(fname)};
            if(it != global_functions_dictionary_.end()) {
                global_functions_dictionary_.erase(it);
            }
        }

        void add_var(std::string const &var_name, valbox const &v) override {
            if(!is_identifier(var_name)) { throw std::runtime_error{std::string{"invalid identifier: \""} + var_name + "\""}; }
            if(is_keyword(str_util::from_utf8(var_name))) {
                throw std::runtime_error{std::string{"name \""} + var_name + "\" is a keyword"};
            }
            if(global_constants_dictionary_.find(var_name) != global_constants_dictionary_.end()) {
                throw std::runtime_error{
                    std::string{"symbol already exists: \""} + var_name + "\""
                };
            }
            global_constants_dictionary_[var_name] = v;
        }

        void remove_var(std::string const &varname) override {
            auto it{global_constants_dictionary_.find(varname)};
            if(it != global_constants_dictionary_.end()) {
                global_constants_dictionary_.erase(it);
            }
        }

        void add_method(std::string const &class_name, std::string const &method_name, std::function<valbox(std::vector<valbox> &)> f) override {
            if(!is_identifier(class_name)) { throw std::runtime_error{std::string{"invalid identifier: \""} + class_name + "\""}; }
            if(is_keyword(str_util::from_utf8(class_name))) {
                throw std::runtime_error{std::string{"name \""} + class_name + "\" is a keyword"};
            }
            if(!is_identifier(method_name)) { throw std::runtime_error{std::string{"invalid identifier: \""} + method_name + "\""}; }
            if(is_keyword(str_util::from_utf8(method_name))) {
                throw std::runtime_error{std::string{"name \""} + method_name + "\" is a keyword"};
            }
            if(global_methods_dictionary_[class_name].find(method_name) != global_methods_dictionary_[class_name].end()) {
                throw std::runtime_error{std::string{"method already exists: \""} + method_name + "\""};
            }
            global_methods_dictionary_[class_name][method_name] = valbox{std::move(f), method_name, false};
        }

        void remove_method(std::string const &class_name, std::string const &method_name) override {
            auto class_name_it{global_methods_dictionary_.find(class_name)};
            if(class_name_it != global_methods_dictionary_.end()) {
                auto method_name_it{class_name_it->second.find(method_name)};
                if(method_name_it != class_name_it->second.end()) {
                    class_name_it->second.erase(method_name_it);
                    if(class_name_it->second.empty()) {
                        global_methods_dictionary_.erase(class_name_it);
                    }
                }
            }
        }


        void remove_object_services(std::string const &class_name) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_.erase(class_name);
        }

        void add_object_serializer(
            std::string const &class_name,
            std::function<std::optional<std::string>(valbox const &)> const &fun
        ) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_[class_name].serializer = fun;
        }

        void add_object_deserializer(
            std::string const &class_name,
            std::function<valbox(std::string const &, std::string const &)> const &fun
        ) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_[class_name].deserializer = fun;
        }

        virtual void add_object_stringifier(
            std::string const &class_name,
            std::function<valbox(valbox const &)> const &fun
        ) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_[class_name].stringify = fun;
        }

        void add_object_unary_operation(
            std::string const &class_name, std::string const &op_code,
            std::function<valbox(valbox &)> const &fun
        ) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_[class_name].unops[op_code] = fun;
        }

        void add_object_binary_operation(
            std::string const &class_name, std::string const &op_code,
            std::function<valbox(valbox &, valbox &)> const &fun
        ) override {
            std::unique_lock l{obj_ser_mtp_};
            obj_svc_[class_name].binops[op_code] = fun;
        }

        obj_services def_obj_svc_{
            [](valbox const &) -> std::optional<std::string> { return std::optional<std::string>{}; },
            [](std::string const &, std::string const &) -> valbox { return {}; },
            [](valbox const &) -> valbox { return std::string{}; }
        };
        obj_services const *get_object_services(std::string const &class_name) const override {
            std::shared_lock l{obj_ser_mtp_};
            auto it{obj_svc_.find(class_name)};
            return it == obj_svc_.end() ? &def_obj_svc_ : &it->second;
        }


        valbox get_node_val(std::string const &name) {
            std::shared_lock l{workers_mtp_};
            auto it{worker_cells_.find(name)};
            if(it != worker_cells_.end()) {
                std::shared_ptr<worker_cell_instance> ci{it->second};
                l.unlock();
                return ci->value();
            }
            throw std::runtime_error{std::string{"node not found: \""} + name + "\""};
        }

        size_t worker_cells_count() const {
            std::shared_lock l{workers_mtp_};
            return worker_cells_.size();
        }

        void set_single_thread_mode() {
            stop_mt();
            unterminate();
            set_thread_mode_single();
        }

        void set_multi_thread_mode() {
            set_thread_mode_multi();
        }

        void run_cycles(std::size_t n) {
            for(std::size_t i{0}; i < n; ++i) {
                run_cycle();
                if(termination_requested()) {
                    break;
                }
            }
        }

        void run_cycle() {
            if(termination_requested()) {
                return;
            }
            if(!is_current_thread_mode_single()) {
                if(is_current_thread_mode_none()) {
                    set_thread_mode_single();
                } else {
                    throw std::runtime_error{"set single thread mode first"};
                }
            }
            start_extcell_processing();
            exctx_.clear_all_jumps_request();
            for(auto &&w: worker_cells_) {
                std::shared_ptr<worker_cell_instance> &curr_cell{w.second};
                std::string const &curr_cell_type_name{curr_cell->type_name()};

                if(!curr_cell->type_info_transferred()) {
                    auto type_it{worker_cells_templates_.find(curr_cell_type_name)};
                    if(type_it != worker_cells_templates_.end()) {
                        if(curr_cell->actual_args_info().size() != static_cast<size_t>(type_it->second.num_args())) {
                            throw runtime_error{curr_cell->line(), curr_cell->col(), "actual arguments count for the cell mismatch"};
                        }
                        curr_cell->set_type_info(type_it->second.num_args(), type_it->second.arg_names());
                    }
                }

                exctx_.set_self_fields(curr_cell->cell_self_values_ptr());

                exctx_.clear_stack_soft();
                exctx_.new_stack_frame();

                auto &&args_info{curr_cell->actual_args_info()};
                for(std::size_t curr_arg_number{0}; curr_arg_number < args_info.size(); ++curr_arg_number) {
                    auto &&ai{args_info[curr_arg_number]};
                    std::string curr_arg_name{ai.argname};
                    if(ai.is_cell) {
                        if(ai.cell_ptr != nullptr) {
                            exctx_.set_local_value(curr_arg_name, ai.cell_ptr->value());
                        } else {
                            auto w_it{worker_cells_.find(ai.cell_name)};
                            if(w_it != worker_cells_.end()) {
                                ai.cell_ptr = w_it->second.get();
                                exctx_.set_local_value(curr_arg_name, w_it->second->value());
                            } else {
                                auto in_it{input_cells_.find(ai.cell_name)};
                                if(in_it != input_cells_.end()) {
                                    ai.cell_ptr = in_it->second.get();
                                    exctx_.set_local_value(curr_arg_name, in_it->second->value());
                                } else {
                                    auto ex_it{extern_cells_.find(ai.cell_name)};
                                    if(ex_it != extern_cells_.end()) {
                                        ai.cell_ptr = ex_it->second.get();
                                        exctx_.set_local_value(curr_arg_name, ex_it->second->value());
                                    } else {
                                        throw runtime_error{
                                            curr_cell->line(), curr_cell->col(),
                                            std::string{"input value not found for compute element \""} +
                                                curr_cell->inst_name() + "\""
                                        };
                                    }
                                }
                            }
                        }
                    } else {
                        if(ai.expr_val.is_undefined_ref()) {
                            ai.expr_val = ai.expr->eval(&exctx_, eval_caller_type::no_matter, nullptr);
                        }
                        valbox vb{ai.expr_val};
                        exctx_.set_local_value(curr_arg_name, vb);
                    }
                }

                if(!curr_cell->body()) {
                    auto body_it{worker_bodies_.find(curr_cell_type_name)};
                    if(body_it == worker_bodies_.end()) {
                        throw runtime_error{curr_cell->line(), curr_cell->col(), "cell not found"};
                    }
                    curr_cell->set_body(body_it->second);
                }
                curr_cell->body()->exec(&exctx_);

                if(termination_requested()) {
                    break;
                }
                if(exctx_.return_requested()) {
                    curr_cell->set_value(exctx_.return_result());
                    if(!curr_cell->output_name().empty()) {
                        exctx_.set_output(curr_cell->output_name(), exctx_.return_result());
                    }
                }
                exctx_.clear_all_jumps_request();

                if(sleep_between_cycles_nanoseconds_ > 0) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds{sleep_between_cycles_nanoseconds_});
                }
            }
        }

        void terminate() {
            termination_requested_.store(1, std::memory_order_release);
        }

        void unterminate() {
            termination_requested_.store(0, std::memory_order_release);
        }

        bool termination_requested() const {
            return termination_requested_.load(std::memory_order_acquire) != 0;
        }

        void fail(std::string const &descr) {
            std::unique_lock l{failure_mtp_};
            failure_description_ = descr;
            failure_.store(1, std::memory_order_release);
        }

        void unfail() {
            std::unique_lock l{failure_mtp_};
            failure_description_.clear();
            failure_.store(0, std::memory_order_release);
        }

        bool failure_occured() const {
            return failure_.load(std::memory_order_acquire) != 0;
        }

        std::string failure_description() const {
            std::shared_lock l{failure_mtp_};
            return failure_description_;
        }

        bool programmatic_termination_enabled() const {
            return programmatic_termination_enabled_ != 0;
        }

        void set_programmatic_termination_enabled(bool val) {
            programmatic_termination_enabled_ = (val ? 1 : 0);
        }

        int exit_status() const noexcept {
            return exit_status_;
        }

        std::uint64_t sleep_between_cycles_nanoseconds() const noexcept {
            return sleep_between_cycles_nanoseconds_;
        }

        void set_nanoseconds_of_sleeping_between_cycles(std::uint64_t val) noexcept {
            sleep_between_cycles_nanoseconds_ = val;
        }

        std::uint64_t sleep_inactive_thread_nanoseconds() const noexcept {
            return sleep_inactive_thread_nanoseconds_;
        }

        void set_sleep_inactive_thread_nanoseconds(std::uint64_t val) noexcept {
            sleep_inactive_thread_nanoseconds_ = val;
        }

        void stop_mt() {
            std::unique_lock l{threads_mtp_};
            if(!is_current_thread_mode_multi()) {
                return;
            }
            termination_requested_.store(1, std::memory_order_release);
            for(auto &&t: threads_) {
                if(t.joinable()) {
                    t.join();
                }
            }
            threads_.clear();
            set_thread_mode_none();
        }

        void run_mt(int thrd_cnt) {
            std::unique_lock l{threads_mtp_};
            if(!is_current_thread_mode_multi()) {
                if(is_current_thread_mode_none()) {
                    set_thread_mode_multi();
                } else {
                    throw std::runtime_error{"set multi thread mode first"};
                }
            }
            if(!threads_.empty()) {
                return;
            }
            unfail();
            unterminate();

            start_extcell_processing();
            for(int i{0}; i < thrd_cnt; ++i) {
                threads_.emplace_back([this]() {
                    bool excepted{false};
                    std::string exbuf{};
#ifndef TEAL_DEBUGGING
                    try {
#endif
                        std::shared_ptr<execution_context> exctx{std::make_shared<execution_context>()};
                        execution_context *exctx_ptr{exctx.get()};
                        exctx_ptr->set_runtime_interface(this);
                        while(!termination_requested() && !failure_occured()) {
                            exctx_ptr->clear_all_jumps_request();
                            bool have_locked{false};
                            for(auto &&wrkcl: worker_cells_) {
                                std::shared_ptr<worker_cell_instance> &curr_cell{wrkcl.second};
                                bool cell_executed{false};
                                if(curr_cell->try_lock()) {
                                    shut_on_destroy sod{[&]() { curr_cell->unlock(); }};
                                    cell_executed = true;
                                    have_locked = true;
                                    if(termination_requested()) {
                                        break;
                                    }
                                    std::string const &curr_cell_type_name{curr_cell->type_name()};

                                    if(!curr_cell->type_info_transferred()) {
                                        auto type_it{worker_cells_templates_.find(curr_cell_type_name)};
                                        if(type_it != worker_cells_templates_.end()) {
                                            if(curr_cell->actual_args_info().size() != static_cast<size_t>(type_it->second.num_args())) {
                                                throw runtime_error{curr_cell->line(), curr_cell->col(), "actual arguments count for the cell mismatch"};
                                            }
                                            curr_cell->set_type_info(type_it->second.num_args(), type_it->second.arg_names());
                                        }
                                    }

                                    exctx_ptr->set_self_fields(curr_cell->cell_self_values_ptr());

                                    exctx_ptr->clear_stack_soft();
                                    exctx_ptr->new_stack_frame();

                                    std::vector<worker_cell_instance::arg_info> &args_info{curr_cell->actual_args_info()};
                                    for(std::size_t curr_arg_number{0}; curr_arg_number < args_info.size(); ++curr_arg_number) {
                                        worker_cell_instance::arg_info &ai{args_info[curr_arg_number]};
                                        std::string curr_arg_name{ai.argname};
                                        if(ai.is_cell) {
                                            if(ai.cell_ptr != nullptr) {
                                                exctx_ptr->set_local_value(curr_arg_name, ai.cell_ptr->value());
                                            } else {
                                                auto w_it{worker_cells_.find(ai.cell_name)};
                                                if(w_it != worker_cells_.end()) {
                                                    ai.cell_ptr = w_it->second.get();
                                                    exctx_ptr->set_local_value(curr_arg_name, ai.cell_ptr->value());
                                                } else {
                                                    auto in_it{input_cells_.find(ai.cell_name)};
                                                    if(in_it != input_cells_.end()) {
                                                        ai.cell_ptr = in_it->second.get();
                                                        exctx_ptr->set_local_value(curr_arg_name, ai.cell_ptr->value());
                                                    } else {
                                                        auto ex_it{extern_cells_.find(ai.cell_name)};
                                                        if(ex_it != extern_cells_.end()) {
                                                            ai.cell_ptr = ex_it->second.get();
                                                            exctx_ptr->set_local_value(curr_arg_name, ai.cell_ptr->value());
                                                        } else {
                                                            throw runtime_error{
                                                                curr_cell->line(), curr_cell->col(),
                                                                std::string{"input value not found for compute element \""} +
                                                                    curr_cell->inst_name() + "\""
                                                            };
                                                        }
                                                    }
                                                }
                                            }
                                        } else {
                                            if(ai.expr_val.is_undefined_ref()) {
                                                ai.expr_val = ai.expr->eval(exctx_ptr, eval_caller_type::no_matter, nullptr);
                                            }
                                            valbox vb{ai.expr_val};
                                            exctx_ptr->set_local_value(curr_arg_name, vb);
                                        }
                                    }

                                    if(!curr_cell->body()) {
                                        auto body_it{worker_bodies_.find(curr_cell_type_name)};
                                        if(body_it == worker_bodies_.end()) {
                                            throw runtime_error{curr_cell->line(), curr_cell->col(), "cell not found"};
                                        }
                                        curr_cell->set_body(body_it->second);
                                    }
                                    curr_cell->body()->exec(exctx_ptr);

                                    if(termination_requested() || failure_occured()) {
                                        break;
                                    }
                                    if(exctx_ptr->return_requested()) {
                                        curr_cell->set_value(exctx_ptr->return_result());
                                        if(!curr_cell->output_name().empty()) {
                                            exctx_ptr->set_output(curr_cell->output_name(), exctx_ptr->return_result());
                                        }
                                    }
                                    exctx_ptr->clear_all_jumps_request();
                                }
                                if(cell_executed && sleep_between_cycles_nanoseconds_ > 0) {
                                    std::this_thread::sleep_for(std::chrono::nanoseconds{sleep_between_cycles_nanoseconds_});
                                }
                            }
                            if(!have_locked) {
                                std::this_thread::sleep_for(std::chrono::nanoseconds{sleep_inactive_thread_nanoseconds_});
                            }
                        }
#ifndef TEAL_DEBUGGING
                    } catch(std::exception const &e) {
                        excepted = true;
                        exbuf = e.what();
                    } catch(valbox const &v) {
                        excepted = true;
                        exbuf = v.cast_to_string();
                    }
#endif
                    if(excepted) {
                        fail(exbuf);
                    }
                });
            }
        }

        bool wait(long double secnds) {
            if(thread_mode_ == thread_mode::multi) {
                auto slpfor{std::chrono::nanoseconds{
                        std::min<std::int64_t>(static_cast<std::int64_t>(secnds * 1'000'000'000.0L),
                        wait_granularity_nsec_)
                    }
                };
                auto future{
                    std::chrono::steady_clock::now() +
                    std::chrono::nanoseconds{static_cast<std::int64_t>(secnds * 1'000'000'000.0L)}
                };
                while(
                    !termination_requested() &&
                    !failure_occured() &&
                    std::chrono::steady_clock::now() < future
                ) {
                    std::this_thread::sleep_for(slpfor);
                }
                std::shared_lock l{threads_mtp_};
                if(termination_requested() || failure_occured()) {
                    for(auto &&t: threads_) {
                        if(t.joinable()) {
                            t.join();
                        }
                    }
                }
                return termination_requested() || failure_occured();
            }
            return true;
        }

        std::int64_t wait_granularity_nsec() const noexcept {
            return wait_granularity_nsec_;
        }

        void set_wait_granularity_nsec(std::int64_t val) noexcept {
            wait_granularity_nsec_ = val;
        }

        // runtime interface ---------------------------------------------------------------
        str_map_t<valbox> const *global_constants_dictionary() const override {
            return &global_constants_dictionary_;
        }

        str_map_t<valbox> const *global_functions_dictionary() const override {
            return &global_functions_dictionary_;
        }

        str_map_t<str_map_t<valbox>> const *global_methods_dictionary() const override {
            return &global_methods_dictionary_;
        }

        std::function<bool(std::string)> const &user_functions_search() const override {
            return user_functions_search_;
        }

        std::function<valbox(std::vector<valbox> &)> const &user_function_selector() const override {
            return user_function_selector_;
        }

        valbox get_input(std::string const &name) override {
            std::shared_lock l{input_cells_mtp_};
            return input_cells_[name]->value();
        }

        valbox const &get_output(std::string const &name) override {
            std::shared_lock l{outputs_mtp_};
            return outputs_[name];
        }

        void set_input(std::string const &name, valbox const &val) override {
            std::unique_lock l{input_cells_mtp_};
            auto it{input_names_to_instances_mapping_.find(name)};
            if(it == input_names_to_instances_mapping_.end()) {
                throw std::runtime_error{name + ": input name not found"};
            }
            input_cells_[it->second]->set_value(val);
        }

        void set_output(std::string const &name, valbox const &val) override {
            std::unique_lock l{outputs_mtp_};
            outputs_[name] = val;
        }

        void clear_input(std::string const &name) override {
            std::unique_lock l{input_cells_mtp_};
            input_cells_[name]->set_value({});
        }

        void clear_output(std::string const &name) override {
            std::unique_lock l{outputs_mtp_};
            outputs_.erase(name);
        }

        void clear_inputs() override {
            std::unique_lock l{input_cells_mtp_};
            input_cells_.clear();
        }

        void clear_outputs() override {
            std::unique_lock l{outputs_mtp_};
            outputs_.clear();
        }

        void start_net_server(
            net::address_family af,
            std::string const &bind_addr,
            std::uint16_t port,
            long double stale_connections_removal_timeout
        ) override {
            {
                std::shared_lock l1{ppserver_mtp_};
                std::unique_lock l2{cq_mtp_};
                if(!cq_) {
                    cq_ = std::make_unique<command_queue>(std::thread::hardware_concurrency());
                    cq_->set_workload_to_start_spawn_threads(0.9);
                    cq_->set_workload_to_start_kill_threads(0.1);
                    cq_->set_min_seconds_between_killings(1);
                }
            }
            std::shared_lock l{ppserver_mtp_};
            pp_subs_functor_terminate_ = false;
            if(!pp_subs_functor_running()) {
                cq_->enqueue_urgent(pp_subs_functor_);
            }
            if(!ppserver_) {
                l.unlock();
                std::unique_lock l1{ppserver_mtp_};
                if(!ppserver_) {
                    ppserver_ = std::make_unique<pp_server_udp>(cq_.get(), af, stale_connections_removal_timeout);
                    ppserver_->set_on_data_arrived([this](conn_id_t conn_id, bytevec const &data) {
                        json requ{json::bdeserialize(data)};
                        auto act{requ["act"].as_string()};
                        json resp{};
                        if(act == "sub") {
                            auto nme{requ["name"].as_string()};
                            auto ali{requ["alias"].as_string()};
                            resp["act"] = "sub";
                            resp["name"] = ali;
                            pp_subscribe(conn_id, nme, ali);
                        }
                    });
                }
            }
            if(!ppserver_->started()) {
                ppserver_->start(bind_addr, port, 1);
            }
        }

        void stop_net_server() override {
            std::unique_lock l{ppserver_mtp_};
            pp_subs_functor_terminate_ = true;
            ppserver_.reset();
        }

        bool net_server_running() const override {
            std::shared_lock l{ppserver_mtp_};
            return ppserver_->started();
        }

        void set_external_cells_update_interval(long double seconds) override {
            ext_cells_refresh_interval_nanos_ = seconds * 1'000'000'000.0L;
        }

        long double external_cells_update_interval() const override {
            return static_cast<long double>(ext_cells_refresh_interval_nanos_) / 1'000'000'000.0L;
        }

        // TODO: implement this and the standalone distributed service
        void net_hub_connect(std::string const &/*host_addr*/, std::uint16_t /*port*/, std::string const &/*unique_net_name*/) override {
        }

    private:
        void load_source_string(std::string const &src) {
            lexer lxr{};
            parser prs{};
            lxr.set_callback([&](token const &tkn, bool /*space_with_nl*/) {
                if(tkn.tktype() != token::type::SPACE) {
                    prs.add_token(tkn);
                }
            });
            for(auto &&c: str_util::from_utf8(src)) {
                lxr.consume_char(c);
            }
            lxr.consume_eof();
            auto ast{prs.parse()};

            code_generator lj{};
            lj.chop(
                ast, input_cells_, input_names_to_instances_mapping_, worker_cells_templates_,
                worker_cells_, worker_bodies_, user_functions_, global_functions_dictionary_, extern_cells_
            );
        }

        bool load_library(std::string const &fname) {
            bool res{false};
            try {
                std::shared_ptr<so> dll_ptr{std::make_shared<so>(fname)};
                if(dll_ptr->ok()) {
                    auto ld_fn{dll_ptr->symbol<extension_interface *(*)()>("create_teal_extension")};
                    auto unld_fn{dll_ptr->symbol<void (*)(extension_interface *)>("remove_teal_extension")};
                    if(ld_fn && unld_fn) {
                        extension_interface *ext{ld_fn()};
                        if(ext) {
                            ext->register_runtime(this);
                            std::unique_lock l{loaded_extensions_mtp_};
                            loaded_extensions_.emplace_back(std::move(dll_ptr), ext);
                            res = true;
                        }
                    }
                }
            } catch(...) {
            }
            return res;
        }

        void unload_extensions() {
            std::unique_lock l{loaded_extensions_mtp_};
            for(auto &&ep: loaded_extensions_) {
                ep.second->unregister_runtime();
                ep.first->symbol<void (*)(extension_interface *)>("remove_teal_extension")(ep.second);
                ep.first->close();
            }
            loaded_extensions_.clear();
        }

    private:
        std::function<std::size_t(valbox const &)> sizeof_func_{
            [&](valbox const &vb) {
                valbox const &der{vb.deref()};
                switch(der.val_or_pointed_type()) {
                    case valbox::type::BOOL: return static_cast<std::size_t>(sizeof(bool));
                    case valbox::type::CHAR: return static_cast<std::size_t>(sizeof(char));
                    case valbox::type::S8: return static_cast<std::size_t>(sizeof(std::int8_t));
                    case valbox::type::U8: return static_cast<std::size_t>(sizeof(std::uint8_t));
                    case valbox::type::S16: return static_cast<std::size_t>(sizeof(std::int16_t));
                    case valbox::type::U16: return static_cast<std::size_t>(sizeof(std::uint16_t));
                    case valbox::type::WCHAR: return static_cast<std::size_t>(sizeof(wchar_t));
                    case valbox::type::S32: return static_cast<std::size_t>(sizeof(std::int32_t));
                    case valbox::type::U32: return static_cast<std::size_t>(sizeof(std::uint32_t));
                    case valbox::type::S64: return static_cast<std::size_t>(sizeof(std::int64_t));
                    case valbox::type::U64: return static_cast<std::size_t>(sizeof(std::uint64_t));
                    case valbox::type::FLOAT: return static_cast<std::size_t>(sizeof(float));
                    case valbox::type::DOUBLE: return static_cast<std::size_t>(sizeof(double));
                    case valbox::type::LONG_DOUBLE: return static_cast<std::size_t>(sizeof(long double));
                    case valbox::type::VEC4: return static_cast<std::size_t>(sizeof(math::vector4<long double>));
                    case valbox::type::MAT4: return static_cast<std::size_t>(sizeof(math::matrix4<long double>));
                    case valbox::type::POINTER: return static_cast<std::size_t>(sizeof(void *));
                    case valbox::type::CLASS: return static_cast<std::size_t>(1);
                    case valbox::type::FUNC: return static_cast<std::size_t>(1);
                    case valbox::type::ARRAY: {
                        std::size_t res{};
                        for(auto &&v: der.as_array()) {
                            res += sizeof_func_(v);
                        }
                        return res;
                    }
                    case valbox::type::OBJECT: {
                        std::size_t res{};
                        for(auto &&v: der.as_object()) {
                            res += v.first.size() + sizeof_func_(v.second);
                        }
                        return res;
                    }
                    case valbox::type::STRING: return static_cast<std::size_t>(der.as_string().size());
                    case valbox::type::WSTRING: return static_cast<std::size_t>(der.as_wstring().size() * sizeof(std::wstring::value_type));
                    case valbox::type::UNDEFINED: return static_cast<std::size_t>(1);
                    default: return static_cast<std::size_t>(1);
                }
            }
        };

    private:
        console con_{this};

        std::atomic<std::int64_t> failure_{0};
        mutable shared_mutex failure_mtp_{};
        std::string failure_description_{};

        std::atomic<std::int64_t> termination_requested_{0};
        enum class thread_mode{none, single, multi };
        thread_mode thread_mode_{thread_mode::none};
        void set_thread_mode_single() { thread_mode_ = thread_mode::single; }
        void set_thread_mode_multi() { thread_mode_ = thread_mode::multi; }
        void set_thread_mode_none() { thread_mode_ = thread_mode::none; }
        bool is_current_thread_mode_none() const { return thread_mode_ == thread_mode::none; }
        bool is_current_thread_mode_single() const { return thread_mode_ == thread_mode::single; }
        bool is_current_thread_mode_multi() const { return thread_mode_ == thread_mode::multi; }
        std::uint64_t sleep_between_cycles_nanoseconds_{0};
        std::uint64_t sleep_inactive_thread_nanoseconds_{100'000ULL};

        execution_context exctx_{};
        shared_mutex threads_mtp_{};
        std::list<std::thread> threads_{};
        std::int64_t wait_granularity_nsec_{100LL};

        str_map_t<valbox> global_constants_dictionary_{
            {"M_E", valbox{2.7182818284590452353602874713526624977572470936999595749669676277240766L}},
            {"M_PI", valbox{3.1415926535897932384626433832795028841971693993751058209749445923078164L}},
            {"M_PHI", valbox{1.6180339887498948482045868343656381177203091798057628621354486227052605L}},
        };
        str_map_t<valbox> global_functions_dictionary_{};
        str_map_t<str_map_t<valbox>> global_methods_dictionary_{};
        shared_mutex input_cells_mtp_{};
        str_map_t<std::shared_ptr<input_cell>> input_cells_{};
        str_map_t<std::string> input_names_to_instances_mapping_{};
        shared_mutex outputs_mtp_{};
        mutable str_map_t<valbox> outputs_{};
        mutable shared_mutex workers_mtp_{};
        str_map_t<worker_cell_definition_info> worker_cells_templates_{};
        str_map_t<std::shared_ptr<worker_cell_instance>> worker_cells_{};
        str_map_t<statement_ptr> worker_bodies_{};
        str_map_t<function_definition> user_functions_{};
        std::function<bool(std::string)> user_functions_search_{
            [this](std::string const &fname) -> bool {
                auto it{user_functions_.find(fname)};
                return it != user_functions_.end();
            }
        };
        std::function<valbox(std::vector<valbox> &)> user_function_selector_{
            [this](std::vector<valbox> &fargs) -> valbox {
                execution_context *exctx{reinterpret_cast<execution_context *>(fargs[0].as_ptr())};

                typename str_map_t<function_definition>::const_iterator it{user_functions_.find(fargs[1].as_string())};

                if(it == user_functions_.end()) {
                    throw std::runtime_error{"function not found"};
                }

                function_definition const &fdef{it->second};

                exctx->push_frame_ignore();
                shut_on_destroy leave_frame_ign{[exctx]() { exctx->pop_frame_ignore(); }};

                exctx->new_stack_frame();
                shut_on_destroy leave_frame{[exctx]() {
                    exctx->clear_return_request();
                    exctx->del_stack_frame();
                }};

                std::vector<std::string> const &anames{fdef.arg_names()};
                auto fargs_size{fargs.size()};
                for(std::size_t i{0}; i < anames.size(); ++i) {
                    if(i >= fargs_size) {
                        exctx->set_local_value(anames[i], valbox{valbox_no_initialize::dont_do_it});
                    } else {
                        exctx->set_local_value(anames[i], fargs[i + 2]);
                    }
                }

                fdef.body()->exec(exctx);
                if(exctx->return_requested()) {
                    return exctx->return_result();
                }

                return valbox{valbox_no_initialize::dont_do_it};
            }
        };
        int64_t exit_status_{};
        std::size_t programmatic_termination_enabled_{1};

        valbox find_func(std::string const &name) const {
            valbox fn{};
            if((user_functions_search())(name)) {
                fn = valbox{user_function_selector(), name, true};
                return fn;
            }
            auto gvd_it{global_functions_dictionary()->find(name)};
            if(gvd_it != global_functions_dictionary()->end()) {
                fn = gvd_it->second;
                return fn;
            }
            return fn;
        }

        math_ext math_ext_{};
        time_ext time_ext_{};
        crypto_ext crypt_{};
        file_ext fpool_{};
        cpu_ext perf_stat_{};
        rand_ext randlib_{};
        array_buffer_ext array_buffer_ext_{};
        socket_ext sockext_{};
        containers_ext dict_ext_{};
#ifndef TEALSCRIPT_NO_EIGEN
        eigen_ext eigen_ext_{};
#endif
        mutable shared_mutex obj_ser_mtp_{};
        std::map<std::string, obj_services> obj_svc_{};
        friend class valbox;

        shared_mutex loaded_extensions_mtp_{};
        std::list<std::pair<std::shared_ptr<so>, extension_interface *>> loaded_extensions_{};
        static std::size_t constexpr version_major_{1};
        static std::size_t constexpr version_minor_{5};
        static std::size_t constexpr version_patch_{14};

        mutable shared_mutex cq_mtp_{};
        std::unique_ptr<command_queue> cq_{};

        std::string network_access_point_url_{};
        std::string network_name_{};
        mutable shared_mutex extern_cells_mtp_{};
        str_map_t<std::shared_ptr<extern_cell>> extern_cells_{};
        mutable shared_mutex ext_cells_processor_mtp_{};
        std::atomic<bool> ext_cells_processor_started_{false};
        bool ext_cells_processor_enabled_{true};
        std::thread ext_cells_processor_{};

        mutable shared_mutex pp_clients_mtp_{};
        std::map<std::string, std::map<int, std::shared_ptr<pp_client_udp>>> pp_clients_{};

        std::shared_ptr<pp_client_udp> get_connected_client(extern_cell const *ecp) {
            std::unique_lock l{pp_clients_mtp_};
            std::shared_ptr<pp_client_udp> res{pp_clients_[ecp->remote_host()][*ecp->remote_url().port()]};
            if(!res || !res->connected() || res->seconds_with_no_data_arrivals() > 10) {
                res = std::make_shared<pp_client_udp>(cq_.get());
                res->set_on_data_arrived([this](bytevec const &rsp) {
                    try {
                        json resp{json::bdeserialize(rsp)};
                        auto act{resp["act"].as_string()};
                        if(act == "pub") {
                            auto nme{resp["name"].as_string()};
                            auto it{extern_cells_.find(nme)};
                            if(it != extern_cells_.end()) {
                                valbox desin{ valbox_no_initialize::dont_do_it};
                                desin.deserialize(resp["result"], this);
                                it->second->set_value(desin);
                            }
                        }
                    } catch (...) {
                    }
                });
                res->start(ecp->remote_host(), *ecp->remote_url().port());
                if(!res->connected()) {
                    res.reset();
                }
                pp_clients_[ecp->remote_host()][*ecp->remote_url().port()] = res;
            }
            return res;
        }

        bool extcell_processing_started() const {
            return ext_cells_processor_started_.load(std::memory_order_relaxed);
        }

        void start_extcell_processing() {
            if(extcell_processing_started()) {
                return;
            }

            {
                std::unique_lock l1{cq_mtp_};
                if(!cq_) {
                    cq_ = std::make_unique<command_queue>(std::thread::hardware_concurrency());
                    cq_->set_workload_to_start_spawn_threads(0.9);
                    cq_->set_workload_to_start_kill_threads(0.1);
                    cq_->set_min_seconds_between_killings(1);
                }
            }

            std::unique_lock l{ext_cells_processor_mtp_};

            ext_cells_processor_enabled_ = false;
            if(ext_cells_processor_.joinable()) {
                ext_cells_processor_.join();
            }

            ext_cells_processor_enabled_ = true;
            ext_cells_processor_ = std::thread{[this]() {
                long double last_sub_time{0};
                while(ext_cells_processor_enabled_) {
                    if(!extern_cells_.empty()) {
                        if(steady_time_sec() > last_sub_time + 5) {
                            last_sub_time = steady_time_sec();
                            std::shared_lock l{extern_cells_mtp_};
                            for(auto cp: extern_cells_) {
                                json requ{};
                                requ["act"] = "sub";
                                requ["name"] = cp.second->remote_var_name();
                                requ["alias"] = cp.second->inst_name();
                                if(auto con{get_connected_client(cp.second.get())}) {
                                    con->send(requ.bserialize());
                                }
                            }
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds{10});
                }
                ext_cells_processor_started_ = false;
            }};
            ext_cells_processor_started_ = true;
        }

        void stop_extcell_processing() {
            {
                std::unique_lock l{ext_cells_processor_mtp_};
                ext_cells_processor_enabled_ = false;
                if(ext_cells_processor_.joinable()) {
                    ext_cells_processor_.join();
                }
                ext_cells_processor_started_ = false;
            }
            {
                std::unique_lock l1{pp_clients_mtp_};
                pp_clients_.clear();
            }
            {
                std::shared_lock l{ppserver_mtp_};
                std::unique_lock l1{cq_mtp_};
                if(!ppserver_) {
                    cq_.reset();
                }
            }
        }

        mutable shared_mutex ppserver_mtp_{};
        std::unique_ptr<pp_server_udp> ppserver_{nullptr};
        class net_value_subscriber {
            struct name_attrs;

        public:
            void subscribe(std::string const &name, std::string const &alias) {
                names_[name] = {alias, {}, 0}; actualize_input_activity();
            }
            void unsubscribe(std::string const &name) { names_.erase(name); actualize_input_activity(); }
            void actualize_input_activity() { last_incoming_activity_time_ = steady_time_sec(); }
            long double seconds_that_side_inactive() const {
                return steady_time_sec() - last_incoming_activity_time_;
            }
            void actualize_send_val(std::string const &n, std::vector<std::uint8_t> const &v) {
                names_[n].val = v;
            }
            void actualize_send_time(std::string const &n, long double t) {
                names_[n].last_sent_timestamp_ = t;
            }
            std::vector<std::pair<std::string, name_attrs>> names() const {
                return std::vector<std::pair<std::string, name_attrs>>{names_.begin(), names_.end()};
            }

        private:
            long double last_incoming_activity_time_{steady_time_sec()};
            struct name_attrs{ std::string alias; std::vector<std::uint8_t> val; long double last_sent_timestamp_; };
            std::map<std::string, name_attrs> names_{};
        };

        mutable std::shared_mutex net_subs_mtp_{};
        std::atomic_bool pp_subs_functor_running_{false};
        std::atomic_bool pp_subs_functor_terminate_{false};
        std::map<conn_id_t, net_value_subscriber> net_subs_{};
        uint64_t ext_cells_refresh_interval_nanos_{1000000ULL};

        void pp_subscribe(conn_id_t conn_id, std::string const &name, std::string const &alias) {
            std::unique_lock l{net_subs_mtp_};
            if(!pp_subs_functor_terminate_) {
                net_subs_[conn_id].subscribe(name, alias);
            }
        }
        void pp_unsubscribe(conn_id_t conn_id, std::string const &name) {
            std::unique_lock l{net_subs_mtp_};
            if(!pp_subs_functor_terminate_) {
                net_subs_[conn_id].unsubscribe(name);
            }
        }
        void pp_remove(conn_id_t conn_id) {
            std::unique_lock l{net_subs_mtp_};
            if(!pp_subs_functor_terminate_) {
                net_subs_.erase(conn_id);
            }
        }

        bool pp_subs_functor_running() const {
            std::shared_lock l{net_subs_mtp_};
            return pp_subs_functor_running_;
        }

        std::function<void()> pp_subs_functor_{[this]() {
            {
                std::unique_lock l{net_subs_mtp_};
                pp_subs_functor_running_ = true;
                for(auto it{net_subs_.begin()}; it != net_subs_.end(); ) {
                    if(it->second.seconds_that_side_inactive() > 10) {
                        it = net_subs_.erase(it);
                        continue;
                    }
                    auto name_pairs{it->second.names()};
                    for(auto &&np: name_pairs) {
                        try {
                            json resp{};
                            cell_base *cellptr{nullptr};
                            auto const &nme{np.first};
                            auto &v{np.second};
                            resp["act"] = "pub";
                            resp["name"] = v.alias;
                            std::vector<std::uint8_t> last_value{v.val};
                            do {
                                {
                                    auto it{input_cells_.find(nme)};
                                    if(it != input_cells_.end()) { cellptr = it->second.get(); break; }
                                }
                                {
                                    auto it{worker_cells_.find(nme)};
                                    if(it != worker_cells_.end()) { cellptr = it->second.get(); break; }
                                }
                                {
                                    auto it{extern_cells_.find(nme)};
                                    if(it != extern_cells_.end()) { cellptr = it->second.get(); break; }
                                }
                            } while(false);
                            bool need_send{false};
                            if(cellptr != nullptr) {
                                json vbsr{cellptr->value_clone().serialize(this)};
                                std::vector<std::uint8_t> bs{vbsr.bserialize()};
                                if(last_value != bs) {
                                    need_send = true;
                                    it->second.actualize_send_val(nme, bs);
                                    it->second.actualize_send_time(nme, steady_time_sec());
                                    resp["result"] = std::move(vbsr);
                                }
                            } else {
                                if(steady_time_sec() > v.last_sent_timestamp_ + 3) {
                                    need_send = true;
                                    it->second.actualize_send_time(nme, steady_time_sec());
                                    resp["result"] = valbox{}.serialize(this);
                                }
                            }
                            if(need_send) {
                                ppserver_->send(it->first, resp.bserialize());
                            }
                        } catch (...) {
                        }
                    }
                    ++it;
                }
            }
            if(ext_cells_refresh_interval_nanos_ > 0) {
                std::this_thread::sleep_for(std::chrono::nanoseconds{ext_cells_refresh_interval_nanos_});
            }
            if(!termination_requested() && !pp_subs_functor_terminate_) {
                cq_->enqueue_urgent(pp_subs_functor_);
            } else {
                std::unique_lock l{net_subs_mtp_};
                net_subs_.clear();
                pp_subs_functor_running_ = false;
            }
        }};
    };

}
