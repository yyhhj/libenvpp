#include "envpp11.hpp"
#include <iostream>
#include <cassert>

void test_basic_parsing() {
    std::cout << "Testing basic parsing...\n";
    
    // Test integer parsing
    auto config = env11::prefix("TESTBASIC");
    auto int_id = config.register_variable<int>("INT");
    auto float_id = config.register_variable<float>("FLOAT");
    auto bool_id = config.register_variable<bool>("BOOL");
    auto string_id = config.register_variable<std::string>("STRING");
    
    auto parsed = config.parse_and_validate();
    
    // All should be optional and have defaults
    assert(parsed.ok());
    assert(parsed.get_or(int_id, 42) == 42);
    assert(parsed.get_or(float_id, 3.14f) == 3.14f);
    assert(parsed.get_or(bool_id, true) == true);
    assert(parsed.get_or(string_id, std::string("default")) == "default");
    
    std::cout << "✓ Basic parsing tests passed\n";
}

void test_range_validation() {
    std::cout << "Testing range validation...\n";
    
    auto config = env11::prefix("TESTRANGE");
    auto port_id = config.register_range<int>("PORT", 1000, 2000);
    
    auto parsed = config.parse_and_validate();
    assert(parsed.ok());
    
    // Default should work even outside range (as documented in libenvpp)
    assert(parsed.get_or(port_id, 8080) == 8080);
    
    std::cout << "✓ Range validation tests passed\n";
}

void test_boolean_parsing() {
    std::cout << "Testing boolean parsing variants...\n";
    
    // Test different boolean representations
    std::vector<std::pair<std::string, bool>> test_cases = {
        {"true", true}, {"TRUE", true}, {"True", true},
        {"false", false}, {"FALSE", false}, {"False", false},
        {"1", true}, {"0", false},
        {"on", true}, {"ON", true}, {"off", false}, {"OFF", false},
        {"yes", true}, {"YES", true}, {"no", false}, {"NO", false}
    };
    
    // We can't easily test this without setting environment variables,
    // but the parse_bool function should handle all these cases
    for (const auto& test_case : test_cases) {
        try {
            bool result = env11::detail::parse_bool(test_case.first);
            assert(result == test_case.second);
        } catch (const std::exception& e) {
            std::cout << "Failed to parse '" << test_case.first << "': " << e.what() << std::endl;
            assert(false);
        }
    }
    
    std::cout << "✓ Boolean parsing tests passed\n";
}

void test_error_cases() {
    std::cout << "Testing error cases...\n";
    
    auto config = env11::prefix("TESTERROR");
    auto required_id = config.register_required_variable<int>("REQUIRED");
    
    auto parsed = config.parse_and_validate();
    
    // Should have error for missing required variable
    assert(!parsed.ok());
    assert(!parsed.errors().empty());
    
    std::string error_msg = parsed.error_message();
    assert(error_msg.find("TESTERROR_REQUIRED") != std::string::npos);
    
    std::cout << "✓ Error case tests passed\n";
}

void test_help_message() {
    std::cout << "Testing help message...\n";
    
    auto config = env11::prefix("TESTHELP");
    config.register_variable<std::string>("OPTIONAL");
    config.register_required_variable<int>("REQUIRED");
    
    std::string help = config.help_message();
    assert(help.find("TESTHELP_") != std::string::npos);
    assert(help.find("optional") != std::string::npos);
    assert(help.find("required") != std::string::npos);
    
    std::cout << "✓ Help message tests passed\n";
}

void test_single_variable_functions() {
    std::cout << "Testing single variable functions...\n";
    
    // Test get_or with non-existent variable
    auto value = env11::get_or<int>("NONEXISTENT_VAR", 123);
    assert(value == 123);
    
    // Test get_or with string
    auto str_value = env11::get_or<std::string>("NONEXISTENT_STR", std::string("default"));
    assert(str_value == "default");
    
    std::cout << "✓ Single variable function tests passed\n";
}

int main() {
    std::cout << "Running comprehensive tests for envpp11...\n";
    std::cout << "==========================================\n\n";
    
    try {
        test_basic_parsing();
        test_range_validation();
        test_boolean_parsing();
        test_error_cases();
        test_help_message();
        test_single_variable_functions();
        
        std::cout << "\n🎉 All tests passed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception\n";
        return 1;
    }
    
    return 0;
}