#include <clocale>
#include <vector>
#include <thread>
#include <filesystem>
#include <iostream>

#include <tealscript_runtime.hpp>

// Just a regular C++ class to be added as an <<object type>> to the scripting runtime
class example_object {
public:
    example_object() = default;
    example_object(int v): v_{v} {}
    void set_val(int v) { v_ = v; }
    int get_val() const { return v_; }

private:
    int v_{};
};

#ifdef USE_CUSTOM_MEMORY_ALLOCATION

static teal::binned_allocator<512 * 1024 * 1024, 16, 4096> glbl_alloc{};

void* operator new(std::size_t sz) {
    if(sz == 0) { ++sz; }

    if(void *ptr = glbl_alloc.allocate(sz)) {
        return ptr;
    }

    throw std::bad_alloc{};
}

void* operator new[](std::size_t sz) {
    if(sz == 0) { ++sz; }
    if(void *ptr = glbl_alloc.allocate(sz)) {
        return ptr;
    }
    throw std::bad_alloc{};
}

void operator delete(void* ptr) noexcept {
    glbl_alloc.deallocate(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    glbl_alloc.deallocate(ptr, size);
}

void operator delete[](void* ptr) noexcept {
    glbl_alloc.deallocate(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    glbl_alloc.deallocate(ptr, size);
}

#endif

int main(int argc, char **argv) {
    std::vector<std::string> args{argv, argv + argc};
    std::setlocale(LC_ALL, "en_US.UTF-8");

    if(args.size() < 2) {
        return 0;
    }

#if defined(SIGPIPE)
    std::signal(SIGPIPE, SIG_IGN);
#endif

    // The runtime
    teal::runtime rt{};

    // -----------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    // The host part of scripting language possibilities extending example.
    // For usage, see script "examples/extending_example.teal".
    // -----------------------------------------------------------------------------------
    // Example of adding function to the runtime
    rt.add_function("hello_from_cpp",
        TEALFUN(args) {
            std::cout << "C++ extension function hello_from_cpp() called with arguments:" << std::endl;
            for(auto &&a: args) {
                std::cout << "\t" << a << std::endl;
            }
            return args.size();
        }
    );

    // -----------------------------------------------------------------------------------
    // Example of adding named value to the runtime
    rt.add_var("The_Answer_to_the_Ultimate_Question_of_Life_the_Universe_and_Everything", 42);

    // -----------------------------------------------------------------------------------
    // Example of adding object type to the runtime
    rt.add_function("example_object",
        TEALFUN(args) {
            if(args.size() > 0) {
                return teal::valbox{example_object{args[0].cast_to_s32()}, "example_object"};
            }
            return teal::valbox{example_object{}, "example_object"};
        }
    );
    rt.add_method("example_object", "set_val", TEALFUN(args) {
        // check number of arguments, when needed, including
        // implicit object reference as the first arg
        TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 2)
        if(!args[1].is_numeric()) {
            throw std::runtime_error{"the value must be of numeric type"};
        }
        TEALTHIS(args, example_object).set_val(args[1].cast_to_s32());
        return 0;
    });
    rt.add_method("example_object", "get_val", TEALFUN(args) {
        TEAL_CHCK_FUN_PARMS_NUM_EQ(args, 1)
        return TEALTHIS(args, example_object).get_val();
    });
    // -----------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------
    // -----------------------------------------------------------------------------------

#ifndef TEAL_DEBUGGING
    try {
#endif
        for(std::size_t i{1}; i < args.size(); ++i) {
            if(std::filesystem::is_regular_file(args[i])) {
                rt.load_file(args[i]);
            } else if(std::filesystem::is_directory(args[i])) {
                for(auto const &dir_entry: std::filesystem::recursive_directory_iterator{args[i]}) {
                    if(dir_entry.is_regular_file()) {
                        rt.load_file(dir_entry.path());
                    }
                }
            } else {
                throw std::runtime_error{args[i] + " - no such file or directory"};
            }
        }
        rt.loading_complete();

        if(rt.worker_cells_count() == 0) {
            throw std::runtime_error{"nothing to do - no working elements"};
        }

        rt.start_net_server(teal::net::address_family::inet4, "0.0.0.0", 43987, 0);
        rt.set_external_cells_update_interval(0.001L);

#ifdef TEAL_SINGLE_THREADED
        while(!rt.termination_requested()) {
            rt.run_cycle();
        }
#else
        rt.run_mt(std::thread::hardware_concurrency());
        while(!rt.wait(0.1)) {
        }
        if(rt.failure_occured()) { throw std::runtime_error{rt.failure_description()}; }
#endif

#ifndef TEAL_DEBUGGING
    } catch(std::exception const &e) {
        std::cerr << "error: " << e.what() << std::endl;
    }
#endif

    return rt.exit_status();
}
