#include "envpp11.hpp"
#include <iostream>
#include <string>

// Example usage similar to the original libenvpp
int main() {
    std::cout << "envpp11 - Simple Usage Example\n";
    std::cout << "===============================\n\n";
    
    try {
        // Create a prefix for your application
        auto pre = env11::prefix("MYAPP");
        
        // Register variables
        const auto log_path_id = pre.register_variable<std::string>("LOG_FILE_PATH");
        const auto num_threads_id = pre.register_required_variable<int>("NUM_THREADS");
        const auto port_id = pre.register_range<int>("PORT", 1024, 65535);
        const auto debug_id = pre.register_variable<bool>("DEBUG");
        
        // Parse and validate
        auto parsed = pre.parse_and_validate();
        
        if (parsed.ok()) {
            // Get values with defaults for optional variables
            auto log_path = parsed.get_or(log_path_id, std::string("/default/log/path"));
            auto num_threads = parsed.get(num_threads_id);  // Required, no default needed
            auto port = parsed.get_or(port_id, 8080);
            auto debug = parsed.get_or(debug_id, false);
            
            std::cout << "Configuration:\n";
            std::cout << "  Log path   : " << log_path << "\n";
            std::cout << "  Num threads: " << num_threads << "\n";
            std::cout << "  Port       : " << port << "\n";
            std::cout << "  Debug      : " << (debug ? "enabled" : "disabled") << "\n";
            
        } else {
            std::cout << "Configuration errors found:\n";
            std::cout << parsed.error_message();
            
            if (!parsed.warnings().empty()) {
                std::cout << "\nWarnings:\n";
                std::cout << parsed.warning_message();
            }
            
            std::cout << "\n" << parsed.help_message();
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}