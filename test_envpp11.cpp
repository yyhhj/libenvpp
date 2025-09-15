#include "envpp11.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Testing envpp11.hpp - C++11 Environment Variable Parser\n";
    std::cout << "======================================================\n\n";
    
    try {
        // Test 1: Basic prefix usage (similar to original libenvpp example)
        std::cout << "Test 1: Basic prefix usage\n";
        auto prefix = env11::prefix("MYPROG");
        
        auto log_path_id = prefix.register_variable<std::string>("LOG_FILE_PATH");
        auto num_threads_id = prefix.register_required_variable<int>("NUM_THREADS");
        
        std::cout << "Help message:\n" << prefix.help_message() << std::endl;
        
        auto parsed = prefix.parse_and_validate();
        
        if (parsed.ok()) {
            auto log_path = parsed.get_or(log_path_id, std::string("/default/log/path"));
            auto num_threads = parsed.get(num_threads_id);
            
            std::cout << "Log path: " << log_path << std::endl;
            std::cout << "Num threads: " << num_threads << std::endl;
        } else {
            std::cout << "Errors:\n" << parsed.error_message();
            std::cout << "Warnings:\n" << parsed.warning_message();
        }
        
        std::cout << "\n";
        
        // Test 2: Single variable access
        std::cout << "Test 2: Single variable access\n";
        
        auto debug_mode = env11::get_or<bool>("DEBUG", false);
        std::cout << "Debug mode: " << (debug_mode ? "enabled" : "disabled") << std::endl;
        
        auto home_dir = env11::get_or<std::string>("HOME", std::string("/tmp"));
        std::cout << "Home directory: " << home_dir << std::endl;
        
        std::cout << "\n";
        
        // Test 3: Range validation
        std::cout << "Test 3: Range validation\n";
        auto range_prefix = env11::prefix("RANGE_TEST");
        auto port_id = range_prefix.register_range<int>("PORT", 1024, 65535);
        
        auto range_parsed = range_prefix.parse_and_validate();
        if (range_parsed.ok()) {
            auto port = range_parsed.get_or(port_id, 8080);
            std::cout << "Port: " << port << std::endl;
        } else {
            std::cout << "Range errors:\n" << range_parsed.error_message();
        }
        
        std::cout << "\n";
        
        // Test 4: Boolean parsing
        std::cout << "Test 4: Boolean parsing\n";
        auto bool_prefix = env11::prefix("BOOL_TEST");
        auto enable_feature_id = bool_prefix.register_variable<bool>("ENABLE_FEATURE");
        
        auto bool_parsed = bool_prefix.parse_and_validate();
        if (bool_parsed.ok()) {
            auto enable_feature = bool_parsed.get_or(enable_feature_id, false);
            std::cout << "Feature enabled: " << (enable_feature ? "yes" : "no") << std::endl;
        } else {
            std::cout << "Boolean parsing errors:\n" << bool_parsed.error_message();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "All tests completed successfully!\n";
    return 0;
}