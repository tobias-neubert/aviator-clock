# Welcome to the Aviator Clock
This project is based on https://www.instructables.com/ESP32-3D-Printed-Aviator-Clock/.

## Usage
### Execute Tests Locally
```
pio test -e unity_tests
pio test -e google_tests
```

### Upload and Execute Programm
```
pio run -e nodemcu-32s -t upload
```

### Install a Library
```
pio lib install "AccelStepper"
```

## Tips and Tricks
### References or Pointers?
#### When to Use a Reference (T&)
Use a reference when the object does not own the dependency. It is simply "using" or "borrowing" it. 

Guaranteed to Exist: A reference cannot be null. By accepting a reference, you are operating on the guarantee that a valid object was given to you. This eliminates the need for if (_timeProvider == nullptr) checks.

No Ownership: Your ClockHand doesn't need to create or destroy the timeProvider or the logger. They are created in main.cpp and will exist for the entire duration of the program. The ClockHand just needs a way to talk to them.

#### When to Use a Pointer (and which kind)
Pointers become necessary when you need to express ownership or when you need more flexibility than a reference can provide.

##### Ownership (std::unique_ptr<T>)
Does a ClockHand own its IStepperMotor? I would argue yes. Each hand has its own dedicated motor. If you destroy a ClockHand component, its associated motor component should probably be destroyed too. This is a clear ownership relationship.

`std::unique_ptr` is the modern C++ way to express exclusive ownership. It's a "smart pointer" that automatically handles the memory for you, so you never have to call delete. It has the same performance as a raw pointer but is much safer.

##### Polymorphism in Containers
This is the bigger reason. Imagine your AviatorClock class. It needs to hold a collection of the three ClockHand objects. All ClockHands are ISocComponents.

You might want to create a `std::vector` to hold them.

You cannot do `std::vector<ISocComponent>`. This is because `ISocComponent` is an abstract interface and cannot be instantiated. Even if it weren't abstract, you would "slice" your obects, losing all the ClockHand data.

You must store a collection of pointers or references to the base class. The standard C++ way to do this safely is `std::vector<std::unique_ptr<ISocComponent>>`. This is where the smart pointers become essential for the composite design pattern to work correctly and safely.

### The Gpolden Role of Arduino Programming
> **Declare pointers globally, instantiate and wire them up in setup().**

This rule is so important because of the static initialization order fiasco.

### The Static Initialization Order Fiasco
The C++ language standard does not guarantee the order in which global objects are initialized across different files. Even within a single file, the dependency chain can be fragile.