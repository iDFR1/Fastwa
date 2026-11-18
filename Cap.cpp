#include "imgui.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// هيكل بيانات اللاعب الإداري
struct AdminPlayer {
    int serverId;
    std::string name;
    std::string job;
    float health;
    float distance;
    ImVec2 screenPos; // إحداثيات الشاشة (WorldToScreen)
    bool isOnScreen;
    bool selected = false; // مفتاح التبديل (Switch) لكل لاعب
};

// هيكل الإعدادات والتحكم الكامل
struct AdminSystemConfig {
    // 1. إعدادات الـ Admin ESP
    bool enableESP = true;
    bool showNames = true;
    bool showJobs = true;
    bool showHealthBar = true;
    bool showDistance = true;
    bool showSnaplines = false;
    float maxDistance = 300.0f;
    float espColor[4] = { 0.2f, 0.7f, 1.0f, 1.0f }; // أزرق نيون

    // 2. إعدادات دائرة نطاق المراقبة (Scan Circle Overlay)
    bool enableScanCircle = true;
    float scanRadius = 150.0f;
    bool highlightInScan = true;
    float scanCircleColor[4] = { 0.0f, 0.85f, 1.0f, 0.7f };

    // 3. إعدادات الكاميرا الحرة (FreeCam)
    bool enableFreeCam = false;
    float freeCamSpeed = 2.0f;
    int freeCamMode = 0; // 0: Spectate, 1: Teleport Here, 2: Marker

    // قائمة اللاعبين التجريبية (تتحدث ديناميكياً من اللعبة)
    std::vector<AdminPlayer> players = {
        { 1, "Ahmed_KSA", "Police", 100.0f, 14.5f, {0,0}, false, false },
        { 2, "Faisal_XD", "Mechanic", 85.0f, 62.0f, {0,0}, false, false },
        { 3, "Saud_99", "Civilian", 50.0f, 120.3f, {0,0}, false, false }
    };
};

AdminSystemConfig adminSys;

// دالة رسم الـ Overlays على الشاشة (ESP + Scan Circle)
void RenderAdminOverlays(ImDrawList* drawList) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screenCenter(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);

    // 1. رسم دائرة نطاق المراقبة (Scan Range Circle)
    if (adminSys.enableScanCircle) {
        ImU32 circleCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
            adminSys.scanCircleColor[0], adminSys.scanCircleColor[1],
            adminSys.scanCircleColor[2], adminSys.scanCircleColor[3]
        ));
        drawList->AddCircle(screenCenter, adminSys.scanRadius, circleCol, 120, 1.5f);
    }

    // 2. رسم الـ ESP الإداري لكل لاعب
    if (adminSys.enableESP) {
        ImU32 defaultTextCol = ImGui::ColorConvertFloat4ToU32(ImVec4(
            adminSys.espColor[0], adminSys.espColor[1], adminSys.espColor[2], adminSys.espColor[3]
        ));

        for (const auto& p : adminSys.players) {
            if (!p.isOnScreen || p.distance > adminSys.maxDistance) continue;

            // حساب هل اللاعب داخل دائرة النطاق أم لا
            float dx = p.screenPos.x - screenCenter.x;
            float dy = p.screenPos.y - screenCenter.y;
            bool inRange = (dx * dx + dy * dy) <= (adminSys.scanRadius * adminSys.scanRadius);

            ImU32 activeCol = (adminSys.highlightInScan && inRange) 
                              ? IM_COL32(255, 215, 0, 255) // ذهبي إذا كان داخل النطاق
                              : defaultTextCol;

            float yOffset = 0.0f;

            // عرض الاسم والآيدي
            if (adminSys.showNames) {
                std::string header = "[" + std::to_string(p.serverId) + "] " + p.name;
                drawList->AddText(ImVec2(p.screenPos.x, p.screenPos.y + yOffset), activeCol, header.c_str());
                yOffset += 14.0f;
            }

            // عرض الوظيفة
            if (adminSys.showJobs) {
                std::string jobStr = "Job: " + p.job;
                drawList->AddText(ImVec2(p.screenPos.x, p.screenPos.y + yOffset), IM_COL32(200, 200, 200, 255), jobStr.c_str());
                yOffset += 14.0f;
            }

            // عرض المسافة
            if (adminSys.showDistance) {
                std::string distStr = std::to_string((int)p.distance) + "m";
                drawList->AddText(ImVec2(p.screenPos.x, p.screenPos.y + yOffset), IM_COL32(160, 160, 160, 255), distStr.c_str());
                yOffset += 14.0f;
            }

            // شريط الصحة (Health Bar)
            if (adminSys.showHealthBar) {
                float barW = 50.0f;
                float hpRatio = std::clamp(p.health / 100.0f, 0.0f, 1.0f);
                drawList->AddRectFilled(ImVec2(p.screenPos.x, p.screenPos.y + yOffset), ImVec2(p.screenPos.x + barW, p.screenPos.y + yOffset + 4.0f), IM_COL32(40, 40, 40, 200));
                drawList->AddRectFilled(ImVec2(p.screenPos.x, p.screenPos.y + yOffset), ImVec2(p.screenPos.x + (barW * hpRatio), p.screenPos.y + yOffset + 4.0f), IM_COL32(0, 230, 118, 255));
            }

            // خطوط التتبع (Snaplines)
            if (adminSys.showSnaplines) {
                drawList->AddLine(ImVec2(screenCenter.x, io.DisplaySize.y), p.screenPos, activeCol, 1.0f);
            }
        }
    }
}

// دالة رسم واجهة المنيو الرئيسية (GUI)
void RenderAdminGUI() {
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("FiveM Server Admin Control Panel", nullptr, ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("AdminPanelTabs")) {

        // ==========================================
        // تبويب 1: قائمة اللاعبين (ترتيب تلقائي + Switches)
        // ==========================================
        if (ImGui::BeginTabItem("Online Players")) {
            // ترتيب اللاعبين تلقائياً من الأقرب إلى الأبعد
            std::sort(adminSys.players.begin(), adminSys.players.end(), 
                [](const AdminPlayer& a, const AdminPlayer& b) {
                    return a.distance < b.distance;
                });

            ImGui::Text("Players List (Sorted: Nearest to Farthest):");
            ImGui::Separator();

            ImGui::BeginChild("PlayersScrollArea", ImVec2(0, 210), true);
            for (auto& p : adminSys.players) {
                std::string label = "[" + std::to_string(p.serverId) + "] " + p.name + " (" + p.job + ") - " + std::to_string((int)p.distance) + "m";
                ImGui::Checkbox(label.c_str(), &p.selected); // Switch / Checkbox لكل لاعب
            }
            ImGui::EndChild();

            ImGui::Spacing();
            if (ImGui::Button("Select All", ImVec2(90, 25))) { for(auto& p : adminSys.players) p.selected = true; }
            ImGui::SameLine();
            if (ImGui::Button("Deselect All", ImVec2(90, 25))) { for(auto& p : adminSys.players) p.selected = false; }
            ImGui::SameLine();
            if (ImGui::Button("Bring Selected", ImVec2(110, 25))) { /* تنفيذ أمر سحب اللاعب */ }
            ImGui::SameLine();
            if (ImGui::Button("Freeze / Unfreeze", ImVec2(130, 25))) { /* تنفيذ أمر التجميد */ }

            ImGui::EndTabItem();
        }

        // ==========================================
        // تبويب 2: إعدادات الـ ESP ونطاق المراقبة
        // ==========================================
        if (ImGui::BeginTabItem("ESP & Scan Radar")) {
            ImGui::Checkbox("Enable Admin ESP", &adminSys.enableESP);
            if (adminSys.enableESP) {
                ImGui::Indent();
                ImGui::Checkbox("Show Names & IDs", &adminSys.showNames);
                ImGui::Checkbox("Show Player Jobs", &adminSys.showJobs);
                ImGui::Checkbox("Show Health Bars", &adminSys.showHealthBar);
                ImGui::Checkbox("Show Distance", &adminSys.showDistance);
                ImGui::Checkbox("Show Snaplines", &adminSys.showSnaplines);
                ImGui::SliderFloat("Max Distance", &adminSys.maxDistance, 50.0f, 1000.0f, "%.0f m");
                ImGui::ColorEdit4("ESP Color", adminSys.espColor);
                ImGui::Unindent();
            }

            ImGui::Separator();

            ImGui::Checkbox("Enable Scan Circle Overlay", &adminSys.enableScanCircle);
            if (adminSys.enableScanCircle) {
                ImGui::Indent();
                ImGui::Checkbox("Highlight Players Inside Circle", &adminSys.highlightInScan);
                ImGui::SliderFloat("Circle Radius", &adminSys.scanRadius, 50.0f, 400.0f, "%.0f px");
                ImGui::ColorEdit4("Circle Color", adminSys.scanCircleColor);
                ImGui::Unindent();
            }

            ImGui::EndTabItem();
        }

        // ==========================================
        // تبويب 3: الكاميرا الحرة (FreeCam)
        // ==========================================
        if (ImGui::BeginTabItem("FreeCam Controls")) {
            ImGui::Checkbox("Activate FreeCam Mode", &adminSys.enableFreeCam);
            if (adminSys.enableFreeCam) {
                ImGui::SliderFloat("Camera Speed", &adminSys.freeCamSpeed, 0.5f, 15.0f, "%.1f");
                const char* modes[] = { "Spectate Only", "Teleport to Cursor", "Marker / Action Point" };
                ImGui::Combo("Click Action Mode", &adminSys.freeCamMode, modes, IM_ARRAYSIZE(modes));
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
