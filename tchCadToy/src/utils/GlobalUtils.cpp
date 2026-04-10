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
#include "PlatformUtils.h"
#include "LocalizationManager.h"


namespace tch {
namespace Utils {

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
    // 静态变量保存对话框状态（全部使用 PlatformUtils::Path，内部 UTF-8）
    PlatformUtils::Path s_lastPath;    // 上次使用的路径
    PlatformUtils::Path s_currentPath; // 当前浏览路径
    std::string s_fileName;            // 当前输入的文件名（UTF-8）
    std::vector<std::string> s_dirs;   // 当前目录的子目录名（UTF-8）
    std::vector<std::string> s_files;  // 当前目录的 .cad.json 文件名（UTF-8，不含后缀）
    
    const std::string FILE_EXTENSION = ".cad.json";
    
    constexpr size_t kNameBufSize = 256;
    char s_nameBuf[kNameBufSize] = {}; // 静态缓冲区，避免频繁分配栈空间
    
    // 刷新当前目录的文件列表
    void refreshFileList() {
        s_dirs.clear();
        s_files.clear();
        
        try {
            // 使用 Path 的 iterate 方法遍历目录
            s_currentPath.iterate([&](const PlatformUtils::Path& entryPath) {
                if (entryPath.isDirectory()) {
                    s_dirs.push_back(entryPath.filename());
                } else if (entryPath.isRegularFile()) {
                    std::string filename = entryPath.filename();
                    // 检查是否是 .cad.json
                    if (filename.size() >= FILE_EXTENSION.size() &&
                        filename.substr(filename.size() - FILE_EXTENSION.size()) == FILE_EXTENSION) {
                        // 存储不带后缀的文件名
                        s_files.push_back(filename.substr(0, filename.size() - FILE_EXTENSION.size()));
                    }
                }
            });
            
            std::sort(s_dirs.begin(), s_dirs.end());
            std::sort(s_files.begin(), s_files.end());
        } catch (...) {
            // 目录访问失败，忽略
        }
    }
    
    // 分解路径为各级目录（输入输出都是 Path）
    void splitPath(const PlatformUtils::Path& path, std::vector<PlatformUtils::Path>& parts) {
        parts.clear();
        PlatformUtils::Path current = path;
        while (!current.empty()) {
            // 如果当前路径 filename 为空（末尾有分隔符），跳过添加
            // 这种情况发生在传入的路径以分隔符结尾时
            if (!current.filename().empty()) {
                parts.push_back(current);
            }
            PlatformUtils::Path parent = current.parent();
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

void showFileDialog(bool& bShowDialog, bool& bReturned, std::string& outFullPath,
                   bool isOpen, const std::string& initialPath, const std::string& title) {
    
    auto& loc = LocalizationManager::getInstance();
    
    // 初始化路径
    if (s_currentPath.empty()) {
        if (!initialPath.empty()) {
            s_currentPath = PlatformUtils::Path(initialPath);
        } else if (!s_lastPath.empty()) {
            s_currentPath = s_lastPath;
        } else {
            // 使用可执行文件所在目录
            s_currentPath = PlatformUtils::Path(g_pathCwd);
        }
        s_fileName.clear();
        refreshFileList();
    }
    
    std::string windowTitle = title.empty() ? (isOpen ? loc.get("fileDialog.title.open") : loc.get("fileDialog.title.save")) : title;
    
    // 设置模态对话框
    ImGui::OpenPopup(windowTitle.c_str());
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal(windowTitle.c_str(), &bShowDialog,
                               ImGuiWindowFlags_NoMove)) {
        
        // ========== 顶部：路径导航 ==========
        std::vector<PlatformUtils::Path> pathParts;
        splitPath(s_currentPath, pathParts);
        
        std::string comboLabel = "##PathTree";
        std::string comboPreview = s_currentPath.string();  // UTF-8
        
        if (ImGui::BeginCombo(comboLabel.c_str(), comboPreview.c_str())) {
            // 倒序显示（从根到当前）
            int indent = 0;
            for (auto it = pathParts.rbegin(); it != pathParts.rend(); ++it, ++indent) {
                // 对于根目录，filename() 返回空，使用完整路径字符串
                std::string name = it->filename();
                if (name.empty()) {
                    name = it->string();
                }
                // 确保 display 不为空（添加空格防止 ImGui 断言失败）
                std::string display = std::string(indent * 2, ' ') + name;
                if (display.empty() || display.find_first_not_of(' ') == std::string::npos) {
                    display = " ";
                }
                if (ImGui::Selectable(display.c_str())) {
                    s_currentPath = *it;
                    s_fileName.clear();
                    refreshFileList();
                }
            }
            ImGui::EndCombo();
        }
        
        // ImGui::Separator();
        
        // ========== 中间：文件和目录列表 ==========
        ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true);
        
        // 父目录 ".."
        PlatformUtils::Path parentPath = s_currentPath.parent();
        if (!parentPath.empty() && parentPath.string() != s_currentPath.string()) {
            if (ImGui::Selectable("[..]##parent", false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    s_currentPath = parentPath;
                    s_fileName.clear();
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
                    s_currentPath = s_currentPath / dir;
                    s_fileName.clear();
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
            // 双击文件确认（打开或者保存）
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                s_fileName = file;
                PlatformUtils::Path fullPath = s_currentPath / (s_fileName + FILE_EXTENSION);
                outFullPath = fullPath.string();  // 返回 UTF-8
                s_lastPath = s_currentPath;
                bReturned = true;
                bShowDialog = false;
                s_currentPath = PlatformUtils::Path();
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
            PlatformUtils::Path fullPath = s_currentPath / (s_fileName + FILE_EXTENSION);
            outFullPath = fullPath.string();  // 返回UTF-8路径
            s_lastPath = s_currentPath;
            bReturned = true;
            bShowDialog = false;
            s_currentPath = PlatformUtils::Path();
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
            s_currentPath = PlatformUtils::Path();
            ImGui::CloseCurrentPopup();
        }
        
        // 显示当前要保存到的完整路径
        ImGui::Separator();
        if (!s_fileName.empty()) {
            PlatformUtils::Path fullPath = s_currentPath / (s_fileName + FILE_EXTENSION);
            ImGui::Text("%s %s", loc.get("fileDialog.saveTo").c_str(), fullPath.string().c_str());
        } else {
            ImGui::Text("%s %s", loc.get("fileDialog.saveTo").c_str(), loc.get("fileDialog.noFileName").c_str());
        }
        
        ImGui::EndPopup();
    }
    
    // 对话框关闭时重置状态
    if (!bShowDialog && !s_currentPath.empty()) {
        s_currentPath = PlatformUtils::Path();
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
    if (bShow && s_messageBoxContent.empty()) {
        s_messageBoxContent = message;
        s_messageBoxTitle = title.empty() ? loc.get("messageBox.defaultTitle") : title;
    }
    
    // 设置模态对话框
    ImGui::OpenPopup(s_messageBoxTitle.c_str());
    
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal(s_messageBoxTitle.c_str(), &bShow,
                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        
        // 显示消息内容
        ImGui::TextWrapped("%s", s_messageBoxContent.c_str());
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // 确定按钮（居中）
        float scale = getUIScaleFactor();
        float btnWidth = 80 * scale;
        float windowWidth = ImGui::GetWindowSize().x;
        ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
        
        if (ImGui::Button(loc.get("messageBox.ok").c_str(), ImVec2(btnWidth, 0))
            || ImGui::IsKeyPressed(ImGuiKey_Enter)
            || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)
            || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            bShow = false;
            s_messageBoxContent.clear();
            s_messageBoxTitle.clear();
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    // 对话框关闭时重置状态
    if (!bShow) {
        s_messageBoxContent.clear();
        s_messageBoxTitle.clear();
    }
}

// ============================================================================
// 文件读写封装接口
// ============================================================================

bool readTextFile(const std::string& filePathUtf8, std::string& outContent) {
    outContent.clear();
    
    if (filePathUtf8.empty()) {
        return false;
    }
    
    try {
        // 将 UTF-8 路径转换为本地编码用于文件操作
        std::string filePathLocal = PlatformUtils::utf8ToLocal(filePathUtf8);
        std::ifstream file(filePathLocal, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // 读取文件内容
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (size > 0) {
            outContent.resize(static_cast<size_t>(size));
            file.read(outContent.data(), size);
        }
        
        file.close();
        return true;
    } catch (...) {
        outContent.clear();
        return false;
    }
}

bool writeTextFile(const std::string& filePathUtf8, const std::string& content) {
    if (filePathUtf8.empty()) {
        return false;
    }
    
    try {
        // 将 UTF-8 路径转换为本地编码用于文件操作
        std::string filePathLocal = PlatformUtils::utf8ToLocal(filePathUtf8);
        std::ofstream file(filePathLocal, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        // 写入文件内容
        file.write(content.data(), content.size());
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace Utils
} // namespace tch
