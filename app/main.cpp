#include "app.h"
#include "graph_renderer.h"
#include "input_panel.h"
#include "result_panel.h"
#include "renderer3d.h"
#include "console.h"
#include "design.h"
#include "tools.h"
#include "datalab.h"
#include "implicit.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

static void apply_dark_theme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
}

static void apply_light_theme() {
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.96f, 1.00f);
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1400, 800, "MathEngine", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    apply_dark_theme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    AppState state;
    {
        PlotEntry entry;
        entry.id = state.next_plot_id++;
        entry.color = kPlotColors[0];
        snprintf(entry.expression, sizeof(entry.expression), "sin(x)");
        try_parse_plot(entry);
        state.plots.push_back(std::move(entry));
    }

    bool first_frame = true;
    auto last_theme = state.theme;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (state.theme != last_theme) {
            if (state.theme == AppState::Theme::Dark) apply_dark_theme();
            else apply_light_theme();
            last_theme = state.theme;
        }

        // Animation tick
        if (state.anim_playing) {
            double dt = ImGui::GetIO().DeltaTime;
            state.anim_time += dt * state.anim_speed;
            if (state.anim_time > state.anim_t_max)
                state.anim_time = state.anim_t_min;
            state.needs_resample = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Full-window tab bar ---
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        ImGui::Begin("##MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_MenuBar);

        ImGui::PopStyleVar(2);

        // Tab bar
        if (ImGui::BeginTabBar("##MainTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("  Graph  ")) {
                state.active_tab = AppState::Tab::Graph;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Calculator  ")) {
                state.active_tab = AppState::Tab::Calculator;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  3D  ")) {
                state.active_tab = AppState::Tab::ThreeD;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Matrix  ")) {
                state.active_tab = AppState::Tab::Matrix;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Data  ")) {
                state.active_tab = AppState::Tab::Data;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Implicit  ")) {
                state.active_tab = AppState::Tab::Implicit;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Design  ")) {
                state.active_tab = AppState::Tab::Design;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Tools  ")) {
                state.active_tab = AppState::Tab::Tools;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("  Console  ")) {
                state.active_tab = AppState::Tab::Console;
                ImGui::EndTabItem();
            }

            // Theme toggle
            float avail = ImGui::GetContentRegionAvail().x;
            if (avail > 120) {
                ImGui::SameLine(ImGui::GetCursorPosX() + avail - 110);
                bool is_dark = state.theme == AppState::Theme::Dark;
                if (ImGui::SmallButton(is_dark ? "Light Mode" : "Dark Mode")) {
                    state.theme = is_dark ? AppState::Theme::Light : AppState::Theme::Dark;
                }
            }

            ImGui::EndTabBar();
        }

        // --- Tab content ---
        ImVec2 content_size = ImGui::GetContentRegionAvail();

        if (state.active_tab == AppState::Tab::Graph) {
            // Graph tab: dockspace with Controls | Results | Graph
            ImGuiID graph_dock_id = ImGui::GetID("GraphDockspace");
            ImGui::DockSpace(graph_dock_id, content_size);

            if (first_frame) {
                first_frame = false;
                ImGui::DockBuilderRemoveNode(graph_dock_id);
                ImGui::DockBuilderAddNode(graph_dock_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(graph_dock_id, content_size);

                ImGuiID left_id, right_id;
                ImGui::DockBuilderSplitNode(graph_dock_id, ImGuiDir_Left, 0.25f, &left_id, &right_id);

                ImGuiID left_top_id, left_bottom_id;
                ImGui::DockBuilderSplitNode(left_id, ImGuiDir_Up, 0.65f, &left_top_id, &left_bottom_id);

                ImGui::DockBuilderDockWindow("Controls", left_top_id);
                ImGui::DockBuilderDockWindow("Results", left_bottom_id);
                ImGui::DockBuilderDockWindow("Graph", right_id);
                ImGui::DockBuilderFinish(graph_dock_id);
            }

            render_input_panel(state);
            render_result_panel(state);

            ImGui::Begin("Graph");
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 cursor = ImGui::GetCursorScreenPos();
            if (avail.x > 50 && avail.y > 50) {
                render_graph(state, cursor, avail);
            }
            ImGui::End();

        } else if (state.active_tab == AppState::Tab::Calculator) {
            render_cas_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::ThreeD) {
            render_3d_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Matrix) {
            render_cas_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Data) {
            render_data_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Implicit) {
            render_implicit_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Design) {
            render_design_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Tools) {
            render_tools_tab(state, content_size);
        } else if (state.active_tab == AppState::Tab::Console) {
            render_console_tab(state, content_size);
        }

        // --- Command Palette (Cmd+K) ---
        if (ImGui::GetIO().KeySuper && ImGui::IsKeyPressed(ImGuiKey_K)) {
            state.cmd_palette_open = !state.cmd_palette_open;
            state.cmd_palette_query[0] = '\0';
        }
        if (state.cmd_palette_open) {
            ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x/2 - 200,
                                          viewport->WorkPos.y + 80));
            ImGui::SetNextWindowSize(ImVec2(400, 0));
            ImGui::Begin("##CmdPalette", &state.cmd_palette_open,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            ImGui::SetNextItemWidth(-1);
            bool enter = ImGui::InputTextWithHint("##cmd", "Type a command...",
                state.cmd_palette_query, sizeof(state.cmd_palette_query),
                ImGuiInputTextFlags_EnterReturnsTrue);

            struct Cmd { const char* name; AppState::Tab tab; };
            Cmd commands[] = {
                {"Graph", AppState::Tab::Graph},
                {"Calculator", AppState::Tab::Calculator},
                {"3D Surface", AppState::Tab::ThreeD},
                {"Matrix", AppState::Tab::Matrix},
                {"Data Lab", AppState::Tab::Data},
                {"Implicit Curves", AppState::Tab::Implicit},
                {"Design", AppState::Tab::Design},
                {"Tools / Constants", AppState::Tab::Tools},
                {"Console", AppState::Tab::Console},
            };

            std::string q = state.cmd_palette_query;
            for (auto& c : q) c = std::tolower(c);

            for (auto& cmd : commands) {
                std::string name = cmd.name;
                std::string lower_name = name;
                for (auto& c : lower_name) c = std::tolower(c);
                if (!q.empty() && lower_name.find(q) == std::string::npos) continue;

                if (ImGui::Selectable(cmd.name) || (enter && q == lower_name)) {
                    state.active_tab = cmd.tab;
                    state.cmd_palette_open = false;
                }
            }

            // Special commands
            if (q == "dark" || q == "dark mode" || q == "dark theme") {
                if (ImGui::Selectable("Switch to Dark Mode") || enter) {
                    state.theme = AppState::Theme::Dark;
                    state.cmd_palette_open = false;
                }
            }
            if (q == "light" || q == "light mode" || q == "light theme") {
                if (ImGui::Selectable("Switch to Light Mode") || enter) {
                    state.theme = AppState::Theme::Light;
                    state.cmd_palette_open = false;
                }
            }
            if (q == "export" || q == "png") {
                if (ImGui::Selectable("Export PNG") || enter) {
                    state.export_requested = true;
                    state.cmd_palette_open = false;
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) state.cmd_palette_open = false;
            ImGui::End();
        }

        ImGui::End(); // MainWindow

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        float clear_r = (state.theme == AppState::Theme::Dark) ? 0.08f : 0.92f;
        float clear_g = (state.theme == AppState::Theme::Dark) ? 0.08f : 0.92f;
        float clear_b = (state.theme == AppState::Theme::Dark) ? 0.10f : 0.94f;
        glClearColor(clear_r, clear_g, clear_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // PNG export
        if (state.export_requested) {
            state.export_requested = false;
            int w = display_w, h = display_h;
            std::vector<unsigned char> pixels(w * h * 3);
            glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
            std::vector<unsigned char> flipped(w * h * 3);
            for (int row = 0; row < h; row++) {
                memcpy(&flipped[row * w * 3], &pixels[(h - 1 - row) * w * 3], w * 3);
            }
            time_t now = time(nullptr);
            char filename[128];
            strftime(filename, sizeof(filename), "graph_%Y%m%d_%H%M%S.png", localtime(&now));
            stbi_write_png(filename, w, h, 3, flipped.data(), w * 3);
            state.result_text = std::string("Exported: ") + filename;
        }

        // --- Native file picker for background image ---
        if (state.bg_image_pick_requested) {
            state.bg_image_pick_requested = false;
#ifdef __APPLE__
            FILE* pipe = popen(
                "osascript -e 'try' "
                "-e 'POSIX path of (choose file of type {\"png\", \"jpg\", \"jpeg\", \"bmp\", \"gif\", \"tga\", \"tiff\"} "
                "with prompt \"Choose background image\")' "
                "-e 'on error' -e 'return \"\"' -e 'end try' 2>/dev/null", "r");
            if (pipe) {
                char buf[1024] = {0};
                if (fgets(buf, sizeof(buf), pipe)) {
                    size_t len = strlen(buf);
                    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = '\0';
                    if (len > 0) {
                        snprintf(state.bg_image_path, sizeof(state.bg_image_path), "%s", buf);
                        state.bg_image_load_requested = true;
                    }
                }
                pclose(pipe);
            }
#elif defined(_WIN32)
            extern int win32_open_file_dialog(char* out, size_t out_size);
            char buf[1024] = {0};
            if (win32_open_file_dialog(buf, sizeof(buf))) {
                snprintf(state.bg_image_path, sizeof(state.bg_image_path), "%s", buf);
                state.bg_image_load_requested = true;
            }
#else
            // Linux: TODO use zenity or kdialog
#endif
        }

        // --- Load background image for design tab ---
        if (state.bg_image_load_requested) {
            state.bg_image_load_requested = false;
            int iw, ih, ic;
            unsigned char* data = stbi_load(state.bg_image_path, &iw, &ih, &ic, 4);
            if (data) {
                // Delete existing texture
                if (state.bg_image_texture != 0) {
                    GLuint tex = (GLuint)state.bg_image_texture;
                    glDeleteTextures(1, &tex);
                }
                GLuint tex_id = 0;
                glGenTextures(1, &tex_id);
                glBindTexture(GL_TEXTURE_2D, tex_id);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iw, ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
                state.bg_image_texture = (unsigned int)tex_id;
                state.bg_image_width = iw;
                state.bg_image_height = ih;
                // Reset transform on new load
                state.bg_image_offset_x = 0;
                state.bg_image_offset_y = 0;
                state.bg_image_scale = 1.0f;
                char buf[256];
                snprintf(buf, sizeof(buf), "Loaded %dx%d", iw, ih);
                state.bg_image_status = buf;
            } else {
                state.bg_image_status = std::string("Failed: ") + stbi_failure_reason();
            }
        }

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
