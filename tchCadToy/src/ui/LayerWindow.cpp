// 对应头文件
#include "LayerWindow.h"

// C++ 标准库
#include <cstring>
#include <format>

// 第三方库
#include <imgui.h>

// 项目头文件
#include "Database.h"
#include "DbLayer.h"
#include "DocManager.h"
#include "Document.h"
#include "IconManager.h"
#include "IconDefines.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"
#include "StringUtils.h"

namespace tch {

// ================================================================================================
// 单例实现
// ================================================================================================

LayerWindow& LayerWindow::getInstance() {
    static LayerWindow instance;
    return instance;
}

LayerWindow::LayerWindow() {
    // 初始化编辑缓冲区
    m_nameEditBuffer[0] = '\0';
    m_descEditBuffer[0] = '\0';
}

// ================================================================================================
// 窗口控制
// ================================================================================================

void LayerWindow::show() {
    m_visible = true;
}

void LayerWindow::hide() {
    m_visible = false;
}

void LayerWindow::toggle() {
    m_visible = !m_visible;
}

// ================================================================================================
// 绘制窗口
// ================================================================================================

void LayerWindow::draw() {
    if (!m_visible) {
        return;
    }

    auto& loc = LocalizationManager::getInstance();

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;

    if (ImGui::Begin(loc.get("window.layer.title").c_str(), &m_visible, flags)) { // 图层管理器
        // 悬停时如果还没有获得焦点则自动获得焦点，以方便选中检测等行为，让交互变得更丝滑，仅鼠标进入时设置一次，避免每帧都设置（会导致内部窗口内部弹出的菜单失焦）
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)
            && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            ImGui::SetWindowFocus();
        }
        drawLayerTable();
    }
    ImGui::End();
    
    // 图层窗口中可能弹出的所有模态对话框，注意不能再ImGui::Begin/End之间显示模态对话框，会扰乱ImGui的窗口堆栈管理，导致闪烁
    // 删除错误消息框
    if (m_showDeleteError) {
        Utils::showMessageBox(m_showDeleteError, m_layerDeleteErrorMsg, m_layerMessageBoxTitle);
    }

    // 重命名错误消息框
    if (m_showRenameError) {
        Utils::showMessageBox(m_showRenameError, m_layerRenameErrorMsg, m_layerMessageBoxTitle);
    }
}

void LayerWindow::drawLayerTable() {
    auto& loc = LocalizationManager::getInstance();
    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersV |
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_RowBg;

    // 新建 / 置为当前 / 删除 图层按钮
    ImGui::Spacing();
    if (ImGui::Button(loc.get("window.layer.operations.new").c_str(), ImVec2(0, 0))) { // 新建图层
        Database* pDb = DocManager::getCurrentDocument().getDatabase();
        if (pDb) {
            std::string name;
            int counter = 1;
            do {
                name = "图层" + std::to_string(counter++);
            } while (pDb->layerExists(name));

            ObjectId newId = pDb->addLayer(name);
            if (newId != 0) {
                m_selectedLayerId = newId;
                // 创建新图层成功后，进入名称编辑状态
                m_editingNameId = m_selectedLayerId;
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(loc.get("window.layer.operations.setCurrent").c_str(), ImVec2(0, 0))) { // 置为当前
        if (m_selectedLayerId != 0) {
            Database* pDb = DocManager::getCurrentDocument().getDatabase();
            if (pDb) {
                pDb->setCurrentLayerId(m_selectedLayerId);
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(loc.get("window.layer.operations.delete").c_str(), ImVec2(0, 0))) { // 删除图层
        if (m_selectedLayerId != 0) {
            Database* pDb = DocManager::getCurrentDocument().getDatabase();
            DbLayer* pLayer = pDb ? pDb->getLayer(m_selectedLayerId) : nullptr;
            if (pLayer) {
                ObjectId currentLayerId = pDb->currentLayer() ? pDb->currentLayer()->id() : 0;

                // 构造消息框标题：图层管理器 - 删除图层
                m_layerMessageBoxTitle = StringUtils::format(
                    "{} - {}",
                    loc.get("window.layer.title"),      // 图层管理器
                    loc.get("window.layer.operations.delete")); // 删除图层

                if (pLayer->name() == "0") {
                    m_layerDeleteErrorMsg = loc.get("window.layer.prompts.cannotDeleteLayer0"); // 无法删除图层0。
                    m_showDeleteError = true;
                } else if (m_selectedLayerId == currentLayerId) {
                    m_layerDeleteErrorMsg = loc.get("window.layer.prompts.cannotDeleteCurrent"); // 无法删除当前图层。
                    m_showDeleteError = true;
                } else if (!pDb->getEntitiesOnLayer(m_selectedLayerId).empty()) {
                    m_layerDeleteErrorMsg = StringUtils::format(
                        loc.get("window.layer.prompts.cannotDeleteHasEntities"), // 图层 "{}" 上有实体，不能删除。
                        pLayer->name());
                    m_showDeleteError = true;
                } else {
                    // 查找上一个图层作为删除后的选中项
                    const auto& ids = pDb->layerIds();
                    ObjectId prevLayerId = currentLayerId;
                    for (size_t i = 0; i < ids.size(); i++) {
                        if (ids[i] == m_selectedLayerId) {
                            if (i > 0) {
                                prevLayerId = ids[i - 1];
                            } else if (i + 1 < ids.size()) {
                                prevLayerId = ids[i + 1];
                            }
                            break;
                        }
                    }
                    pDb->removeLayer(m_selectedLayerId);
                    m_selectedLayerId = prevLayerId;
                }
            }
        }
    }

    if (ImGui::BeginTable("layer_table", 9, tableFlags)) {
        // 列定义（由 ImGui 自动管理宽度，列宽信息跨帧缓存）
        ImGui::TableSetupColumn(loc.get("window.layer.columns.status").c_str());        // 状态
        ImGui::TableSetupColumn(loc.get("window.layer.columns.name").c_str());          // 名称
        ImGui::TableSetupColumn(loc.get("window.layer.columns.frozen").c_str());        // 冻结
        ImGui::TableSetupColumn(loc.get("window.layer.columns.locked").c_str());        // 锁定
        ImGui::TableSetupColumn(loc.get("window.layer.columns.color").c_str());         // 颜色
        ImGui::TableSetupColumn(loc.get("window.layer.columns.linetype").c_str());      // 线型
        ImGui::TableSetupColumn(loc.get("window.layer.columns.lineweight").c_str());    // 线宽
        ImGui::TableSetupColumn(loc.get("window.layer.columns.transparency").c_str());  // 透明度
        ImGui::TableSetupColumn(loc.get("window.layer.columns.description").c_str());   // 说明

        // 表头
        ImGui::TableHeadersRow();

        // 获取数据库
        Document& doc = DocManager::getCurrentDocument();
        Database* pDb = doc.getDatabase();
        if (!pDb) {
            ImGui::EndTable();
            return;
        }

        // 获取当前图层ID
        DbLayer* pCurrentLayer = pDb->currentLayer();
        ObjectId currentLayerId = pCurrentLayer ? pCurrentLayer->id() : 0;

        // 选中初始化：窗口打开或选中图层被删除或文档切换导致选中图层ID失效时，默认选中当前图层
        if (m_selectedLayerId == 0 || pDb->getLayer(m_selectedLayerId) == nullptr) {
            if (pCurrentLayer) {
                m_selectedLayerId = currentLayerId;
            }
        }

        // 绘制所有图层行
        for (ObjectId layerId : pDb->layerIds()) {
            drawLayerRow(layerId, layerId == currentLayerId, pDb);
        }

        ImGui::EndTable();
    }
}

void LayerWindow::drawLayerRow(ObjectId layerId, bool isCurrent, Database* pDb) {
    DbLayer* pLayer = pDb->getLayer(layerId);
    if (!pLayer) {
        return;
    }

    ImGui::TableNextRow();

    // 选中行高亮（Table 原生支持，跨列自动生效）
    if (m_selectedLayerId == layerId) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(0x74, 0x87, 0xa5, 0x9f));
    }

    // 状态列（进入第一列后 cursor.x 即为表格左边缘）
    ImGui::TableNextColumn();
    ImVec2 rowStartPos = ImGui::GetCursorScreenPos();
    float rowWidth = (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x) - rowStartPos.x;

    drawStatusColumn(layerId, isCurrent);

    // 名称列
    ImGui::TableNextColumn();
    drawNameColumn(pLayer, layerId);

    // 冻结列
    ImGui::TableNextColumn();
    drawFrozenColumn(pLayer);

    // 锁定列
    ImGui::TableNextColumn();
    drawLockedColumn(pLayer);

    // 颜色列
    ImGui::TableNextColumn();
    drawColorColumn(pLayer);

    // 线型列
    ImGui::TableNextColumn();
    drawLinetypeColumn(pLayer);

    // 线宽列
    ImGui::TableNextColumn();
    drawLineWeightColumn(pLayer);

    // 透明度列
    ImGui::TableNextColumn();
    drawTransparencyColumn(pLayer);

    // 说明列
    ImGui::TableNextColumn();
    drawDescriptionColumn(pLayer, layerId);

    // 整行点击选中（在 9 列绘制完毕、光标位于下一行后，计算本行矩形区域）
    ImVec2 nextRowPos = ImGui::GetCursorScreenPos();
    float rowHeight = nextRowPos.y - rowStartPos.y;

    if (rowHeight > 0.0f) {
        float rowEndX = rowStartPos.x + rowWidth;

        ImVec2 rowMin(rowStartPos.x, rowStartPos.y);
        ImVec2 rowMax(rowEndX, rowStartPos.y + rowHeight);
        // 这里第三个参数必须传入false，不对传入矩形进行剪切，默认true会根据imgui内部设置进行剪切之后才判断悬停，会剪切到最后一列的位置，则只有点击最后一列才能切换
        if (ImGui::IsMouseHoveringRect(rowMin, rowMax, false)) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                // 因为通过悬停和鼠标获取不会考虑窗口焦点，在有上层重叠窗口和模态对话框打开时也会响应，所以需要附带检测窗口焦点
                if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                    m_selectedLayerId = layerId;
                }
            }
        }
    }
}

void LayerWindow::drawStatusColumn(ObjectId layerId, bool isCurrent) {
    IconManager& iconMgr = IconManager::getInstance();
    ImTextureID icon = isCurrent ?
        iconMgr.getIcon(IconID::kLayerCurrent) :
        iconMgr.getIcon(IconID::kLayerOther);

    // 计算整格区域用于双击检测
    float cellWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 cellMin = ImGui::GetCursorScreenPos();

    if (icon) {
        float iconSize = ImGui::GetTextLineHeight();
        ImVec2 size(iconSize, iconSize);
        ImGui::Image(icon, size);
    }

    ImVec2 cellMax(cellMin.x + cellWidth, ImGui::GetCursorScreenPos().y);

    // 双击整格：切换当前图层
    bool hoveredFullCell = ImGui::IsMouseHoveringRect(cellMin, cellMax);
    if (hoveredFullCell && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        Database* pDb = DocManager::getCurrentDocument().getDatabase();
        if (pDb) {
            pDb->setCurrentLayerId(layerId);
        }
    }
}

void LayerWindow::drawNameColumn(DbLayer* pLayer, ObjectId layerId) {
    // 检查是否正在编辑
    if (m_editingNameId == layerId) {
        // 显示输入框
        ImGui::PushID(static_cast<int>(layerId));

        // 如果刚刚进入编辑状态，备份原始值并填充缓冲区
        if (!m_nameEditActive) {
            m_nameEditOriginal = pLayer->name();
            const std::string& name = m_nameEditOriginal;
            size_t copyLen = std::min(name.size(), sizeof(m_nameEditBuffer) - 1);
            std::copy(name.begin(), name.begin() + copyLen, m_nameEditBuffer);
            m_nameEditBuffer[copyLen] = '\0';
            m_nameEditActive = true;
        }

        // 在 InputText 之前检测 Esc（此时 InputText 尚未消费该按键）
        bool cancelByEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);

        ImGui::SetKeyboardFocusHere();
        // 输入框（EscapeClearsAll: Esc 清除缓冲区；EnterReturnsTrue: Enter 提交）
        if (ImGui::InputText("##name_edit", m_nameEditBuffer, sizeof(m_nameEditBuffer),
                             ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
            // 按回车提交，检查名称冲突
            commitLayerName(pLayer);
            m_editingNameId = 0;
            m_nameEditActive = false;
        }

        // Esc 取消，清除缓冲区，结束编辑
        if (cancelByEscape) {
            m_nameEditBuffer[0] = '\0';
            m_editingNameId = 0;
            m_nameEditActive = false;
        }

        // 失去焦点且未被 Esc 取消 → 自动提交，检查名称冲突
        if (m_editingNameId == layerId && ImGui::IsItemDeactivated()) {
            commitLayerName(pLayer);
            m_editingNameId = 0;
            m_nameEditActive = false;
        }

        ImGui::PopID();
    } else {
        // 显示文本，点击区域扩展到整个单元格
        float cellWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 cellMin = ImGui::GetCursorScreenPos();

        ImGui::Text("%s", pLayer->name().c_str());
        ImVec2 cellMax(cellMin.x + cellWidth, ImGui::GetCursorScreenPos().y);

        // 检测单击和双击（整格响应，选中由行级统一处理）
        bool hoveredFullCell = ImGui::IsMouseHoveringRect(cellMin, cellMax);
        if (hoveredFullCell) {
            bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
            bool singleClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !doubleClicked;

            if (doubleClicked) {
                // 双击：切换当前图层
                Database* pDb = DocManager::getCurrentDocument().getDatabase();
                if (pDb) {
                    pDb->setCurrentLayerId(layerId);
                }
            } else if (singleClicked) {
                // 单击：已选中则进入编辑（选中由行级处理）
                if (m_selectedLayerId == layerId) {
                    // 图层0不能重命名
                    if (pLayer->name() == "0") {
                        auto& loc = LocalizationManager::getInstance();
                        m_layerMessageBoxTitle = StringUtils::format(
                            "{} - {}",
                            loc.get("window.layer.title"),              // 图层管理器
                            loc.get("window.layer.operations.rename")); // 重命名
                        m_layerRenameErrorMsg = loc.get("window.layer.prompts.cannotRenameLayer0"); // 图层0不能重命名。
                        m_showRenameError = true;
                    } else {
                        m_editingNameId = layerId;
                        m_nameEditActive = false;
                    }
                }
            }
        }
    }
}

void LayerWindow::drawFrozenColumn(DbLayer* pLayer) {
    IconManager& iconMgr = IconManager::getInstance();
    bool frozen = pLayer->isFrozen();

    ImTextureID icon = frozen ?
        iconMgr.getIcon(IconID::kLayerFrozen) :
        iconMgr.getIcon(IconID::kLayerUnFrozen);

    if (icon) {
        float iconSize = ImGui::GetTextLineHeight();
        ImVec2 size(iconSize, iconSize);
        ImGui::PushID(static_cast<int>(pLayer->id()));
        if (ImGui::ImageButton("##frozen", icon, size)) {
            pLayer->setFrozen(!frozen);
        }
        ImGui::PopID();
    }
}

void LayerWindow::drawLockedColumn(DbLayer* pLayer) {
    IconManager& iconMgr = IconManager::getInstance();
    bool locked = pLayer->isLocked();

    ImTextureID icon = locked ?
        iconMgr.getIcon(IconID::kLayerLocked) :
        iconMgr.getIcon(IconID::kLayerUnlocked);

    if (icon) {
        float iconSize = ImGui::GetTextLineHeight();
        ImVec2 size(iconSize, iconSize);
        ImGui::PushID(static_cast<int>(pLayer->id()));
        if (ImGui::ImageButton("##locked", icon, size)) {
            pLayer->setLocked(!locked);
        }
        ImGui::PopID();
    }
}

void LayerWindow::drawColorColumn(DbLayer* pLayer) {
    const DbColor& color = pLayer->color();

    // 绘制颜色方块
    ImVec4 imColor;
    if (color.type() == DbColor::kRGB) {
        uint32_t rgb = color.rgb();
        imColor.x = ((rgb >> 16) & 0xFF) / 255.0f;
        imColor.y = ((rgb >> 8) & 0xFF) / 255.0f;
        imColor.z = (rgb & 0xFF) / 255.0f;
        imColor.w = 1.0f;
    } else if (color.isByLayer()) {
        imColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 图层 ByLayer 视为默认白色
    } else {
        imColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 白色
    }

    float iconSize = ImGui::GetTextLineHeight();
    ImVec2 size(iconSize, iconSize);

    ImGui::PushID(static_cast<int>(pLayer->id()));
    if (ImGui::ColorButton("##color", imColor, 0, size)) {
        // TODO: 打开颜色对话框
    }
    ImGui::PopID();

    ImGui::SameLine();

    // 显示颜色值
    if (color.type() == DbColor::kRGB) {
        ImGui::Text("#%06X", color.rgb() & 0xFFFFFF);
    } else if (color.isByLayer()) {
        ImGui::Text("ByLayer");
    } else if (color.isByBlock()) {
        ImGui::Text("ByBlock");
    }
}

void LayerWindow::drawLinetypeColumn(DbLayer* pLayer) {
    const DbLinetypeRef& linetype = pLayer->linetype();

    std::string linetypeName;
    if (linetype.isContinuous()) {
        linetypeName = "Continuous";
    } else if (linetype.isByLayer()) {
        linetypeName = "ByLayer";
    } else if (linetype.isByBlock()) {
        linetypeName = "ByBlock";
    } else {
        linetypeName = "Other";
    }

    ImGui::PushID(static_cast<int>(pLayer->id()));
    if (ImGui::Selectable(linetypeName.c_str(), false, ImGuiSelectableFlags_None)) {
        // TODO: 打开线型对话框
    }
    ImGui::PopID();
}

void LayerWindow::drawLineWeightColumn(DbLayer* pLayer) {
    // 绘制线宽预览线段
    float uiScale = Utils::getUIScaleFactor();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float lineHeight = 2.0f;

    // 保留原始 lw 用于字符串显示（忠实反映数据）
    DbLineWeight lw = pLayer->lineWeight();

    // 解析预览线宽（kByLwDefault 时获取实际默认值用于绘制预览）
    DbLineWeight previewLw = lw;
    if (previewLw == DbLineWeight::kByLwDefault) {
        Database* pDb = DocManager::getCurrentDocument().getDatabase();
        if (pDb) {
            previewLw = pDb->defaultLineWeight();
        }
        if (previewLw == DbLineWeight::kByLwDefault) {
            previewLw = DbLineWeight::k000;
        }
    }
    lineHeight = static_cast<float>(previewLw) * 0.04f;
    if (lineHeight < 1.0f) { lineHeight = 1.0f; }
    if (lineHeight > 10.0f) { lineHeight = 10.0f; }

    // 绘制线段（线宽不随 UI 缩放，线段长度随 UI 缩放）
    ImVec2 p1(pos.x, pos.y + 8.0f * uiScale);
    ImVec2 p2(pos.x + 30.0f * uiScale, pos.y + 8.0f * uiScale);
    drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), lineHeight);

    ImGui::Dummy(ImVec2(35.0f * uiScale, 16.0f * uiScale));
    ImGui::SameLine();

    // 用原始 lw 显示字符串，忠实反映数据
    std::string lwStr = getLineWeightString(lw);
    ImGui::PushID(static_cast<int>(pLayer->id()));
    if (ImGui::Selectable(lwStr.c_str(), false, ImGuiSelectableFlags_None)) {
        // TODO: 打开线宽对话框
    }
    ImGui::PopID();
}

void LayerWindow::drawTransparencyColumn(DbLayer* pLayer) {
    float transparency = pLayer->transparency();
    ImGui::Text("%.2f", transparency);
}

void LayerWindow::drawDescriptionColumn(DbLayer* pLayer, ObjectId layerId) {
    // 检查是否正在编辑
    if (m_editingDescId == layerId) {
        // 显示输入框
        ImGui::PushID(static_cast<int>(layerId) + 10000); // 避免与名称编辑ID冲突

        // 如果刚刚进入编辑状态，备份原始值并填充缓冲区
        if (!m_descEditActive) {
            m_descEditOriginal = pLayer->description();
            const std::string& desc = m_descEditOriginal;
            size_t copyLen = std::min(desc.size(), sizeof(m_descEditBuffer) - 1);
            std::copy(desc.begin(), desc.begin() + copyLen, m_descEditBuffer);
            m_descEditBuffer[copyLen] = '\0';
            m_descEditActive = true;
        }

        // 在 InputText 之前检测 Esc
        bool cancelByEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);

        ImGui::SetKeyboardFocusHere();
        // 输入框
        if (ImGui::InputText("##desc_edit", m_descEditBuffer, sizeof(m_descEditBuffer),
                             ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_EnterReturnsTrue)) {
            // 按回车提交
            pLayer->setDescription(m_descEditBuffer);
            m_editingDescId = 0;
            m_descEditActive = false;
        }

        // Esc 取消，清除缓冲区，结束编辑
        if (cancelByEscape) {
            m_descEditBuffer[0] = '\0';
            m_editingDescId = 0;
            m_descEditActive = false;
        }

        // 失去焦点且未被 Esc 取消 → 自动提交
        if (m_editingDescId == layerId && ImGui::IsItemDeactivated()) {
            pLayer->setDescription(m_descEditBuffer);
            m_editingDescId = 0;
            m_descEditActive = false;
        }

        ImGui::PopID();
    } else {
        // 显示文本，点击区域扩展到整个单元格
        float cellWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 cellMin = ImGui::GetCursorScreenPos();

        const std::string& desc = pLayer->description();
        if (desc.empty()) {
            ImGui::Text(" ");
        } else {
            ImGui::Text("%s", desc.c_str());
        }
        ImVec2 cellMax(cellMin.x + cellWidth, ImGui::GetCursorScreenPos().y);

        // 检测单击（整格响应，选中由行级统一处理，仅支持编辑）
        bool hoveredFullCell = ImGui::IsMouseHoveringRect(cellMin, cellMax);
        if (hoveredFullCell) {
            bool singleClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            if (singleClicked) {
                // 单击：已选中则进入编辑（选中由行级处理）
                if (m_selectedLayerId == layerId) {
                    m_editingDescId = layerId;
                    m_descEditActive = false;
                }
            }
        }
    }
}

void LayerWindow::commitLayerName(DbLayer* pLayer) {
    std::string newName(m_nameEditBuffer);
    if (newName.empty()) {
        return;
    }

    if (newName != m_nameEditOriginal) {
        Database* pDb = DocManager::getCurrentDocument().getDatabase();
        if (pDb && pDb->layerExists(newName)) {
            auto& loc = LocalizationManager::getInstance();
            m_layerMessageBoxTitle = StringUtils::format(
                "{} - {}",
                loc.get("window.layer.title"),              // 图层管理器
                loc.get("window.layer.operations.rename")); // 重命名
            m_layerRenameErrorMsg = StringUtils::format(
                loc.get("window.layer.prompts.nameInUse"), // 图层名称"{}"已经在使用，请输入其他名称。
                newName);
            m_showRenameError = true;
            return;
        }
    }
    pLayer->setName(newName);
}

std::string LayerWindow::getLineWeightString(DbLineWeight lw) const {
    if (lw == DbLineWeight::kByLwDefault) {
        return LocalizationManager::getInstance().get("window.layer.defaultLineWeight"); // 默认
    } else if (lw == DbLineWeight::kByLayer) {
        return "ByLayer";
    } else if (lw == DbLineWeight::kByBlock) {
        return "ByBlock";
    } else {
        // 转换为 mm 显示
        float mm = static_cast<float>(lw) / 100.0f;
        return std::format("{:.2f}mm", mm);
    }
}

} // namespace tch
