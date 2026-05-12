#include "mathengine/ode.h"
#include <cmath>
#include <algorithm>

namespace mathengine {

std::vector<OdePoint> solve_rk4(OdeFunc f, double t0, double y0, double t_end, double dt) {
    std::vector<OdePoint> result;
    double t = t0, y = y0;
    result.push_back({t, y});

    while (t < t_end) {
        if (t + dt > t_end) dt = t_end - t;
        double k1 = dt * f(t, y);
        double k2 = dt * f(t + dt/2, y + k1/2);
        double k3 = dt * f(t + dt/2, y + k2/2);
        double k4 = dt * f(t + dt, y + k3);
        y += (k1 + 2*k2 + 2*k3 + k4) / 6.0;
        t += dt;
        result.push_back({t, y});
    }
    return result;
}

std::vector<OdePoint> solve_rkf45(OdeFunc f, double t0, double y0, double t_end, double tol) {
    std::vector<OdePoint> result;
    double t = t0, y = y0;
    double dt = (t_end - t0) / 100.0;
    result.push_back({t, y});

    while (t < t_end - 1e-15) {
        if (t + dt > t_end) dt = t_end - t;

        double k1 = dt * f(t, y);
        double k2 = dt * f(t + dt/4.0, y + k1/4.0);
        double k3 = dt * f(t + 3.0*dt/8.0, y + 3.0*k1/32.0 + 9.0*k2/32.0);
        double k4 = dt * f(t + 12.0*dt/13.0, y + 1932.0*k1/2197.0 - 7200.0*k2/2197.0 + 7296.0*k3/2197.0);
        double k5 = dt * f(t + dt, y + 439.0*k1/216.0 - 8.0*k2 + 3680.0*k3/513.0 - 845.0*k4/4104.0);
        double k6 = dt * f(t + dt/2.0, y - 8.0*k1/27.0 + 2.0*k2 - 3544.0*k3/2565.0 + 1859.0*k4/4104.0 - 11.0*k5/40.0);

        double y4 = y + 25.0*k1/216.0 + 1408.0*k3/2565.0 + 2197.0*k4/4104.0 - k5/5.0;
        double y5 = y + 16.0*k1/135.0 + 6656.0*k3/12825.0 + 28561.0*k4/56430.0 - 9.0*k5/50.0 + 2.0*k6/55.0;

        double err = std::abs(y5 - y4);
        if (err < 1e-15) err = 1e-15;

        if (err <= tol) {
            t += dt;
            y = y5;
            result.push_back({t, y});
        }

        double scale = 0.84 * std::pow(tol / err, 0.25);
        scale = std::max(0.1, std::min(scale, 4.0));
        dt *= scale;
    }
    return result;
}

} // namespace mathengine
