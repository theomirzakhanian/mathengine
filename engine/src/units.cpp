#include "mathengine/units.h"
#include <cmath>

namespace mathengine {

const std::vector<PhysConstant>& get_constants() {
    static const std::vector<PhysConstant> constants = {
        {"Speed of light", "c", 299792458.0, "m/s"},
        {"Planck constant", "h", 6.62607015e-34, "J*s"},
        {"Reduced Planck", "hbar", 1.054571817e-34, "J*s"},
        {"Boltzmann constant", "k_B", 1.380649e-23, "J/K"},
        {"Gravitational constant", "G", 6.67430e-11, "m^3/(kg*s^2)"},
        {"Elementary charge", "e_charge", 1.602176634e-19, "C"},
        {"Electron mass", "m_e", 9.1093837015e-31, "kg"},
        {"Proton mass", "m_p", 1.67262192369e-27, "kg"},
        {"Avogadro number", "N_A", 6.02214076e23, "1/mol"},
        {"Gas constant", "R", 8.314462618, "J/(mol*K)"},
        {"Vacuum permittivity", "eps_0", 8.8541878128e-12, "F/m"},
        {"Vacuum permeability", "mu_0", 1.25663706212e-6, "N/A^2"},
        {"Stefan-Boltzmann", "sigma", 5.670374419e-8, "W/(m^2*K^4)"},
        {"Standard gravity", "g", 9.80665, "m/s^2"},
        {"Atomic mass unit", "u", 1.66053906660e-27, "kg"},
    };
    return constants;
}

// Unit conversion tables (base unit for each category, factor to convert to base)
struct UnitDef { std::string name; std::string category; double to_base; };

static const std::vector<UnitDef>& get_unit_defs() {
    static const std::vector<UnitDef> defs = {
        // Length (base: meters)
        {"m", "length", 1.0}, {"km", "length", 1000.0}, {"cm", "length", 0.01},
        {"mm", "length", 0.001}, {"um", "length", 1e-6}, {"nm", "length", 1e-9},
        {"in", "length", 0.0254}, {"ft", "length", 0.3048}, {"yd", "length", 0.9144},
        {"mi", "length", 1609.344}, {"nmi", "length", 1852.0},
        // Mass (base: kg)
        {"kg", "mass", 1.0}, {"g", "mass", 0.001}, {"mg", "mass", 1e-6},
        {"lb", "mass", 0.453592}, {"oz", "mass", 0.0283495}, {"ton", "mass", 1000.0},
        // Time (base: seconds)
        {"s", "time", 1.0}, {"ms", "time", 0.001}, {"us", "time", 1e-6},
        {"min", "time", 60.0}, {"hr", "time", 3600.0}, {"day", "time", 86400.0},
        // Temperature (special handling needed, but store offset for linear approx)
        {"K", "temperature", 1.0}, {"C", "temperature", 1.0}, {"F", "temperature", 0.555556},
        // Speed (base: m/s)
        {"m/s", "speed", 1.0}, {"km/h", "speed", 0.277778}, {"mph", "speed", 0.44704},
        {"knot", "speed", 0.514444},
        // Force (base: N)
        {"N", "force", 1.0}, {"kN", "force", 1000.0}, {"lbf", "force", 4.44822},
        // Energy (base: J)
        {"J", "energy", 1.0}, {"kJ", "energy", 1000.0}, {"cal", "energy", 4.184},
        {"kcal", "energy", 4184.0}, {"eV", "energy", 1.602176634e-19},
        {"kWh", "energy", 3.6e6},
        // Pressure (base: Pa)
        {"Pa", "pressure", 1.0}, {"kPa", "pressure", 1000.0}, {"atm", "pressure", 101325.0},
        {"bar", "pressure", 100000.0}, {"psi", "pressure", 6894.76},
        // Angle (base: radians)
        {"rad", "angle", 1.0}, {"deg", "angle", M_PI/180.0},
    };
    return defs;
}

double convert_units(const std::string& from, const std::string& to) {
    const auto& defs = get_unit_defs();
    double from_factor = 0, to_factor = 0;
    std::string from_cat, to_cat;
    for (auto& d : defs) {
        if (d.name == from) { from_factor = d.to_base; from_cat = d.category; }
        if (d.name == to) { to_factor = d.to_base; to_cat = d.category; }
    }
    if (from_factor == 0 || to_factor == 0 || from_cat != to_cat) return 0;
    return from_factor / to_factor;
}

std::vector<std::string> get_unit_categories() {
    return {"length", "mass", "time", "speed", "force", "energy", "pressure", "angle"};
}

std::vector<std::pair<std::string, double>> get_units_in_category(const std::string& cat) {
    std::vector<std::pair<std::string, double>> result;
    for (auto& d : get_unit_defs())
        if (d.category == cat) result.push_back({d.name, d.to_base});
    return result;
}

} // namespace mathengine
