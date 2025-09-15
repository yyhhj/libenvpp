# envpp11.hpp - Compact C++11 Environment Variable Parser

A minimal, header-only C++11 library for type-safe parsing of environment variables, inspired by [libenvpp](https://github.com/ph3at/libenvpp).

## Features

- **C++11 compatible** - No external dependencies beyond standard library
- **Header-only** - Just include `envpp11.hpp` and you're ready to go
- **Type-safe parsing** - Automatic parsing for built-in types (int, float, bool, string, etc.)
- **Prefix support** - Organize environment variables with prefixes
- **Required/Optional variables** - Distinguish between mandatory and optional configuration
- **Range validation** - Built-in range checking for numeric types
- **Error handling** - Comprehensive error reporting and validation
- **Compact** - Single header file under 700 lines

## Quick Start

```cpp
#include "envpp11.hpp"
#include <iostream>

int main() {
    try {
        // Create a prefix for your application
        auto config = env11::prefix("MYAPP");
        
        // Register variables
        auto log_path_id = config.register_variable<std::string>("LOG_PATH");
        auto num_threads_id = config.register_required_variable<int>("NUM_THREADS");
        auto port_id = config.register_range<int>("PORT", 1024, 65535);
        auto debug_id = config.register_variable<bool>("DEBUG");
        
        // Parse and validate all variables
        auto parsed = config.parse_and_validate();
        
        if (parsed.ok()) {
            // Get values with defaults for optional variables
            auto log_path = parsed.get_or(log_path_id, std::string("/tmp/app.log"));
            auto num_threads = parsed.get(num_threads_id);  // Required, no default
            auto port = parsed.get_or(port_id, 8080);
            auto debug = parsed.get_or(debug_id, false);
            
            std::cout << "Log path: " << log_path << std::endl;
            std::cout << "Threads: " << num_threads << std::endl;
            std::cout << "Port: " << port << std::endl;
            std::cout << "Debug: " << (debug ? "on" : "off") << std::endl;
        } else {
            std::cout << parsed.error_message();
            std::cout << parsed.help_message();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

## Usage

### Environment Variables

Set environment variables with your prefix:
```bash
export MYAPP_NUM_THREADS=4
export MYAPP_LOG_PATH=/var/log/app.log
export MYAPP_PORT=9090
export MYAPP_DEBUG=true
```

### Single Variable Access

For simple cases without prefixes:
```cpp
// Get with default value
auto debug = env11::get_or<bool>("DEBUG", false);
auto port = env11::get_or<int>("PORT", 8080);

// Get required variable (throws if not found)
try {
    auto threads = env11::get<int>("NUM_THREADS");
} catch (const env11::env_error& e) {
    std::cerr << e.what() << std::endl;
}
```

### Supported Types

- **Numeric**: `int`, `long`, `float`, `double`, etc.
- **Boolean**: `true`/`false`, `1`/`0`, `on`/`off`, `yes`/`no` (case-insensitive)
- **String**: `std::string`
- **Custom types**: Any type with stream extraction operator (`>>`)

### Range Validation

```cpp
auto config = env11::prefix("APP");

// Port must be between 1024 and 65535
auto port_id = config.register_range<int>("PORT", 1024, 65535);

// Also works with required variables
auto threads_id = config.register_required_range<int>("THREADS", 1, 16);
```

### Error Handling

```cpp
auto parsed = config.parse_and_validate();

if (!parsed.ok()) {
    // Print all errors
    std::cout << parsed.error_message();
    
    // Print warnings (if any)
    std::cout << parsed.warning_message();
    
    // Show help for all registered variables
    std::cout << parsed.help_message();
    
    // Or access individual errors
    for (const auto& error : parsed.errors()) {
        std::cout << "Variable: " << error.name 
                  << ", Error: " << error.message << std::endl;
    }
}
```

## Comparison with libenvpp

| Feature | libenvpp | envpp11.hpp |
|---------|----------|-------------|
| C++ Version | C++17 | C++11 |
| Dependencies | fmt library | None (standard library only) |
| File Count | Multiple headers + sources | Single header |
| Size | ~2000+ lines | ~700 lines |
| Type Safety | ✓ | ✓ |
| Prefix Support | ✓ | ✓ |
| Range Validation | ✓ | ✓ |
| Custom Parsers | ✓ | ✗ (planned) |
| Option Variables | ✓ | ✗ (planned) |
| Testing Support | ✓ | ✗ (could be added) |
| Typo Detection | ✓ | ✗ (could be added) |

## Building

Since it's header-only, just include the file:

```cpp
#include "envpp11.hpp"
```

Compile with C++11 support:
```bash
g++ -std=c++11 your_program.cpp
```

## License

This implementation is provided as-is for educational and practical use. Based on the concepts from libenvpp by ph3at.