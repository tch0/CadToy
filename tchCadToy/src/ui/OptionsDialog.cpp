// 对应头文件
#include "OptionsDialog.h"

// C++ 标准库
#include <algorithm>

// 第三方库
#include <imgui.h>

// 项目头文件
#include "DocManager.h"
#include "DisplayConfigManager.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "Renderer.h"

namespace tch {

OptionsDialog::OptionsDialog()
    : m_visible(false)
    , m_tempShowGrid(false)
    , m_tempShowAxes(false)
    , m_tempCrossCursorSize(50)
    , m_tempPickBoxSize(5)
    , m_tempFontSize(18) {
}

OptionsDialog& OptionsDialog::getInstance() {
    static OptionsDialog instance;
    return instance;
}

void OptionsDialog::initialize() {
    // 从各系统获取当前配置
    m_tempShowGrid = DocManager::getCurrentDocument().isShowGrid();
    m_tempShowAxes = DocManager::getCurrentDocument().isShowAxes();
    m_tempCrossCursorSize = static_cast<int>(Renderer::getCrossCursorSize());
    m_tempPickBoxSize = static_cast<int>(Renderer::getPickBoxSize());
    m_tempFontSize = DisplayConfigManager::getInstance().getCurrentFontSize();
    
    // 设置可见性
    m_visible = true;
}

void OptionsDialog::writeBack() {
    // 将临时变量写回各系统
    DocManager::getCurrentDocument().setShowGrid(m_tempShowGrid);
    DocManager::getCurrentDocument().setShowAxes(m_tempShowAxes);
    Renderer::setCrossCursorSize(static_cast<float>(m_tempCrossCursorSize));
    Renderer::setPickBoxSize(static_cast<float>(m_tempPickBoxSize));
    DisplayConfigManager::getInstance().setFontSize(m_tempFontSize);
}

bool OptionsDialog::show() {
    if (!m_visible) {
        return false;
    }
    
    auto& loc = LocalizationManager::getInstance();
    float uiScale = Utils::getUIScaleFactor();
    
    // 使用BeginPopupModal创建真正的模态对话框
    ImGui::OpenPopup("Options");
    
    // 设置对话框位置为屏幕中央
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    // 使用模态对话框标志，允许调整大小
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::BeginPopupModal("Options", &m_visible, flags)) {
        // 对话框标题
        ImGui::Text(loc.get("optionsDialog.title").c_str());
        ImGui::Separator();
        
        // 创建选项卡栏
        if (ImGui::BeginTabBar("OptionsTabs")) {
            // 第一个选项卡：显示
            if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.display").c_str())) {
                // Grid & Axes 标题
                ImGui::Text(loc.get("optionsDialog.gridAxes").c_str());
                ImGui::Spacing();
                
                // 选项
                ImGui::Checkbox(loc.get("optionsDialog.showGrid").c_str(), &m_tempShowGrid);
                ImGui::Checkbox(loc.get("optionsDialog.showAxes").c_str(), &m_tempShowAxes);
                
                // 在十字光标大小设置前添加分隔线，与前面的栅格坐标轴设置分开
                ImGui::Separator();
                
                // 十字光标大小
                ImGui::Spacing();
                ImGui::Text(loc.get("optionsDialog.crossCursorSize").c_str());
                ImGui::Spacing();
                
                // 滑块控件，范围10-200，使用整数
                ImGui::PushItemWidth(500.0f * uiScale);
                ImGui::SliderInt("##CrossCursorSize", &m_tempCrossCursorSize, 10, 200, "%d");
                ImGui::PopItemWidth();
                
                ImGui::EndTabItem();
            }
            
            // 第二个选项卡：选择集
            if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.selection").c_str())) {
                // 拾取框大小
                ImGui::Spacing();
                ImGui::Text(loc.get("optionsDialog.pickBoxSize").c_str());
                ImGui::Spacing();
                
                // 首先绘制预览框
                ImGui::BeginGroup();
                
                // 创建一个更大的预览区域，确保最大拾取框也能完全显示
                ImVec2 previewSize(120.0f, 120.0f);
                ImGui::BeginChild("Preview", previewSize, true);
                
                // 计算预览框的位置和大小
                ImVec2 previewPos = ImGui::GetCursorScreenPos();
                ImVec2 center(previewSize.x / 2 - 8, previewSize.y / 2 - 8);
                
                // 绘制预览框（正方形）
                ImGui::GetWindowDrawList()->AddRect(
                    ImVec2(previewPos.x + center.x - m_tempPickBoxSize, previewPos.y + center.y - m_tempPickBoxSize),
                    ImVec2(previewPos.x + center.x + m_tempPickBoxSize, previewPos.y + center.y + m_tempPickBoxSize),
                    IM_COL32(255, 255, 255, 255),
                    0.0f,
                    0,
                    1.0f
                );
                
                ImGui::EndChild();
                ImGui::EndGroup();
                
                // 然后在左侧绘制滑块，对齐到预览框底部
                ImGui::SameLine(0.0f, 20.0f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50.0f);
                
                ImGui::PushItemWidth(250.0f * uiScale);
                ImGui::SliderInt("##PickBoxSize", &m_tempPickBoxSize, 0, 50, "%d");
                ImGui::PopItemWidth();
                
                ImGui::EndTabItem();
            }
            
            // 第三个选项卡：UI
            if (ImGui::BeginTabItem(loc.get("optionsDialog.tab.ui").c_str())) {
                // 语言选择
                ImGui::Spacing();
                ImGui::Text(loc.get("optionsDialog.language").c_str());
                ImGui::Spacing();
                
                // 获取当前语言
                std::string currentLanguage = loc.getCurrentLanguage();
                
                // 语言选择下拉框，显示固定的语言选项
                if (ImGui::BeginCombo("##LanguageSelect", (currentLanguage == "en" ? "English" : "中文"))) {
                    bool isEnglishSelected = (currentLanguage == "en");
                    if (ImGui::Selectable("English", isEnglishSelected)) {
                        loc.setLanguage("en");
                    }
                    if (isEnglishSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                    
                    bool isChineseSelected = (currentLanguage == "zh");
                    if (ImGui::Selectable("中文", isChineseSelected)) {
                        loc.setLanguage("zh");
                    }
                    if (isChineseSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                
                ImGui::Separator();
                
                // 字体大小
                ImGui::Spacing();
                ImGui::Text(loc.get("optionsDialog.fontSize").c_str());
                ImGui::Spacing();
                
                ImGui::PushItemWidth(300.0f * uiScale);
                ImGui::SliderInt("##FontSize", &m_tempFontSize, 18, 50, "%d");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::SetNextItemWidth(60.0f * uiScale);
                ImGui::InputInt("##FontSizeInput", &m_tempFontSize, 0, 0);
                m_tempFontSize = std::clamp(m_tempFontSize, 18, 50);
                
                ImGui::SameLine();
                ImGui::PushFont(nullptr, static_cast<float>(m_tempFontSize));
                ImGui::Text("%s", loc.get("optionsDialog.fontPreview").c_str());
                ImGui::PopFont();
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        // 垂直填充空间，将按钮推到底部上方一定距离
        float availHeight = ImGui::GetContentRegionAvail().y;
        float buttonAreaHeight = 100.0f * uiScale;
        if (availHeight > buttonAreaHeight) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availHeight - buttonAreaHeight);
        }
        
        // 底部分隔线
        ImGui::Separator();
        
        // 右对齐按钮
        float buttonWidth = 80.0f * uiScale;
        float buttonHeight = 30.0f * uiScale;
        float buttonSpacing = ImGui::GetStyle().ItemSpacing.x;
        float totalButtonWidth = buttonWidth * 3 + buttonSpacing * 2;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - totalButtonWidth - ImGui::GetStyle().WindowPadding.x);
        
        // 应用
        bool applyClicked = ImGui::Button(loc.get("optionsDialog.apply").c_str(), ImVec2(buttonWidth, buttonHeight));
        // 确定：点击确定或者按下Enter
        ImGui::SameLine();
        bool okClicked = ImGui::Button(loc.get("optionsDialog.ok").c_str(), ImVec2(buttonWidth, buttonHeight))
                         || (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter));
        // 取消：点击取消按钮或者按下Esc
        ImGui::SameLine();
        bool cancelClicked = ImGui::Button(loc.get("optionsDialog.cancel").c_str(), ImVec2(buttonWidth, buttonHeight))
                             || ImGui::IsKeyPressed(ImGuiKey_Escape);
        
        if (applyClicked || okClicked) {
            writeBack();
        }
        
        if (okClicked || cancelClicked) {
            ImGui::CloseCurrentPopup();
            m_visible = false;
        }
        
        ImGui::EndPopup();
    }
    
    return m_visible;
}

} // namespace tch
