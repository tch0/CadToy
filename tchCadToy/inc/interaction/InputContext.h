#pragma once

// C++ 标准库
#include <string>
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "CommonTypes.h"
#include "SelectionTask.h"
#include "SelectionSet.h"

namespace tch {


// 允许输入类型枚举
enum class InputType {
    kInteger,           // 整数输入
    kFloat,             // 浮点数输入
    kString,            // 字符串输入
    kKeyword,           // 关键字输入
    kPoint,             // 点坐标输入
    kEntitySelection    // 实体选择输入
};

// 特殊按键事件类型枚举
enum class SpecialKeyEventType {
    kNone,              // 无事件
    kEnterPressed,      // 回车键按下
    kSpacePressed,      // 空格键按下
    kEscPressed,        // Esc键按下
    kUpPressed,         // Up键按下，用于命令执行历史功能
    kDownPressed        // Down键按下，用于命令执行历史功能
};

// 输入上下文类，用于命令状态机模式的输入管理
class InputContext {
public:
    // 构造函数
    InputContext();
    
private:
    // 命令执行状态
    bool m_bInCommandExecution;
    
    // 当前输入状态
    InputStatus m_currentStatus;
    
    // 允许的输入类型列表
    std::vector<InputType> m_allowedTypes;
    
    // 提示信息
    std::string m_prompt;
    
    // 错误提示信息
    std::string m_errorPrompt;
    
    // 点输入相关
    glm::dvec3 m_pickedPoint; // 拾取获取输入的点
    bool m_bHasBasePoint; // 是否有基点
    glm::dvec3 m_basePoint; // 点输入的基点，由带基点版本waitForPoint传入
    bool m_bDrawRubberBand; // 是否绘制橡皮线
    
    // 输入的整数
    int m_inputInteger;
    int m_intLimitMin;
    int m_intLimitMax;
    
    // 浮点数输入相关
    double m_inputFloat;
    double m_floatLimitMin;
    double m_floatLimitMax;
    
    // 输入的字符串
    std::string m_inputString;
    
    // 关键字相关
    std::string m_inputKeyword; // 输入的关键字
    std::vector<std::string> m_keywordOptions; // 关键字选项列表
    
    // 选择结果（命令中调用选择的选择结果，移交给命令层）
    SelectionSet m_selectionResult;
    
    // 先选选择集（没有命令活跃时的选择集，由输入上下文亲自管理）
    SelectionSet m_priorSelectionSet;
    
    // 最后一次特殊按键事件
    SpecialKeyEventType m_lastSpecialKeyEvent;
    
    // 输入上下文信息窗口相关
    bool m_inputContextInfoVisible;     // 窗口是否可见
    
    // 交互数据
    InteractionData m_interactionData;
    
    // 选择任务
    SelectionTask m_selectionTask;
    
public:
    // 获取单例实例
    static InputContext& getInstance();
    
    // 命令执行状态管理，CommandManager通过这个接口来全权维护这个标记，命令开始执行时置为true，执行结束/取消执行后置回false
    void setInCommandExecution(bool inExecution);
    bool isInCommandExecution() const;
    // 是否处于命令执行中或者任何任务(例如选择交互、夹点编辑交互)执行中
    bool isAnyCommandOrTaskRunning() const;
    
    // 提示信息相关
    void setPrompt(const std::string& prompt);
    const std::string& getPrompt() const;
    void setErrorPrompt(const std::string& errorPrompt);
    
    // 交互状态管理
    InputStatus getCurrentStatus();
    void resetStatusExceptInteractionData(); // 清除所有交互状态，除了交互数据，提供给选择任务使用
    void resetStatus(); // 清除所有交互状态，包括交互数据
    
    // 允许的输入类型管理
    void setAllowedTypes(const std::vector<InputType>& types);
    const std::vector<InputType>& getAllowedTypes() const;
    
    // 点拾取相关
    void setPickedPoint(const glm::dvec3& point);
    bool getPickedPoint(glm::dvec3& point);
    
    // 处理鼠标左键点击事件（由InputHandler调用）
    void handleLeftMouseClick();
    
    // 处理鼠标右键点击（由InputHandler调用）
    void handleRightMouseClick();
    
    // 数字输入相关
    bool getInteger(int& value);
    bool getFloat(double& value);
    bool getNumber(double& value);
    
    // 字符串输入相关
    bool getString(std::string& str);
    
    // 关键字输入相关
    void setKeywordOptions(const std::vector<std::string>& options);
    const std::vector<std::string>& getKeywordOptions() const;
    bool getKeyword(std::string& keyword);
    
    // 获取选择结果
    bool getSelectionSet(SelectionSet& selectionSet);
    
    // 输入解析
    void parseInput(const std::string& input);
    
    // 特殊按键事件管理，Enter/Space/Esc
    void setSpecialKeyEvent(SpecialKeyEventType event);
    SpecialKeyEventType getLastSpecialKeyEvent() const;
    void clearSpecialKeyEvent();
    
    // 处理Enter/Space/Esc输入
    void handleEnterSpace(const std::string& input); // 处理Enter/Space输入
    void handleEscape(const std::string& input); // 处理Escape输入
    
    // 等待点输入（带基点）
    void waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords, bool drawRubberBand = false);
    
    // 等待点输入（无基点）
    void waitForPoint(const std::string& prompt, const std::vector<std::string>& keywords = {});
    
    // 等待数值输入
    void waitForNumber(const std::string& prompt, double min = -DBL_MAX, double max = DBL_MAX);
    void waitForNumber(const std::string& prompt, double min, double max, const std::vector<std::string>& keywords);
    
    // 等待整数输入
    void waitForInteger(const std::string& prompt, int min = INT_MIN, int max = INT_MAX);
    void waitForInteger(const std::string& prompt, int min, int max, const std::vector<std::string>& keywords);
    
    // 等待浮点数输入
    void waitForFloat(const std::string& prompt, double min = -DBL_MAX, double max = DBL_MAX);
    
    // 等待字符串输入
    void waitForString(const std::string& prompt);
    
    // 等待关键字输入
    void waitForKeyword(const std::string& prompt, const std::vector<std::string>& keywords);
    
    // 等待回车输入
    void waitForEnter(const std::string& prompt);
    
    // 等待选择交互
    void waitForSelection(const std::string& prompt, const std::vector<std::string>& keywords = {});
    
    // 输入上下文信息窗口相关
    void drawInfoWindow();
    bool& getInputContextInfoVisible() { return m_inputContextInfoVisible; }
    
    // 获取交互数据
    InteractionData& getInteractionData();

    // 获取实时鼠标预览点的世界坐标
    glm::dvec3 getPreviewPoint() const;

    // 正交模式是否激活（数据库ORTHOMODE与Shift状态共同决定，且需要基点才有效）
    bool isOrthoActive() const;

    // LastPoint相关方法
    void setLastPoint(const glm::dvec3& point);
    glm::dvec3 getLastPoint() const;

    // 更新输入上下文
    void onUpdate();
    
    // ============================================================================
    // 选择集相关接口: 先选选择集与实体批量高亮
    // ============================================================================

    // 获取先选选择集（没有命令活跃时的选择集）
    const SelectionSet& getPriorSelectionSet() const { return m_priorSelectionSet; }

    // 设置先选选择集并更新高亮（取消旧高亮，高亮新选择集）
    void setPriorSelectionSet(const SelectionSet& selectionSet);

    // 添加到先选选择集并高亮新增实体
    void addToPriorSelectionSet(const SelectionSet& selectionSet);

    // 清空先选选择集并取消高亮
    void clearPriorSelectionSet();

    // 选中高亮选择集中的所有实体（提供给命令层调用）
    void highlightSelectionSet(const SelectionSet& selectionSet);

    // 取消选择集中所有实体的选中高亮（提供给命令层调用）
    void unhighlightSelectionSet(const SelectionSet& selectionSet);
    
private:
    // 激活选择任务
    void activateSelectionTask(SelectionMode mode);
};

} // namespace tch
