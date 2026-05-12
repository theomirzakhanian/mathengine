#pragma once

#include "app.h"
#include "imgui.h"

// Graph tab: controls panel (docked left in graph tab)
void render_input_panel(AppState& state);

// CAS tabs: algebra, calculus, solve, matrix workspaces
void render_cas_tab(AppState& state, ImVec2 content_size);
