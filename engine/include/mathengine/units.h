#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace mathengine {

struct PhysConstant {
    std::string name;
    std::string symbol;
    double value;
    std::string unit;
};

const std::vector<PhysConstant>& get_constants();

// Unit conversion: returns multiplier to convert from_unit to to_unit
// Returns 0 if conversion not possible
double convert_units(const std::string& from_unit, const std::string& to_unit);

// Get all available unit categories
std::vector<std::string> get_unit_categories();

// Get units in a category
std::vector<std::pair<std::string, double>> get_units_in_category(const std::string& category);

} // namespace mathengine
