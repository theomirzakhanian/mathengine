#pragma once

#include <vector>
#include <functional>

namespace mathengine {

using OdeFunc = std::function<double(double t, double y)>;

struct OdePoint {
    double t, y;
};

// Classical 4th-order Runge-Kutta
std::vector<OdePoint> solve_rk4(OdeFunc f, double t0, double y0, double t_end, double dt);

// Adaptive Runge-Kutta-Fehlberg 4(5)
std::vector<OdePoint> solve_rkf45(OdeFunc f, double t0, double y0, double t_end, double tol = 1e-6);

} // namespace mathengine
