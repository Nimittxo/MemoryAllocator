# Memory Allocator
![Write-back with write-allocation](https://upload.wikimedia.org/wikipedia/commons/thumb/4/4e/Write-back_with_write-allocation.svg/1200px-Write-back_with_write-allocation.svg.png)

This project is a memory allocator implemented in C. It provides a set of functions that allow dynamic allocation and deallocation of memory blocks.

## Features

- Efficient memory allocation and deallocation
- Support for different allocation strategies (e.g., first-fit, best-fit, worst-fit)
- Memory block coalescing to reduce fragmentation
- Thread-safe implementation using locks or atomic operations

## Usage

To use the memory allocator in your C project, follow these steps:

1. Include the `memory_allocator.h` header file in your source code.
2. Initialize the memory allocator by calling the `init_allocator()` function.
3. Allocate memory using the `allocate_memory()` function.
4. Free allocated memory using the `free_memory()` function.
5. Clean up the memory allocator by calling the `destroy_allocator()` function.

```c
#include "memory_allocator.h"

int main() {
    // Initialize the memory allocator
    init_allocator();

    // Allocate memory
    void* ptr = allocate_memory(100);

    // Use the allocated memory

    // Free the allocated memory
    free_memory(ptr);

    // Clean up the memory allocator
    destroy_allocator();

    return 0;
}
```

## Customization

The memory allocator can be customized by modifying the configuration parameters in the `memory_allocator.h` header file. These parameters include the allocation strategy, the size of the memory pool, and the locking mechanism used for thread safety.

## License

This project is licensed under the [MIT License](LICENSE).
This project is a memory allocator implemented in C. It provides a set of functions that allow dynamic allocation and deallocation of memory blocks.

## Features

- Efficient memory allocation and deallocation
- Support for different allocation strategies (e.g., first-fit, best-fit, worst-fit)
- Memory block coalescing to reduce fragmentation
- Thread-safe implementation using locks or atomic operations

## Usage

To use the memory allocator in your C project, follow these steps:

1. Include the `memory_allocator.h` header file in your source code.
2. Initialize the memory allocator by calling the `init_allocator()` function.
3. Allocate memory using the `allocate_memory()` function.
4. Free allocated memory using the `free_memory()` function.
5. Clean up the memory allocator by calling the `destroy_allocator()` function.

```c
#include "memory_allocator.h"

int main() {
    // Initialize the memory allocator
    init_allocator();

    // Allocate memory
    void* ptr = allocate_memory(100);

    // Use the allocated memory

    // Free the allocated memory
    free_memory(ptr);

    // Clean up the memory allocator
    destroy_allocator();

    return 0;
}
```

## Customization

The memory allocator can be customized by modifying the configuration parameters in the `memory_allocator.h` header file. These parameters include the allocation strategy, the size of the memory pool, and the locking mechanism used for thread safety.

## License

This project is licensed under the [MIT License](LICENSE).
