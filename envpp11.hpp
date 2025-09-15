#pragma once

//
// envpp11.hpp - A compact C++11 environment variable parsing library
// Based on libenvpp but designed to be minimal and header-only
//

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace env11 {

// Forward declarations
template<typename T, bool IsRequired>
class variable_id;
class prefix;
class parsed_prefix;

//////////////////////////////////////////////////////////////////////////////
// Exception types
//////////////////////////////////////////////////////////////////////////////

class env_error : public std::runtime_error {
public:
    env_error(const std::string& message) : std::runtime_error(message) {}
};

class parser_error : public env_error {
public:
    parser_error(const std::string& message) : env_error(message) {}
};

class validation_error : public env_error {
public:
    validation_error(const std::string& message) : env_error(message) {}
};

class range_error : public env_error {
public:
    range_error(const std::string& message) : env_error(message) {}
};

class option_error : public env_error {
public:
    option_error(const std::string& message) : env_error(message) {}
};

//////////////////////////////////////////////////////////////////////////////
// Error reporting structure
//////////////////////////////////////////////////////////////////////////////

struct error {
    size_t id;
    std::string name;
    std::string message;
    
    error(size_t id, const std::string& name, const std::string& message)
        : id(id), name(name), message(message) {}
};

//////////////////////////////////////////////////////////////////////////////
// Type traits and parsing utilities
//////////////////////////////////////////////////////////////////////////////

namespace detail {

// Check if type is constructible from string
template<typename T>
struct is_string_constructible {
    template<typename U>
    static auto check(int) -> decltype(U(std::string{}), std::true_type{});
    template<typename>
    static std::false_type check(...);
    
    static constexpr bool value = decltype(check<T>(0))::value;
};

// Check if type supports stream extraction
template<typename T>
struct is_stream_extractable {
    template<typename U>
    static auto check(int) -> decltype(std::declval<std::istringstream&>() >> std::declval<U&>(), std::true_type{});
    template<typename>
    static std::false_type check(...);
    
    static constexpr bool value = decltype(check<T>(0))::value;
};

// Parse boolean values (case-insensitive)
inline bool parse_bool(const std::string& str) {
    std::string lower;
    std::transform(str.begin(), str.end(), std::back_inserter(lower), ::tolower);
    
    if (lower == "true" || lower == "1" || lower == "on" || lower == "yes") {
        return true;
    } else if (lower == "false" || lower == "0" || lower == "off" || lower == "no") {
        return false;
    } else {
        throw parser_error("Failed to parse '" + str + "' as boolean");
    }
}

// Generic parsing function
template<typename T>
typename std::enable_if<!std::is_same<T, std::string>::value && !std::is_same<T, bool>::value, T>::type
parse_value(const std::string& value) {
    std::istringstream stream(value);
    T result;
    stream >> result;
    
    if (stream.fail()) {
        throw parser_error("Failed to parse value: " + value);
    }
    
    // Check for extra characters (but allow whitespace)
    stream >> std::ws;
    if (!stream.eof()) {
        throw parser_error("Unexpected trailing characters in: " + value);
    }
    
    return result;
}

// Specialization for bool
template<typename T>
typename std::enable_if<std::is_same<T, bool>::value, T>::type
parse_value(const std::string& value) {
    return parse_bool(value);
}

// Specialization for string (no parsing needed)
template<typename T>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
parse_value(const std::string& value) {
    return value;
}

// Get environment variable
inline std::string get_env(const std::string& name, bool& found) {
    const char* value = std::getenv(name.c_str());
    found = (value != nullptr);
    return found ? std::string(value) : std::string();
}

// Variable data storage
struct variable_data {
    std::string name;
    bool required;
    std::function<void*()> parser;  // Returns parsed value as void*
    std::function<void(void*)> destructor;  // Cleanup function
    void* value;
    bool has_value;
    
    variable_data(const std::string& n, bool req) 
        : name(n), required(req), value(nullptr), has_value(false) {}
    
    ~variable_data() {
        if (has_value && value && destructor) {
            destructor(value);
        }
    }
    
    // Move constructor
    variable_data(variable_data&& other) noexcept
        : name(std::move(other.name)), required(other.required), 
          parser(std::move(other.parser)), destructor(std::move(other.destructor)),
          value(other.value), has_value(other.has_value) {
        other.value = nullptr;
        other.has_value = false;
    }
    
    // Move assignment
    variable_data& operator=(variable_data&& other) noexcept {
        if (this != &other) {
            if (has_value && value && destructor) {
                destructor(value);
            }
            name = std::move(other.name);
            required = other.required;
            parser = std::move(other.parser);
            destructor = std::move(other.destructor);
            value = other.value;
            has_value = other.has_value;
            other.value = nullptr;
            other.has_value = false;
        }
        return *this;
    }
    
private:
    // Disable copy
    variable_data(const variable_data&) = delete;
    variable_data& operator=(const variable_data&) = delete;
};

} // namespace detail

//////////////////////////////////////////////////////////////////////////////
// Default validators and parsers
//////////////////////////////////////////////////////////////////////////////

template<typename T>
struct default_validator {
    void operator()(const T&) const {
        // Default: no validation
    }
};

template<typename T>
struct default_parser {
    T operator()(const std::string& str) const {
        return detail::parse_value<T>(str);
    }
};

//////////////////////////////////////////////////////////////////////////////
// Variable ID type
//////////////////////////////////////////////////////////////////////////////

template<typename T, bool IsRequired>
class variable_id {
    size_t id_;
    
    friend class prefix;
    friend class parsed_prefix;
    
    explicit variable_id(size_t id) : id_(id) {}
    
public:
    variable_id(const variable_id&) = default;
    variable_id& operator=(const variable_id&) = default;
    
    size_t id() const { return id_; }
    
    // Type information
    using value_type = T;
    static constexpr bool is_required = IsRequired;
};

//////////////////////////////////////////////////////////////////////////////
// Parsed prefix class
//////////////////////////////////////////////////////////////////////////////

class parsed_prefix {
    std::vector<detail::variable_data> variables_;
    std::vector<error> errors_;
    std::vector<error> warnings_;
    std::string prefix_name_;
    bool valid_;
    
    friend class prefix;
    
    parsed_prefix(std::vector<detail::variable_data>&& vars, const std::string& prefix)
        : variables_(std::move(vars)), prefix_name_(prefix), valid_(true) {}
    
public:
    // Move-only type
    parsed_prefix(const parsed_prefix&) = delete;
    parsed_prefix& operator=(const parsed_prefix&) = delete;
    
    parsed_prefix(parsed_prefix&& other) noexcept
        : variables_(std::move(other.variables_)),
          errors_(std::move(other.errors_)),
          warnings_(std::move(other.warnings_)),
          prefix_name_(std::move(other.prefix_name_)),
          valid_(other.valid_) {
        other.valid_ = false;
    }
    
    parsed_prefix& operator=(parsed_prefix&& other) noexcept {
        if (this != &other) {
            variables_ = std::move(other.variables_);
            errors_ = std::move(other.errors_);
            warnings_ = std::move(other.warnings_);
            prefix_name_ = std::move(other.prefix_name_);
            valid_ = other.valid_;
            other.valid_ = false;
        }
        return *this;
    }
    
    // Check if parsing was successful
    bool ok() const {
        check_valid();
        return errors_.empty();
    }
    
    // Get required variable value
    template<typename T>
    T get(const variable_id<T, true>& var_id) const {
        check_valid();
        
        if (var_id.id() >= variables_.size()) {
            throw env_error("Invalid variable ID");
        }
        
        const auto& var = variables_[var_id.id()];
        if (!var.has_value) {
            throw env_error("Required variable '" + var.name + "' has no value");
        }
        
        return *static_cast<T*>(var.value);
    }
    
    // Get optional variable value  
    template<typename T>
    T* get(const variable_id<T, false>& var_id) const {
        check_valid();
        
        if (var_id.id() >= variables_.size()) {
            throw env_error("Invalid variable ID");
        }
        
        const auto& var = variables_[var_id.id()];
        return var.has_value ? static_cast<T*>(var.value) : nullptr;
    }
    
    // Get optional variable with default
    template<typename T, typename U>
    T get_or(const variable_id<T, false>& var_id, const U& default_value) const {
        T* value = get(var_id);
        return value ? *value : static_cast<T>(default_value);
    }
    
    // Error/warning access
    const std::vector<error>& errors() const {
        check_valid();
        return errors_;
    }
    
    const std::vector<error>& warnings() const {
        check_valid();
        return warnings_;
    }
    
    std::string error_message() const {
        check_valid();
        return format_messages("Error", errors_);
    }
    
    std::string warning_message() const {
        check_valid();
        return format_messages("Warning", warnings_);
    }
    
    std::string help_message() const {
        check_valid();
        
        std::ostringstream oss;
        oss << "Prefix '" << prefix_name_ << "' supports the following " 
            << variables_.size() << " environment variable(s):\n";
        
        for (size_t i = 0; i < variables_.size(); ++i) {
            const auto& var = variables_[i];
            oss << "    '" << prefix_name_ << var.name << "' " 
                << (var.required ? "required" : "optional") << "\n";
        }
        
        return oss.str();
    }
    
private:
    void check_valid() const {
        if (!valid_) {
            throw env_error("Parsed prefix has been invalidated by move");
        }
    }
    
    std::string format_messages(const std::string& type, const std::vector<error>& messages) const {
        if (messages.empty()) {
            return std::string();
        }
        
        std::ostringstream oss;
        for (const auto& err : messages) {
            oss << type << ": " << err.message << "\n";
        }
        
        return oss.str();
    }
};

//////////////////////////////////////////////////////////////////////////////
// Prefix class
//////////////////////////////////////////////////////////////////////////////

class prefix {
    std::string prefix_name_;
    std::vector<detail::variable_data> variables_;
    bool valid_;
    
public:
    explicit prefix(const std::string& name) 
        : prefix_name_(name + "_"), valid_(true) {
        if (name.empty()) {
            throw env_error("Prefix name cannot be empty");
        }
    }
    
    // Move-only type
    prefix(const prefix&) = delete;
    prefix& operator=(const prefix&) = delete;
    
    prefix(prefix&& other) noexcept
        : prefix_name_(std::move(other.prefix_name_)),
          variables_(std::move(other.variables_)),
          valid_(other.valid_) {
        other.valid_ = false;
    }
    
    prefix& operator=(prefix&& other) noexcept {
        if (this != &other) {
            prefix_name_ = std::move(other.prefix_name_);
            variables_ = std::move(other.variables_);
            valid_ = other.valid_;
            other.valid_ = false;
        }
        return *this;
    }
    
    // Register optional variable
    template<typename T>
    variable_id<T, false> register_variable(const std::string& name) {
        check_valid();
        
        size_t id = variables_.size();
        variables_.emplace_back(name, false);
        auto& var = variables_.back();
        
        // Set up parser and destructor
        var.parser = [name, this]() -> void* {
            std::string full_name = prefix_name_ + name;
            bool found;
            std::string value = detail::get_env(full_name, found);
            
            if (!found) {
                return nullptr;
            }
            
            try {
                T* parsed = new T(default_parser<T>{}(value));
                default_validator<T>{}(*parsed);
                return parsed;
            } catch (const std::exception& e) {
                throw parser_error("Error parsing " + full_name + ": " + e.what());
            }
        };
        
        var.destructor = [](void* ptr) {
            delete static_cast<T*>(ptr);
        };
        
        return variable_id<T, false>(id);
    }
    
    // Register required variable
    template<typename T>
    variable_id<T, true> register_required_variable(const std::string& name) {
        check_valid();
        
        size_t id = variables_.size();
        variables_.emplace_back(name, true);
        auto& var = variables_.back();
        
        // Set up parser and destructor
        var.parser = [name, this]() -> void* {
            std::string full_name = prefix_name_ + name;
            bool found;
            std::string value = detail::get_env(full_name, found);
            
            if (!found) {
                throw env_error("Required environment variable '" + full_name + "' not set");
            }
            
            try {
                T* parsed = new T(default_parser<T>{}(value));
                default_validator<T>{}(*parsed);
                return parsed;
            } catch (const std::exception& e) {
                throw parser_error("Error parsing " + full_name + ": " + e.what());
            }
        };
        
        var.destructor = [](void* ptr) {
            delete static_cast<T*>(ptr);
        };
        
        return variable_id<T, true>(id);
    }
    
    // Register range variable (optional)
    template<typename T>
    variable_id<T, false> register_range(const std::string& name, const T& min_val, const T& max_val) {
        check_valid();
        
        if (min_val > max_val) {
            throw env_error("Invalid range: min > max");
        }
        
        size_t id = variables_.size();
        variables_.emplace_back(name, false);
        auto& var = variables_.back();
        
        var.parser = [name, this, min_val, max_val]() -> void* {
            std::string full_name = prefix_name_ + name;
            bool found;
            std::string value = detail::get_env(full_name, found);
            
            if (!found) {
                return nullptr;
            }
            
            try {
                T parsed_value = default_parser<T>{}(value);
                default_validator<T>{}(parsed_value);
                
                if (parsed_value < min_val || parsed_value > max_val) {
                    std::ostringstream oss;
                    oss << "Value " << parsed_value << " outside of range [" << min_val << ", " << max_val << "]";
                    throw range_error(oss.str());
                }
                
                return new T(parsed_value);
            } catch (const std::exception& e) {
                throw parser_error("Error parsing " + full_name + ": " + e.what());
            }
        };
        
        var.destructor = [](void* ptr) {
            delete static_cast<T*>(ptr);
        };
        
        return variable_id<T, false>(id);
    }
    
    // Register required range variable
    template<typename T>
    variable_id<T, true> register_required_range(const std::string& name, const T& min_val, const T& max_val) {
        check_valid();
        
        if (min_val > max_val) {
            throw env_error("Invalid range: min > max");
        }
        
        size_t id = variables_.size();
        variables_.emplace_back(name, true);
        auto& var = variables_.back();
        
        var.parser = [name, this, min_val, max_val]() -> void* {
            std::string full_name = prefix_name_ + name;
            bool found;
            std::string value = detail::get_env(full_name, found);
            
            if (!found) {
                throw env_error("Required environment variable '" + full_name + "' not set");
            }
            
            try {
                T parsed_value = default_parser<T>{}(value);
                default_validator<T>{}(parsed_value);
                
                if (parsed_value < min_val || parsed_value > max_val) {
                    std::ostringstream oss;
                    oss << "Value " << parsed_value << " outside of range [" << min_val << ", " << max_val << "]";
                    throw range_error(oss.str());
                }
                
                return new T(parsed_value);
            } catch (const std::exception& e) {
                throw parser_error("Error parsing " + full_name + ": " + e.what());
            }
        };
        
        var.destructor = [](void* ptr) {
            delete static_cast<T*>(ptr);
        };
        
        return variable_id<T, true>(id);
    }
    
    // Parse and validate all registered variables
    parsed_prefix parse_and_validate() {
        check_valid();
        
        auto result = parsed_prefix(std::move(variables_), prefix_name_);
        valid_ = false;  // Invalidate this prefix
        
        // Parse all variables
        for (auto& var : result.variables_) {
            try {
                var.value = var.parser();
                var.has_value = (var.value != nullptr);
            } catch (const std::exception& e) {
                if (var.required) {
                    result.errors_.emplace_back(result.variables_.size(), var.name, e.what());
                } else {
                    result.warnings_.emplace_back(result.variables_.size(), var.name, e.what());
                }
            }
        }
        
        return result;
    }
    
    std::string help_message() const {
        check_valid();
        
        std::ostringstream oss;
        oss << "Prefix '" << prefix_name_ << "' supports the following " 
            << variables_.size() << " environment variable(s):\n";
        
        for (size_t i = 0; i < variables_.size(); ++i) {
            const auto& var = variables_[i];
            oss << "    '" << prefix_name_ << var.name << "' " 
                << (var.required ? "required" : "optional") << "\n";
        }
        
        return oss.str();
    }
    
private:
    void check_valid() const {
        if (!valid_) {
            throw env_error("Prefix has been invalidated by parsing or move");
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
// Convenience functions for single variables
//////////////////////////////////////////////////////////////////////////////

template<typename T>
T get(const std::string& name) {
    bool found;
    std::string value = detail::get_env(name, found);
    
    if (!found) {
        throw env_error("Environment variable '" + name + "' not found");
    }
    
    try {
        T result = default_parser<T>{}(value);
        default_validator<T>{}(result);
        return result;
    } catch (const std::exception& e) {
        throw parser_error("Error parsing " + name + ": " + e.what());
    }
}

template<typename T, typename U>
T get_or(const std::string& name, const U& default_value) {
    bool found;
    std::string value = detail::get_env(name, found);
    
    if (!found) {
        return static_cast<T>(default_value);
    }
    
    try {
        T result = default_parser<T>{}(value);
        default_validator<T>{}(result);
        return result;
    } catch (const std::exception&) {
        return static_cast<T>(default_value);
    }
}

} // namespace env11