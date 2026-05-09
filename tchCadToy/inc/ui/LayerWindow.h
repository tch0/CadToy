#pragma once

// C++ 标准库
#include <string>

// 第三方库

// 项目头文件
#include "DbCommon.h"

namespace tch {

// 前置声明
class Database;
class DbLayer;

// ================================================================================================
// 图层窗口类（单例）
// 非模态窗口，显示所有图层信息并支持编辑
// ================================================================================================
class LayerWindow {
public:
    // 获取单例实例
    static LayerWindow& getInstance();

    // 禁止拷贝
    LayerWindow(const LayerWindow&) = delete;
    LayerWindow& operator=(const LayerWindow&) = delete;

    // ================================================================================================
    // 窗口控制
    // ================================================================================================

    // 显示窗口
    void show();

    // 隐藏窗口
    void hide();

    // 切换显示/隐藏
    void toggle();

    // ================================================================================================
    // 绘制窗口（由 Renderer::drawNonModalWindows 调用）
    // ================================================================================================

    void draw();

    // ================================================================================================
    // 查询状态
    // ================================================================================================

    bool isVisible() const { return m_visible; }

private:
    // 私有构造函数
    LayerWindow();
    ~LayerWindow() = default;

    // 绘制图层表格
    void drawLayerTable();

    // 绘制单行
    void drawLayerRow(ObjectId layerId, bool isCurrent, Database* pDb);

    // 绘制状态列
    void drawStatusColumn(ObjectId layerId, bool isCurrent);

    // 绘制名称列（支持编辑）
    void drawNameColumn(DbLayer* pLayer, ObjectId layerId);

    // 绘制冻结列
    void drawFrozenColumn(DbLayer* pLayer);

    // 绘制锁定列
    void drawLockedColumn(DbLayer* pLayer);

    // 绘制颜色列
    void drawColorColumn(DbLayer* pLayer);

    // 绘制线型列
    void drawLinetypeColumn(DbLayer* pLayer);

    // 绘制线宽列
    void drawLineWeightColumn(DbLayer* pLayer);

    // 绘制透明度列
    void drawTransparencyColumn(DbLayer* pLayer);

    // 绘制说明列（支持编辑）
    void drawDescriptionColumn(DbLayer* pLayer, ObjectId layerId);

    // 获取线宽显示字符串
    std::string getLineWeightString(DbLineWeight lw) const;

    // 提交图层名称（检查冲突，冲突时弹窗并拒绝）
    void commitLayerName(DbLayer* pLayer);

    // ================================================================================================
    // 成员变量
    // ================================================================================================

    bool m_visible = false;

    // 选中行
    ObjectId m_selectedLayerId = 0;

    // 编辑状态 - 名称
    ObjectId m_editingNameId = 0;
    char m_nameEditBuffer[256] = {};
    bool m_nameEditActive = false;
    std::string m_nameEditOriginal;

    // 编辑状态 - 说明
    ObjectId m_editingDescId = 0;
    char m_descEditBuffer[256] = {};
    bool m_descEditActive = false;
    std::string m_descEditOriginal;

    // 消息框标题（动态构造：窗口标题 + 操作名）
    std::string m_layerMessageBoxTitle;

    // 删除错误消息框
    bool m_showDeleteError = false;
    std::string m_layerDeleteErrorMsg;

    // 重命名错误消息框
    bool m_showRenameError = false;
    std::string m_layerRenameErrorMsg;
};

} // namespace tch
