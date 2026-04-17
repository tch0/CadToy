// 对应头文件
#include "GlobalUtils.h"

// C++ 标准库
#include <algorithm>
#include <vector>
#include <fstream>

// 第三方库
#include <imgui.h>

// 项目头文件
#include "Renderer.h"
#include "DisplayConfigManager.h"
#include "DocManager.h"
#include "Global.h"
#include "LocalizationManager.h"
#include "StringUtils.h"


namespace tch {
namespace Utils {

// ============================================================================
// 通用全局函数
// ============================================================================

// 全局输出函数，在命令栏中输出信息
void cmdLinePrint(const std::string& content) {
    Renderer::addContentToCommandLineHistory(content);
}

// 获取全局UI缩放比例，用于快速计算UI布局，因为UI随字号变化而变化，所以只与字号相关
float getUIScaleFactor() {
    return DisplayConfigManager::getInstance().getCurrentFontSize() / 18.0f;
}

// 获取当前文档对应Database
Database* getWorkingDatabase() {
    return DocManager::getCurrentDocument().getDatabase();
}

// ============================================================================
// 文件对话框实现
// ============================================================================

namespace {
    // 静态变量保存对话框状态（使用 std::filesystem::path 内部处理编码）
    std::filesystem::path s_currentPath; // 当前浏览路径（同时作为上次路径）
    std::string s_fileName;              // 当前输入的文件名（UTF-8）
    std::vector<std::string> s_dirs;     // 当前目录的子目录名（UTF-8）
    std::vector<std::string> s_files;    // 当前目录的 .cad.json 文件名（UTF-8，不含后缀）
    
    const std::string FILE_EXTENSION = ".cad.json";
    
    constexpr size_t kNameBufSize = 256;
    char s_nameBuf[kNameBufSize] = {}; // 静态缓冲区，避免频繁分配栈空间
    
    // 刷新当前目录的文件列表
    void refreshFileList() {
        s_dirs.clear();
        s_files.clear();
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(s_currentPath)) {
                if (entry.is_directory()) {
                    s_dirs.push_back(PathUtils::toString(entry.path().filename()));
                } else if (entry.is_regular_file()) {
                    std::string filename = PathUtils::toString(entry.path().filename());
                    // 检查是否是 .cad.json
                    if (filename.size() >= FILE_EXTENSION.size() &&
                        filename.substr(filename.size() - FILE_EXTENSION.size()) == FILE_EXTENSION) {
                        // 存储不带后缀的文件名
                        s_files.push_back(filename.substr(0, filename.size() - FILE_EXTENSION.size()));
                    }
                }
            }
            
            std::sort(s_dirs.begin(), s_dirs.end());
            std::sort(s_files.begin(), s_files.end());
        } catch (...) {
            // 目录访问失败，忽略
        }
    }
    
    // 分解路径为各级目录
    void splitPath(const std::filesystem::path& path, std::vector<std::filesystem::path>& parts) {
        parts.clear();
        std::filesystem::path current = path;
        while (!current.empty()) {
            // 如果当前路径 filename 为空（末尾有分隔符），跳过添加
            // 这种情况发生在传入的路径以分隔符结尾时
            if (!current.filename().empty()) {
                parts.push_back(current);
            }
            std::filesystem::path parent = current.parent_path();
            // 检查是否到达根目录（parent 和 current 相同）
            if (parent == current) {
                // 根目录的 filename 为空，需要单独添加
                parts.push_back(parent);
                break;
            }
            current = parent;
        }
    }
}

void showFileDialog(bool& bShowDialog, bool& bReturned, std::filesystem::path& outFullPath,
                   bool isOpen, const std::string& initialFileName,
                   const std::filesystem::path& initialPath, const std::string& title) {
    
    auto& loc = LocalizationManager::getInstance();
    
    // 检测是否是本次显示的第一帧（bShowDialog 从 false 变为 true）
    static bool s_wasShowing = false;
    bool isFirstFrame = bShowDialog && !s_wasShowing;
    s_wasShowing = bShowDialog;
    
    // 第一帧初始化路径和文件名
    if (isFirstFrame) {
        // 设置路径
        if (!initialPath.empty()) {
            s_currentPath = initialPath;
        } else if (s_currentPath.empty()) {
            // 使用可执行文件所在目录
            s_currentPath = g_pathCwd;
        }
        refreshFileList();
        
        // 设置文件名
        if (!initialFileName.empty()) {
            s_fileName = initialFileName;
        } else if (s_fileName.empty() && !isOpen) {
            // 保存模式下文件名为空，使用默认名
            s_fileName = "unnamed";
        }
    }
    
    std::string windowTitle = title.empty() ? (isOpen ? loc.get("fileDialog.title.open") : loc.get("fileDialog.title.save")) : title;
    
    // 设置模态对话框
    ImGui::OpenPopup(windowTitle.c_str());
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    // 窗口固定在中央
    if (ImGui::BeginPopupModal(windowTitle.c_str(), &bShowDialog, ImGuiWindowFlags_NoMove)) {
        
        // ========== 顶部：路径导航 ==========
        std::vector<std::filesystem::path> pathParts;
        splitPath(s_currentPath, pathParts);
        
        std::string comboLabel = "##PathTree";
        std::string comboPreview = PathUtils::toString(s_currentPath);  // UTF-8
        
        if (ImGui::BeginCombo(comboLabel.c_str(), comboPreview.c_str())) {
            // 倒序显示（从根到当前）
            int indent = 0;
            for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it, ++indent) {
                // 对于根目录，filename() 返回空，使用完整路径字符串
                std::string name = PathUtils::toString(it->filename());
                if (name.empty()) {
                    name = PathUtils::toString(*it);
                }
                // 确保 display 不为空（添加空格防止 ImGui 断言失败）
                std::string display = std::string(indent * 2, ' ') + name;
                if (display.empty() || display.find_first_not_of(' ') == std::string::npos) {
                    display = " ";
                }
                if (ImGui::Selectable(display.c_str())) {
                    s_currentPath = *it;
                    refreshFileList();
                }
            }
            ImGui::EndCombo();
        }
        
        // ImGui::Separator();
        
        // ========== 中间：文件和目录列表 ==========
        ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true);
        
        // 父目录 ".."
        std::filesystem::path parentPath = s_currentPath.parent_path();
        if (!parentPath.empty() && parentPath != s_currentPath) {
            if (ImGui::Selectable("[..]##parent", false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    s_currentPath = parentPath;
                    refreshFileList();
                }
            }
        }
        
        // 子目录
        int dirIndex = 0;
        for (const auto& dir : s_dirs) {
            std::string label = "[D] " + dir + "/##dir" + std::to_string(dirIndex++);
            if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    s_currentPath /= PathUtils::toPath(dir);
                    refreshFileList();
                }
            }
        }
        
        // 文件
        int fileIndex = 0;
        for (const auto& file : s_files) {
            bool selected = (s_fileName == file);
            std::string label = "[F] " + file + FILE_EXTENSION + "##file" + std::to_string(fileIndex++);
            if (ImGui::Selectable(label.c_str(), selected)) {
                s_fileName = file;
            }
            // 双击文件确认（仅打开模式）
            if (isOpen && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                s_fileName = file;
                outFullPath = s_currentPath / PathUtils::toPath(s_fileName + FILE_EXTENSION);
                bReturned = true;
                bShowDialog = false;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndChild();
        
        // ImGui::Separator();
        
        // ========== 底部：输入框和按钮 ==========
        // 计算布局：文件名标签 + 输入框 + 后缀 + 保存按钮 + 取消按钮
        float scale = getUIScaleFactor();
        float availWidth = ImGui::GetContentRegionAvail().x;
        float extWidth = 150 * scale;       // 后缀选择宽度
        float btnWidth = 100 * scale;       // 按钮宽度
        float btnSpacing = 8 * scale;       // 按钮间距
        float padding = 150 * scale;        // 额外间距，还要包括文字宽度、各种间距，调试得出
        
        
        // 计算输入框宽度，确保给按钮留出足够空间
        float buttonsTotalWidth = btnWidth * 2 + btnSpacing;
        float inputWidth = availWidth - extWidth - buttonsTotalWidth - padding;
        if (inputWidth < 100 * scale) {
            inputWidth = 100 * scale;  // 最小宽度限制
        }
        
        ImGui::Text("%s", loc.get("fileDialog.fileName").c_str());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        
        // 使用静态缓冲区，清空后复制当前文件名
        std::fill_n(s_nameBuf, kNameBufSize, '\0');
        std::copy(s_fileName.begin(), s_fileName.end(), s_nameBuf);
        
        if (ImGui::InputText("##FileName", s_nameBuf, kNameBufSize)) {
            s_fileName = s_nameBuf;
        }
        
        ImGui::SameLine();
        
        // 后缀选择（目前固定）
        ImGui::SetNextItemWidth(extWidth);
        if (ImGui::BeginCombo("##Extension", FILE_EXTENSION.c_str(), ImGuiComboFlags_NoArrowButton)) {
            ImGui::Selectable(FILE_EXTENSION.c_str(), true);
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        
        // 保存/打开按钮
        bool canConfirm = !s_fileName.empty();
        if (!canConfirm) {
            ImGui::BeginDisabled();
        }
        
        // 按钮右对齐
        availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - buttonsTotalWidth);
        
        // 按下保存/打开按钮、文件名不为空时按下Enter
        const std::string& confirmLabel = isOpen ? loc.get("fileDialog.button.open") : loc.get("fileDialog.button.save");
        if (ImGui::Button(confirmLabel.c_str(), ImVec2(btnWidth, 0))
            || (canConfirm && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))) {
            outFullPath = s_currentPath / PathUtils::toPath(s_fileName + FILE_EXTENSION);
            bReturned = true;
            bShowDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        if (!canConfirm) {
            ImGui::EndDisabled();
        }
        
        ImGui::SameLine();
        
        // 取消按钮、或者按下Esc键
        if (ImGui::Button(loc.get("fileDialog.button.cancel").c_str(), ImVec2(btnWidth, 0))
            || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            bReturned = false;
            bShowDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        // 显示当前要保存到的完整路径
        ImGui::Separator();
        if (!s_fileName.empty()) {
            std::filesystem::path fullPath = s_currentPath / PathUtils::toPath(s_fileName + FILE_EXTENSION);
            ImGui::Text("%s %s", loc.get("fileDialog.saveTo").c_str(), PathUtils::toString(fullPath).c_str());
        } else {
            ImGui::Text("%s %s", loc.get("fileDialog.saveTo").c_str(), loc.get("fileDialog.noFileName").c_str());
        }
        
        ImGui::EndPopup();
    }
    
    // 对话框关闭时重置状态
    if (!bShowDialog) {
        s_wasShowing = false;
    }
}

// ============================================================================
// 消息框实现
// ============================================================================

namespace {
    // 消息框静态状态
    std::string s_messageBoxTitle;      // 标题
    std::string s_messageBoxContent;    // 内容
}

void showMessageBox(bool& bShow, const std::string& message, const std::string& title) {
    auto& loc = LocalizationManager::getInstance();
    
    // 初始化消息框内容（仅在第一次显示时）
    static bool s_wasShowing = false;
    bool isFirstFrame = bShow && !s_wasShowing;
    s_wasShowing = bShow;
    
    if (isFirstFrame) {
        s_messageBoxTitle = title.empty() ? loc.get("messageBox.defaultTitle") : title;
        s_messageBoxContent = message;
    }
    
    // 设置模态对话框
    ImGui::OpenPopup(s_messageBoxTitle.c_str());
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    // 初始窗口大小（考虑缩放）
    float scale = getUIScaleFactor();
    ImGui::SetNextWindowSize(ImVec2(400 * scale, 200 * scale), ImGuiCond_FirstUseEver);
    
    // 窗口固定在中央
    if (ImGui::BeginPopupModal(s_messageBoxTitle.c_str(), &bShow, ImGuiWindowFlags_NoMove)) {
        
        // 显示消息内容
        ImGui::TextWrapped("%s", s_messageBoxContent.c_str());
        
        // 按钮布局计算
        float btnWidth = 100 * scale;
        float btnHeight = 30 * scale;
        
        // 计算按钮位置（底部居中）
        float windowWidth = ImGui::GetWindowSize().x;
        float windowHeight = ImGui::GetWindowSize().y;
        float startX = (windowWidth - btnWidth) * 0.5f;
        float btnY = windowHeight - btnHeight - ImGui::GetStyle().WindowPadding.y * 2;
        
        // 确定按钮（底部居中，Enter键或Esc键都可以关闭）
        ImGui::SetCursorPos(ImVec2(startX, btnY));
        if (ImGui::Button(loc.get("messageBox.button.ok").c_str(), ImVec2(btnWidth, btnHeight))
            || ImGui::IsKeyPressed(ImGuiKey_Enter)
            || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            bShow = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();  // 设置默认焦点
        
        ImGui::EndPopup();
    }
    
    // 对话框关闭时重置状态
    if (!bShow) {
        s_wasShowing = false;
        s_messageBoxContent.clear();
        s_messageBoxTitle.clear();
    }
}

// ============================================================================
// 三态对话框实现（是/否/取消）
// ============================================================================

namespace {
    // 三态对话框静态状态
    std::string s_triStateTitle;        // 标题
    std::string s_triStateMessage;      // 消息内容
    std::string s_triStateYesLabel;     // 是按钮标签
    std::string s_triStateNoLabel;      // 否按钮标签
    std::string s_triStateCancelLabel;  // 取消按钮标签
}

void showYesNoCancelDialog(bool& bShow, TriStateResult& result,
                          const std::string& title, const std::string& message,
                          const std::string& yesLabel, const std::string& noLabel,
                          const std::string& cancelLabel) {
    auto& loc = LocalizationManager::getInstance();
    
    // 初始化对话框内容（仅在第一次显示时）
    static bool s_wasShowing = false;
    bool isFirstFrame = bShow && !s_wasShowing;
    s_wasShowing = bShow;
    
    if (isFirstFrame) {
        s_triStateTitle = title;
        s_triStateMessage = message;
        s_triStateYesLabel = yesLabel.empty() ? loc.get("dialog.yes") : yesLabel;
        s_triStateNoLabel = noLabel.empty() ? loc.get("dialog.no") : noLabel;
        s_triStateCancelLabel = cancelLabel.empty() ? loc.get("dialog.cancel") : cancelLabel;
        result = TriStateResult::kCancel;  // 默认取消
    }
    
    // 设置模态对话框
    ImGui::OpenPopup(s_triStateTitle.c_str());
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    
    // 初始窗口大小（考虑缩放）
    float scale = getUIScaleFactor();
    ImGui::SetNextWindowSize(ImVec2(600 * scale, 400 * scale), ImGuiCond_FirstUseEver);
    
    // 窗口固定在中央
    if (ImGui::BeginPopupModal(s_triStateTitle.c_str(), &bShow, ImGuiWindowFlags_NoMove)) {
        
        // 显示消息内容
        ImGui::TextWrapped("%s", s_triStateMessage.c_str());
        
        // 按钮布局计算
        float btnWidth = 150 * scale;       // 按钮宽度
        float btnHeight = 30 * scale;       // 按钮高度
        float btnSpacing = 30 * scale;      // 按钮间隔
        float buttonsTotalWidth = btnWidth * 3 + btnSpacing * 2;
        
        // 计算按钮位置（底部居中）
        float windowWidth = ImGui::GetWindowSize().x;
        float windowHeight = ImGui::GetWindowSize().y;
        float startX = (windowWidth - buttonsTotalWidth) * 0.5f;
        float btnY = windowHeight - btnHeight - ImGui::GetStyle().WindowPadding.y * 2;
        
        // 是 按钮（设置了默认焦点，Enter键会触发有焦点的按钮）
        ImGui::SetCursorPos(ImVec2(startX, btnY));
        if (ImGui::Button(s_triStateYesLabel.c_str(), ImVec2(btnWidth, btnHeight))) {
            result = TriStateResult::kYes;
            bShow = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();  // 首次显示时设置默认焦点到是按钮
        
        // 否 按钮
        ImGui::SetCursorPos(ImVec2(startX + btnWidth + btnSpacing, btnY));
        if (ImGui::Button(s_triStateNoLabel.c_str(), ImVec2(btnWidth, btnHeight))) {
            result = TriStateResult::kNo;
            bShow = false;
            ImGui::CloseCurrentPopup();
        }
        
        // 取消 按钮（Esc键也响应）
        ImGui::SetCursorPos(ImVec2(startX + (btnWidth + btnSpacing) * 2, btnY));
        if (ImGui::Button(s_triStateCancelLabel.c_str(), ImVec2(btnWidth, btnHeight))
            || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            result = TriStateResult::kCancel;
            bShow = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // 对话框关闭时重置状态
    if (!bShow) {
        s_wasShowing = false;
    }
}

// ============================================================================
// 保存确认对话框实现
// ============================================================================

void showSaveConfirmDialog(bool& bShow, TriStateResult& result,
                          const std::string& fileName,
                          const std::string& title) {
    auto& loc = LocalizationManager::getInstance();
    
    std::string dialogTitle = title.empty() ? loc.get("saveConfirmDialog.title") : title;
    std::string message = StringUtils::format(loc.get("saveConfirmDialog.message"), fileName);
    std::string saveLabel = loc.get("saveConfirmDialog.save");
    std::string discardLabel = loc.get("saveConfirmDialog.discard");
    
    showYesNoCancelDialog(bShow, result, dialogTitle, message, saveLabel, discardLabel);
}

// ============================================================================
// 文件读写接口实现
// ============================================================================

bool readTextFile(const std::filesystem::path& filePath, std::string& outContent) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamoff fileSize = file.tellg(); // std::streampos -> std::streamoff
    file.seekg(0, std::ios::beg);

    // 检查文件大小（空文件直接返回成功）
    if (fileSize <= 0) {
        outContent.clear();
        file.close();
        return true;
    }

    // 读取内容
    outContent.resize(static_cast<size_t>(fileSize));
    file.read(outContent.data(), static_cast<std::streamsize>(fileSize));

    bool success = file.good();
    file.close();
    return success;
}

bool writeTextFile(const std::filesystem::path& filePath, const std::string& content) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(content.data(), content.size());
    bool success = file.good();
    file.close();
    return success;
}

} // namespace Utils
} // namespace tch
