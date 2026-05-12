#include "tools.h"
#include "mathengine/units.h"
#include "mathengine/number_theory.h"
#include "mathengine/statistics.h"
#include "mathengine/complex_num.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

void render_tools_tab(AppState& state, ImVec2 content_size) {
    ImGui::BeginChild("ToolsArea", content_size);
    float padding = 16;
    ImGui::SetCursorPosX(padding);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    float content_w = std::min(ImGui::GetContentRegionAvail().x - padding * 2, 700.0f);
    ImGui::BeginChild("ToolsInner", ImVec2(content_w, 0));

    // Sub-tabs within tools
    static int tools_tab = 0;
    ImGui::RadioButton("Constants", &tools_tab, 0); ImGui::SameLine();
    ImGui::RadioButton("Unit Convert", &tools_tab, 1); ImGui::SameLine();
    ImGui::RadioButton("Number Theory", &tools_tab, 2); ImGui::SameLine();
    ImGui::RadioButton("Statistics", &tools_tab, 3); ImGui::SameLine();
    ImGui::RadioButton("Complex", &tools_tab, 4);
    ImGui::Separator();

    if (tools_tab == 0) {
        // --- Constants ---
        ImGui::Text("Physical Constants");
        ImGui::Separator();
        static char search[64] = "";
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Search...", search, sizeof(search));

        ImGui::BeginChild("ConstList", ImVec2(0, 0), ImGuiChildFlags_Border);
        for (auto& c : mathengine::get_constants()) {
            if (search[0] && c.name.find(search) == std::string::npos &&
                c.symbol.find(search) == std::string::npos) continue;

            char buf[256];
            snprintf(buf, sizeof(buf), "%-25s %-8s = %.6e  %s",
                     c.name.c_str(), c.symbol.c_str(), c.value, c.unit.c_str());
            if (ImGui::Selectable(buf)) {
                // Copy value to clipboard
                snprintf(buf, sizeof(buf), "%.15e", c.value);
                ImGui::SetClipboardText(buf);
            }
        }
        ImGui::EndChild();

    } else if (tools_tab == 1) {
        // --- Unit Conversion ---
        ImGui::Text("Unit Converter");
        ImGui::Separator();
        static double conv_value = 1.0;
        static char from_unit[32] = "m";
        static char to_unit[32] = "ft";
        static std::string conv_result;

        ImGui::SetNextItemWidth(120);
        ImGui::InputDouble("Value", &conv_value);
        ImGui::SetNextItemWidth(80);
        ImGui::InputText("From", from_unit, sizeof(from_unit));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputText("To", to_unit, sizeof(to_unit));
        ImGui::SameLine();
        if (ImGui::Button("Convert")) {
            double factor = mathengine::convert_units(from_unit, to_unit);
            if (factor > 0) {
                char buf[128];
                snprintf(buf, sizeof(buf), "%.6g %s = %.6g %s",
                         conv_value, from_unit, conv_value * factor, to_unit);
                conv_result = buf;
            } else {
                conv_result = "Cannot convert between these units";
            }
        }
        if (!conv_result.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.65f, 1.0f));
            ImGui::Text("%s", conv_result.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        ImGui::Text("Available units:");
        for (auto& cat : mathengine::get_unit_categories()) {
            if (ImGui::TreeNode(cat.c_str())) {
                auto units = mathengine::get_units_in_category(cat);
                for (auto& [name, factor] : units)
                    ImGui::BulletText("%s", name.c_str());
                ImGui::TreePop();
            }
        }

    } else if (tools_tab == 2) {
        // --- Number Theory ---
        ImGui::Text("Number Theory");
        ImGui::Separator();
        static char nt_input[64] = "360";
        static std::string nt_result;

        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Number", nt_input, sizeof(nt_input));

        float bw = (ImGui::GetContentRegionAvail().x - 12) / 3;
        if (ImGui::Button("Factor", ImVec2(bw, 0))) {
            int64_t n = std::atoll(nt_input);
            nt_result = mathengine::factorization_string(n);
        }
        ImGui::SameLine();
        if (ImGui::Button("Is Prime?", ImVec2(bw, 0))) {
            int64_t n = std::atoll(nt_input);
            nt_result = std::to_string(n) + (mathengine::is_prime(n) ? " is prime" : " is not prime");
        }
        ImGui::SameLine();
        if (ImGui::Button("Fibonacci", ImVec2(bw, 0))) {
            int n = std::atoi(nt_input);
            nt_result = "F(" + std::to_string(n) + ") = " + std::to_string(mathengine::fibonacci(n));
        }

        if (ImGui::Button("Factorial", ImVec2(bw, 0))) {
            int n = std::atoi(nt_input);
            nt_result = std::to_string(n) + "! = " + std::to_string(mathengine::factorial(n));
        }
        ImGui::SameLine();
        static char nt_input2[64] = "24";
        ImGui::SetNextItemWidth(80);
        ImGui::InputText("##n2", nt_input2, sizeof(nt_input2));
        ImGui::SameLine();
        if (ImGui::Button("GCD")) {
            int64_t a = std::atoll(nt_input), b = std::atoll(nt_input2);
            nt_result = "gcd(" + std::to_string(a) + ", " + std::to_string(b) + ") = " +
                       std::to_string(mathengine::gcd(a, b));
        }
        ImGui::SameLine();
        if (ImGui::Button("LCM")) {
            int64_t a = std::atoll(nt_input), b = std::atoll(nt_input2);
            nt_result = "lcm(" + std::to_string(a) + ", " + std::to_string(b) + ") = " +
                       std::to_string(mathengine::lcm(a, b));
        }

        if (!nt_result.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.65f, 1.0f));
            ImGui::TextWrapped("%s", nt_result.c_str());
            ImGui::PopStyleColor();
        }

    } else if (tools_tab == 3) {
        // --- Statistics ---
        ImGui::Text("Statistics");
        ImGui::Separator();
        static char data_input[1024] = "1, 2, 3, 4, 5, 6, 7, 8, 9, 10";
        static std::string stat_result;

        ImGui::TextDisabled("Enter comma-separated values:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextMultiline("##data", data_input, sizeof(data_input), ImVec2(-1, 60));

        if (ImGui::Button("Compute Stats", ImVec2(-1, 28))) {
            std::vector<double> data;
            std::istringstream ss(data_input);
            std::string token;
            while (std::getline(ss, token, ',')) {
                double v = std::atof(token.c_str());
                data.push_back(v);
            }
            if (data.size() >= 2) {
                char buf[512];
                snprintf(buf, sizeof(buf),
                    "n = %d\n"
                    "Mean = %.6g\n"
                    "Median = %.6g\n"
                    "Std Dev = %.6g\n"
                    "Variance = %.6g\n"
                    "Min = %.6g\n"
                    "Max = %.6g",
                    (int)data.size(),
                    mathengine::stat_mean(data), mathengine::stat_median(data),
                    mathengine::stat_stddev(data), mathengine::stat_variance(data),
                    mathengine::stat_min(data), mathengine::stat_max(data));
                stat_result = buf;
            } else {
                stat_result = "Need at least 2 data points";
            }
        }

        if (!stat_result.empty()) {
            ImGui::Spacing();
            ImGui::TextWrapped("%s", stat_result.c_str());
        }

        // Normal distribution calculator
        ImGui::Separator();
        ImGui::Text("Normal Distribution");
        static double norm_x = 0, norm_mu = 0, norm_sigma = 1;
        static std::string norm_result;
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("x##norm", &norm_x);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("mu", &norm_mu);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("sigma", &norm_sigma);
        ImGui::SameLine();
        if (ImGui::Button("Calc")) {
            char buf[128];
            snprintf(buf, sizeof(buf), "PDF = %.6g, CDF = %.6g",
                     mathengine::normal_pdf(norm_x, norm_mu, norm_sigma),
                     mathengine::normal_cdf(norm_x, norm_mu, norm_sigma));
            norm_result = buf;
        }
        if (!norm_result.empty()) ImGui::Text("%s", norm_result.c_str());

    } else if (tools_tab == 4) {
        // --- Complex Numbers ---
        ImGui::Text("Complex Number Calculator");
        ImGui::Separator();

        static double z1_re = 3, z1_im = 4, z2_re = 1, z2_im = 2;
        static std::string complex_result;

        ImGui::Text("z1:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("re##1", &z1_re); ImGui::SameLine();
        ImGui::Text("+"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("im##1", &z1_im); ImGui::SameLine();
        ImGui::Text("i");

        ImGui::Text("z2:"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("re##2", &z2_re); ImGui::SameLine();
        ImGui::Text("+"); ImGui::SameLine();
        ImGui::SetNextItemWidth(80); ImGui::InputDouble("im##2", &z2_im); ImGui::SameLine();
        ImGui::Text("i");

        ImGui::Spacing();
        float bw = (ImGui::GetContentRegionAvail().x - 18) / 4;
        mathengine::Complex z1(z1_re, z1_im), z2(z2_re, z2_im);

        if (ImGui::Button("z1 + z2", ImVec2(bw, 0)))
            complex_result = (z1 + z2).to_string();
        ImGui::SameLine();
        if (ImGui::Button("z1 - z2", ImVec2(bw, 0)))
            complex_result = (z1 - z2).to_string();
        ImGui::SameLine();
        if (ImGui::Button("z1 * z2", ImVec2(bw, 0)))
            complex_result = (z1 * z2).to_string();
        ImGui::SameLine();
        if (ImGui::Button("z1 / z2", ImVec2(bw, 0)))
            complex_result = (z1 / z2).to_string();

        if (ImGui::Button("|z1|", ImVec2(bw, 0)))
            complex_result = std::to_string(z1.abs());
        ImGui::SameLine();
        if (ImGui::Button("arg(z1)", ImVec2(bw, 0)))
            complex_result = std::to_string(z1.arg()) + " rad";
        ImGui::SameLine();
        if (ImGui::Button("conj(z1)", ImVec2(bw, 0)))
            complex_result = z1.conj().to_string();
        ImGui::SameLine();
        if (ImGui::Button("sqrt(z1)", ImVec2(bw, 0)))
            complex_result = mathengine::csqrt(z1).to_string();

        if (ImGui::Button("exp(z1)", ImVec2(bw, 0)))
            complex_result = mathengine::cexp(z1).to_string();
        ImGui::SameLine();
        if (ImGui::Button("ln(z1)", ImVec2(bw, 0)))
            complex_result = mathengine::clog(z1).to_string();
        ImGui::SameLine();
        if (ImGui::Button("sin(z1)", ImVec2(bw, 0)))
            complex_result = mathengine::csin(z1).to_string();
        ImGui::SameLine();
        if (ImGui::Button("z1^z2", ImVec2(bw, 0)))
            complex_result = mathengine::cpow(z1, z2).to_string();

        // Complex roots of polynomial
        ImGui::Separator();
        ImGui::Text("Complex roots of polynomial (coeffs a0, a1, ..., an):");
        static char poly_coeffs[256] = "-4, 0, 1";
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("Coefficients", poly_coeffs, sizeof(poly_coeffs));
        if (ImGui::Button("Find Roots", ImVec2(-1, 0))) {
            std::vector<double> coeffs;
            std::istringstream ss(poly_coeffs);
            std::string tok;
            while (std::getline(ss, tok, ',')) coeffs.push_back(std::atof(tok.c_str()));
            auto roots = mathengine::complex_roots(coeffs);
            complex_result = "Roots:\n";
            for (size_t i = 0; i < roots.size(); i++)
                complex_result += "  z" + std::to_string(i+1) + " = " + roots[i].to_string() + "\n";
        }

        if (!complex_result.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 1.0f, 0.65f, 1.0f));
            ImGui::TextWrapped("%s", complex_result.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
    ImGui::EndChild();
}
