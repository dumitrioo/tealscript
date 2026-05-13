# TealScript

TealScript is an embedded scripting engine. Lightweight, easy to integrate, intended to create control logic. Distributed, but free from heavy message brokers (ROS/MQTT). Free from garbage collection overhead, with native C++ integration, designed to seamlessly wire physical and virtual devices into a unified control system. The language in data-flow graph paradigm enforces clear declarative rules for building schemas, making complex logic highly readable and maintainable. Combined with seamless C++ host extensibility, the result is a deterministic, cohesive hardware-software ecosystem driven entirely by data-flow graphs.


## Stateful Data-Flow

While based on the discrete-time, clocked data-flow paradigm (similar to Unreal Engine Blueprints), TealScript departs from strict functional purity. Each compute node is an instance of an object that syntactically resembles a function, but can retain state between execution cycles via instance variables accessible through the this keyword. This makes writing complex state machines or PID controllers as easy as writing simple functions.


## Why TealScript?

When designing control logic, it is tempting to specify what the outcome should be, rather than script how to achieve it step-by-step. While C++ is imperative and requires detailed architectural design, TealScript allows you to broaden your programming approaches without changing your C++ toolchain. 

You get a problem-specific tool to handle complex control schemes for multiple actuators based on numerous sensor signals, drastically reducing the low-level C++ boilerplate required for wiring, state management, and multi-threading, while keeping full native extensibility.


## Key Features
 * C-like sytax, most of C math functions provided.
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
