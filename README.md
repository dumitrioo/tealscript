# TealScript

TealScript is a lightweight, distributed, and deterministic control logic engine—free from heavy message brokers (ROS/MQTT) and garbage collection overhead, with native C++ integration. 

It is an embedded scripting language designed to seamlessly wire physical and virtual devices into a unified control system. The language enforces strict, declarative rules for building schemas, making complex logic highly readable and maintainable. Combined with seamless C++ host extensibility, the result is a deterministic, cohesive hardware-software ecosystem driven entirely by data-flow graphs.


## The "Secret Sauce": Stateful Data-Flow

While based on the discrete-time, clocked data-flow paradigm (similar to Unreal Engine Blueprints), TealScript departs from strict functional purity. Each compute node is an instance of an object that syntactically resembles a function, but can retain state between execution cycles via instance variables accessible through the this keyword. This makes writing complex state machines or PID controllers as easy as writing simple C functions.


## Why TealScript?

When designing control logic, it is tempting to specify what the outcome should be, rather than script how to achieve it step-by-step. While C++ is imperative and requires detailed architectural design, TealScript allows you to broaden your programming approaches without changing your C++ toolchain. 

You get a problem-specific tool to handle complex control schemes for multiple actuators based on numerous sensor signals, drastically reducing the low-level C++ boilerplate required for wiring, state management, and multi-threading, while keeping full native extensibility.

## Key Features

 * Network-Agnostic Distributed Graphs: Seamlessly link variables across different hosts using extern URIs. Built on a custom UDP multiplexing protocol (MTU-safe 1400 bytes) that eliminates Head-of-Line blocking without the overhead of TCP or heavy brokers like MQTT.
 * True Multi-Threading: Execute graph schemas in parallel across available CPU cores. The interpreter safely handles node execution without requiring the user to manage C++ threads or locks.
 * Zero Dependencies & Portable: Implemented as a custom execution tree interpreter (no LLVM/external lexers). It compiles into any C++20 codebase via CMake and is completely hardware-agnostic.
 * Turing Complete & Extensible: Handle general-purpose tasks, math (vec4, mat4), JSON, and custom C++ types. Easily inject host functions into the scripting runtime.


## Quick Example

A logical schema written in TealScript manages physical actuators in parallel by analyzing signals from input devices:

``` TealScript
// input values from C++ host
'in' in;
'kp' kp;
'ki' ki;
'kd' kd;
'km' km;

// worker computation nodes declarative schema
difference      diff1(in, pid);
pid_regulator   pid(diff1, kp, ki, kd) 'pid_out';
difference      diff2(in, mr);
math_regulator  mr(diff2, km) 'pow_out';
logprint        pv(in, diff1, pid, diff2, mr);

// computation nodes functionality...

// PID regulator
pid_regulator(err, kp, ki, kd) {
    if(this.prev_t == undefined) {
        this.prev_t = steady_clock();
        this.prev_err = (float)err;
        this.integral = 0.0;
    }
    t = steady_clock();
    dt = t - this.prev_t;
    if(dt > 0.0) {
        this.prev_t = t;
        proportional = err;
        this.integral += err * dt;
        derivative = (err - this.prev_err) / dt;
        this.prev_err = (float)err;
        output = kp * proportional + ki * this.integral + kd * derivative;
        return output;
    }
}

// Alternative regulator...
math_regulator(err, km) {
    if(this.output == undefined) {
        this.output = 0.0;
        this.prev_t = steady_clock();
    }
    t = steady_clock();
    dt = t - this.prev_t;
    if(dt > 0.0) {
        this.prev_t = t;
        delta = km * dt * (pow(err * 0.02, 3) - pow(err * 0.0199999, 3) + 100000.0 * sin(err * .000001));
        if(abs(delta) > abs(err)) {
            delta = 0.333 * err;
        }
        this.output += delta;
        return this.output;
    }
}

first_defined(a, b) return a != undefined ? a : b;
forward(val) return val;
difference(a, b) return a - b;

val_with_rand(v, dev, expire_time) {
    if(this.prev_t == undefined) {
        this.prev_t = steady_clock();
        this.delta = (randf() - 0.5) * 2.0 * dev;
    }
    t = steady_clock();
    dt = t - this.prev_t;
    if(dt > expire_time) {
        this.delta = (randf() - 0.5) * 2.0 * dev;
        this.prev_t = t;
    }
    return v + this.delta;
}

logprint(in, e1, pidval, e2, mathval) {
    t = steady_clock();
    dt = t - this.prev_t;
    if(dt > 0.1) {
        console.fixed();
        this.prev_t = t;
        console.print("[ target: ", in,
            " || err1: ", e1, ", PID val: ", pidval,
            " | err2: ", e2, ", Math val: ", mathval,
            " ]"
        );
    }
}
```


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

To build the examples (requires Raylib and ZeroMQ on Linux): 
``` bash 
mkdir build && cd build
cmake ..
make
```

Explore the [examples](examples/), directory for advanced use cases like the ALU 74181 hardware simulation  and host-side C++ extensions. 


## More Information

For the complete language specification, type system, and host API reference, see the [Documentation](doc/tealscript_overview.pdf). (PDF - Migrating to Markdown docs is in progress).
