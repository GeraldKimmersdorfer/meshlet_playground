#include "hud.h"
#include <imgui.h>
#include <cstdint>

static inline ImVec4 rgba8(uint8_t r, uint8_t g, uint8_t b, float a) {
    return ImVec4(
        r / (float)UINT8_MAX,
        g / (float)UINT8_MAX,
        b / (float)UINT8_MAX,
        a
    );
}

void activateImGuiStyle(bool darkMode, float alpha)
{
    ImGuiStyle& style = ImGui::GetStyle();

    //https://colorhunt.co/palette/faf1e4cedebd9eb384435334
    auto COL_BACKGROUND = rgba8(250, 241, 228, 0.8f);
    auto COL_ACCENT = rgba8(206, 222, 189, 0.9f);
    auto COL_ACCENT_DARKER = rgba8(158, 179, 132, 0.9f);
    auto COL_ACCENT_EVEN_DARKER = rgba8(67, 83, 52, 0.9f);

    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = COL_BACKGROUND;
    style.Colors[ImGuiCol_PopupBg] = COL_BACKGROUND;
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg] = COL_BACKGROUND;
    style.Colors[ImGuiCol_FrameBgHovered] = COL_ACCENT;
    style.Colors[ImGuiCol_FrameBgActive] = COL_ACCENT_DARKER;
    style.Colors[ImGuiCol_TitleBg] = COL_ACCENT_DARKER;
    style.Colors[ImGuiCol_TitleBgCollapsed] = COL_ACCENT;
    style.Colors[ImGuiCol_TitleBgActive] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_MenuBarBg] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_ScrollbarBg] = COL_BACKGROUND;
    style.Colors[ImGuiCol_ScrollbarGrab] = COL_ACCENT;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = COL_ACCENT_DARKER;
    style.Colors[ImGuiCol_CheckMark] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_SliderGrab] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_SliderGrabActive] = COL_ACCENT_DARKER;
    //style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_Button] = COL_ACCENT;
    style.Colors[ImGuiCol_ButtonHovered] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_ButtonActive] = COL_ACCENT_DARKER;
    style.Colors[ImGuiCol_Header] = COL_ACCENT;
    style.Colors[ImGuiCol_HeaderHovered] = COL_ACCENT_EVEN_DARKER;
    style.Colors[ImGuiCol_HeaderActive] = COL_ACCENT_DARKER;
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

    if (darkMode)
    {
        for (int i = 0; i <= ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            float H, S, V;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

            if (S < 0.1f)
            {
                V = 1.0f - V;
            }
            ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
            if (col.w < 1.00f)
            {
                col.w *= alpha;
            }
        }
    }
    else
    {
        for (int i = 0; i <= ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            if (col.w < 1.00f)
            {
                col.x *= alpha;
                col.y *= alpha;
                col.z *= alpha;
                col.w *= alpha;
            }
        }
    }

}