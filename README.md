# TealScript

TealScript is a lightweight, distributed, and deterministic control logic engine - free from heavy message brokers (ROS/MQTT) and garbage collection overhead, with native C++ integration.

It is an embedded scripting language designed to seamlessly wire physical and virtual devices into a unified control system. The language enforces strict, declarative rules for building schemas, making complex logic highly readable and maintainable. Combined with seamless C++ host extensibility, the result is a deterministic, cohesive hardware-software ecosystem driven entirely by data-flow graphs.


## Stateful Data-Flow

While based on the discrete-time, clocked data-flow paradigm (similar to Unreal Engine Blueprints), TealScript departs from strict functional purity. Each compute node is an instance of an object that syntactically resembles a function, but can retain state between execution cycles via instance variables accessible through the this keyword. This makes writing complex state machines or PID controllers as easy as writing simple functions.


## Why TealScript?

When designing control logic, it is tempting to specify what the outcome should be, rather than script how to achieve it step-by-step. While C++ is imperative and requires detailed architectural design, TealScript allows you to broaden your programming approaches without changing your C++ toolchain. 

You get a problem-specific tool to handle complex control schemes for multiple actuators based on numerous sensor signals, drastically reducing the low-level C++ boilerplate required for wiring, state management, and multi-threading, while keeping full native extensibility.

## Key Features

 * Network-Agnostic Distributed Graphs: Seamlessly link variables across different hosts using extern URIs. Built on a custom UDP multiplexing protocol (MTU-safe 1400 bytes) that eliminates Head-of-Line blocking without the overhead of TCP or heavy brokers like MQTT (see [example script](examples/external_value.teal)).
 * True Multi-Threading: Execute graph schemas in parallel across available CPU cores. The interpreter safely handles node execution without requiring the user to manage C++ threads or locks.
 * Zero Dependencies & Portable: Implemented as a custom execution tree interpreter (no LLVM/external lexers). It compiles into any C++20 codebase via CMake and is completely hardware-agnostic.
 * Turing Complete & Extensible: Handle general-purpose tasks, math (functions, matrices), JSON, and custom host-provided types. Easily inject host functions into the scripting runtime.


## Quick Example

A logical schema written in TealScript manages physical actuators in parallel by analyzing signals from input devices:

```TealScript
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

## Application & Use Cases

TealScript excels at "sense → compute → act" pipelines: 

 * Robotics & Autonomous Systems: High-throughput signal processing, sensor fusion, and driving actuators in real time.
 * Industrial Automation: Replacing heavy PLC logic with portable C++ integrations.
 * Edge Computing / IoT: Orchestration of distributed controllers without centralized message brokers.
 * Simulations & Digital Twins: Logical linking of separately attached hardware or virtual devices.
     

## Usage & Integration

Requires a C++20 compatible compiler.

1. Include the header file: #include "tealscript_runtime.hpp" 
2. Instantiate the TealScript runtime object. 
3. Load source code and register C++ extensions (functions, variables, custom types). 
4. Execute in single- or multi-threaded mode. 

To build the examples (on Linux): 
``` bash 
mkdir build && cd build
cmake ..
make
```

Or you can just use shell script

``` bash 
git clone https://github.com/dumitrioo/tealscript.git
cd tealscript
./build_linux_cmake.sh
```

Explore the [examples](examples/), directory for advanced use cases like the ALU 74181 hardware simulation  and host-side C++ extensions. 


## More Information

For the complete language specification, type system, and host API reference, see the [Documentation](doc/tealscript_overview.pdf). (PDF - Migrating to Markdown docs is in progress).
