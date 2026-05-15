# TealScript

TealScript is an embedded scripting engine. Lightweight, easy to integrate, intended to create control logic. Distributed, but free from heavy message brokers (ROS/MQTT). Free from garbage collection overhead, with native C++ integration, designed to seamlessly wire physical and virtual devices into a unified control system. The language in data-flow graph paradigm enforces clear declarative rules for building schemas, making complex logic highly readable and maintainable. Combined with seamless C++ host extensibility, the result is a deterministic, cohesive hardware-software ecosystem driven entirely by data-flow graphs.


## Stateful Data-Flow

While based on the discrete-time, clocked data-flow paradigm (similar to Unreal Engine Blueprints), TealScript departs from strict functional purity. Each compute node is an instance of an object that syntactically resembles a function, but can retain state between execution cycles via instance variables accessible through the this keyword. This makes writing complex state machines or PID controllers as easy as writing simple functions.


## Why TealScript?

When designing control logic, it is tempting to specify what the outcome should be, rather than describe how to achieve it step-by-step in detail. While C++ is imperative and requires detailed architectural design, TealScript allows you to broaden your programming approaches without changing your C++ toolchain. 

You get a problem-specific tool to handle complex control schemes for multiple actuators based on numerous sensor signals, drastically reducing the low-level C++ boilerplate required for wiring, state management, and multi-threading, while keeping full native extensibility.


## Key Features
 * C-like sytax, most of C math functions provided.
 * Very thoughtful, maximally intuitive syntax in order to provide a full range of capabilities while maintaining conciseness and readability of the program.
 * True Multi-Threading: Execute graph schemas in parallel across available CPU cores. The interpreter safely handles node execution without requiring the user to manage C++ threads or locks.
 * Zero Dependencies & Portable: Implemented as a custom execution tree interpreter (no LLVM/external lexers). It compiles into any C++20 codebase via CMake and is completely hardware-agnostic.
 * Turing Complete & Extensible: Handle general-purpose tasks, math (functions, matrices), JSON, and custom host-provided types. Easily inject host functions into the scripting runtime.
 * Network-Agnostic Distributed Graphs: Seamlessly link variables across different hosts using extern URIs. Built on a custom UDP multiplexing protocol (MTU-safe 1400 bytes) that eliminates Head-of-Line blocking without the overhead of TCP or heavy brokers like MQTT (see [example script](examples/external_value.teal)).


## Quick Example

A logical schema written in TealScript manages physical actuators in parallel by analyzing signals from input devices:

```TealScript
pass(v) return v;

// Configuration for host code
pass friction_force(.5) 'friction_force';
pass cart_mass(1.0) 'cart_mass';
pass pend_mass(0.3) 'pend_mass';
pass start_force_impulse(0.5) 'start_force_impulse';

// PID balancing
balance_pid(angle, dt) {
    if(this.p_error == undefined) {
        this.p_error = 0.0;
        this.i_error = 0.0;
    }
    prev_ang = this.p_error;
    this.p_error = angle;
    this.i_error = dt * this.i_error + angle;
    d_error = (angle - prev_ang) / dt;
    return 60.0 * this.p_error + 20.0 * this.i_error + 20.0 * d_error;
}

// Lazy centering
center_pid(cart_pos, cart_vel) {
    return 4.0 * sqrt(abs(cart_pos)) * sign(cart_pos) + 
           4.0 * sqrt(abs(cart_vel)) * sign(cart_vel);
}

// "Soft wall"
soft_wall(cp) {
    return abs(cp) > 4.5 ? sign(cp) * 8.0 : 0.0;
}

mixer(balance_force, center_force, wall_force) {
    return balance_force + center_force + wall_force;
}

// ---------------------------------------------------------
// The Graph
// ---------------------------------------------------------

// Inputs from host code
'dt' dt;
'ang' angle;
'cart_pos' cart_pos;
'cart_vel' cart_vel;

// Worker nodes
balance_pid balancer(angle, dt);
center_pid centerer(cart_pos, cart_vel);
soft_wall wall(cart_pos);
mixer motor_control(balancer, centerer, wall) 'motor_force';
```

[![Watch the video](https://github.com/dumitrioo/tealscript/blob/main/resources/pid_demo.png)](https://github.com/dumitrioo/tealscript/blob/main/resources/pid_demo.mp4)

https://github.com/dumitrioo/tealscript/blob/main/resources/pid_demo.mp4

[Example application](examples/pendulum/main.cpp)

## Another Example

How would you implement the following logic circuit in a regular programming language?

![Logic cirquit for 74181](examples/alu74181.png)

And while you are thinking, I will offer you a ([solution](examples/alu74181.teal)) in a TealScript programming language.

```TealScript
// ---------------------------------------------------------
// logic gates simulation with
// TealScript computation nodes templates
// ---------------------------------------------------------
not(a) { return !a; }
fwd(a) { return (bool)a; }
and2(a, b) { return a && b; }
and3(a, b, c) { return a && b && c; }
and4(a, b, c, d) { return a && b && c && d; }
and5(a, b, c, d, e) { return a && b && c && d && e; }
nand2(a, b) { return !(a && b); }
nand3(a, b, c) { return !(a && b && c); }
nand4(a, b, c, d) { return !(a && b && c && d); }
nand5(a, b, c, d, e) { return !(a && b && c && d && e); }
or2(a, b) { return a || b; }
or3(a, b, c) { return a || b || c; }
or4(a, b, c, d) { return a || b || c || d; }
nor2(a, b) { return !(a || b); }
nor3(a, b, c) { return !(a || b || c); }
nor4(a, b, c, d) { return !(a || b || c || d); }
xor2(a, b) { return a ^ b; }
xor3(a, b, c) { return a ^ b ^ c; }
xor4(a, b, c, d) { return a ^ b ^ c ^ d; }
xnor2(a, b) { return !(a ^ b); }
xnor3(a, b, c) { return !(a ^ b ^ c); }
xnor4(a, b, c, d) { return !(a ^ b ^ c ^ d); }
i2or2(a, b) { return !a || !b; }

// ---------------------------------------------------------
// The Graph
// ---------------------------------------------------------

// inputs --------------------------------------------------
'A0' a0;
'A1' a1;
'A2' a2;
'A3' a3;
'B0' b0;
'B1' b1;
'B2' b2;
'B3' b3;
'S0' s0;
'S1' s1;
'S2' s2;
'S3' s3;
'C_in' c_in;
'M' m;

// workers -------------------------------------------------
not   alu_0(b3);
not   alu_1(b2);
not   alu_2(b1);
not   alu_3(b0);
not   alu_4(m);
and3  alu_5(b3, s3, a3);
and3  alu_6(a3, s2, alu_0);
and2  alu_7(alu_0, s1);
and2  alu_8(s0, b3);
fwd   alu_9(a3);
and3  alu_10(b2, s3, a2);
and3  alu_11(a2, s2, alu_1);
and2  alu_12(alu_1, s1);
and2  alu_13(s0, b2);
fwd   alu_14(a2);
and3  alu_15(b1, s3, a1);
and3  alu_16(a1, s2, alu_2);
and2  alu_17(alu_2, s1);
and2  alu_18(s0, b1);
fwd   alu_19(a1);
and3  alu_20(b0, s3, a0);
and3  alu_21(a0, s2, alu_3);
and2  alu_22(alu_3, s1);
and2  alu_23(s0, b0);
fwd   alu_24(a0);
nor2  alu_25(alu_5, alu_6);
nor3  alu_26(alu_7, alu_8, alu_9);
nor2  alu_27(alu_10, alu_11);
nor3  alu_28(alu_12, alu_13, alu_14);
nor2  alu_29(alu_15, alu_16);
nor3  alu_30(alu_17, alu_18, alu_19);
nor2  alu_31(alu_20, alu_21);
nor3  alu_32(alu_22, alu_23, alu_24);
xor2  alu_33(alu_25, alu_26);
xor2  alu_34(alu_27, alu_28);
xor2  alu_35(alu_29, alu_30);
xor2  alu_36(alu_31, alu_32);
fwd   alu_37(alu_26);
and2  alu_38(alu_25, alu_28);
and3  alu_39(alu_25, alu_27, alu_30);
and4  alu_40(alu_25, alu_27, alu_29, alu_32);
nand5 alu_41(alu_25, alu_27, alu_32, alu_31, c_in);
nand4 alu_42(alu_25, alu_27, alu_32, alu_31)          'P';
and5  alu_43(c_in, alu_31, alu_32, alu_27, alu_4);
and4  alu_44(alu_32, alu_27, alu_32, alu_4);
and3  alu_45(alu_27, alu_30, alu_4);
and2  alu_46(alu_28, alu_4);
and4  alu_47(c_in, alu_31, alu_29, alu_4);
and3  alu_48(alu_29, alu_32, alu_4);
and2  alu_49(alu_30, alu_4);
and3  alu_50(c_in, alu_31, alu_4);
and2  alu_51(alu_32, alu_4);
nand2 alu_52(c_in, alu_4);
nor4  alu_53(alu_37, alu_38, alu_39, alu_40)          'G';
nor4  alu_54(alu_43, alu_44, alu_45, alu_46);
nor3  alu_55(alu_47, alu_48, alu_49);
nor2  alu_56(alu_50, alu_51);
i2or2 alu_57(alu_53, alu_41)                          'C_out';
xor2  alu_58(alu_33, alu_54)                          'f3';
xor2  alu_59(alu_34, alu_55)                          'f2';
xor2  alu_60(alu_35, alu_56)                          'f1';
xor2  alu_61(alu_36, alu_52)                          'f0';
and4  alu_62(alu_58, alu_59, alu_60, alu_61)          'EQ';
```

As you can see, the graph is composed of computation node instances whose implementations described above that graph. And the program itself turned out to be short, understandable and covering the logical circuit one to one.



## Application & Use Cases

TealScript excels at "sense → compute → act" pipelines: 

 * Robotics & Autonomous Systems: High-throughput signal processing, sensor fusion, and driving actuators in real time.
 * Industrial Automation: Replacing heavy PLC logic with portable C++ integrations.
 * Edge Computing / IoT: Orchestration of distributed controllers without centralized message brokers.
 * Simulations & Digital Twins: Logical linking of separately attached hardware or virtual devices.
     

## Usage & Integration

The usage of library is as simple as following:

1. Include [tealscript_runtime.hpp](src/tealscript_runtime.hpp) header file
2. Instantiate the TealScript runtime object and load scripting sources (from strings or files) into the runtime.
3. Optionally, add your custom C++ functions, variables, types (if any) into the runtime instance.
4. Execute runtime in single/multi-threaded mode exchanging data.


```C++
#include <tealscript_runtime.hpp>

// This example is related to the script example shown above

int main(int argc, char **argv) {
    if(argc < 2) {
        return 0;
    }

    // tealscript runtime instance 
    teal::runtime rt{};

    // loading script file passed from command line
    rt.load_file(argv[1]);
    rt.loading_complete();
    
    ...
    ...
    
    while(simulationInProgress) {
        ...
        ...
        // Sensors update
        rt.set_input("dt", dt);
        rt.set_input("ang", angle);
        rt.set_input("cart_pos", cart_x);
        rt.set_input("cart_vel", cart_vel);

        // Compute in the same thread (single-threaded execution)
        rt.run_cycle();
        
        // Fetch controllong value from runtime
        force = rt.get_output("motor_force").cast_to_double();
        
        // Apply script results
        cartBody->applyCentralForce(btVector3(force, 0, 0));
        ...
        ...
    }

    return 0;
}
```

To build the examples (on Linux): 
``` bash 
mkdir build && cd build
cmake ..
make
```

Or use a provided shell script

``` bash 
git clone https://github.com/dumitrioo/tealscript.git
cd tealscript
./build_linux_cmake.sh
```

Explore the [examples](examples/), directory for advanced use cases like the ALU 74181 hardware simulation  and host-side C++ extensions. 


## More Information

For the complete language specification, type system, and host API reference, see the [Documentation](doc/tealscript_overview.pdf). (PDF - Migrating to Markdown docs is in progress).
