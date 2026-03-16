#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "input/Task.h"
#include "input/SelectionTask.h"
#include "common/CommonTypes.h"

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
    kEscPressed         // Esc键按下
};

// 输入上下文类，用于命令状态机模式的输入管理
class InputContext {
private:
    // 静态实例
    static std::shared_ptr<InputContext> s_instance;
    
public:
    // 构造函数
    InputContext();
    
private:
    // 命令执行状态
    bool m_inCommandExecution;
    
    // 当前输入状态
    InputStatus m_currentStatus;
    
    // 允许的输入类型列表
    std::vector<InputType> m_allowedTypes;
    
    // 提示信息
    std::string m_prompt;
    
    // 错误提示信息
    std::string m_errorPrompt;
    
    // 拾取的点
    glm::dvec3 m_pickedPoint;
    
    // 输入的整数
    int m_inputInteger;
    
    // 输入的浮点数
    double m_inputFloat;
    
    // 输入的字符串
    std::string m_inputString;
    
    // 输入的关键字
    std::string m_inputKeyword;
    
    // 关键字选项列表
    std::vector<std::string> m_keywordOptions;
    
    // 选择的实体
    std::vector<void*> m_selectedEntities;
    
    // 最后一次特殊按键事件
    SpecialKeyEventType m_lastSpecialKeyEvent;
    
    // 输入上下文信息窗口相关
    bool m_inputContextInfoVisible;     // 窗口是否可见
    
    // 交互数据
    InteractionData m_interactionData;
    
    // 当前活动任务
    std::unique_ptr<Task> m_activeTask;
    
    // 选择任务
    std::unique_ptr<SelectionTask> m_selectionTask;
    
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
    
    // 输入状态管理
    InputStatus getCurrentStatus();
    void resetStatus();
    
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
    
    // 实体选择相关
    void setSelectedEntities(const std::vector<void*>& entities);
    bool getSelectedEntities(std::vector<void*>& entities);
    
    // 输入解析
    void parseInput(const std::string& input);
    
    // 预览功能（暂时空实现）
    void drawRubberBand(const glm::dvec3& startPoint);
    
    // 特殊按键事件管理，Enter/Space/Esc
    void setSpecialKeyEvent(SpecialKeyEventType event);
    SpecialKeyEventType getLastSpecialKeyEvent() const;
    void clearSpecialKeyEvent();
    
    // TODO: 现在两个重载处理不同的事情，当事件来临时两个版本函数都会被调用，后续看是否需要统一起来
    void handleEnterSpace(const std::string& input);        // 处理Enter/Space输入
    void handleEscape(const std::string& input);            // 处理Escape输入
    
    // 等待点输入（带基点）
    void waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords);
    
    // 等待点输入（无基点）
    void waitForPoint(const std::string& prompt, const std::vector<std::string>& keywords = {});
    
    // 等待数值输入
    void waitForNumber(const std::string& prompt, double min = -DBL_MAX, double max = DBL_MAX);
    
    // 等待整数输入
    void waitForInteger(const std::string& prompt, int min = INT_MIN, int max = INT_MAX);
    
    // 等待浮点数输入
    void waitForFloat(const std::string& prompt, double min = -DBL_MAX, double max = DBL_MAX);
    
    // 等待字符串输入
    void waitForString(const std::string& prompt);
    
    // 等待关键字输入
    void waitForKeyword(const std::string& prompt, const std::vector<std::string>& options);
    
    // 等待回车输入
    void waitForEnter(const std::string& prompt);
    
    // 等待实体选择输入
    void waitForEntity(const std::string& prompt, const std::vector<void*>& existingEntities = {},
        const std::vector<std::string>& keywords = {});
    
    // 输入上下文信息窗口相关
    void drawInfoWindow();
    bool& getInputContextInfoVisible() { return m_inputContextInfoVisible; }
    
    // 获取交互数据
    InteractionData& getInteractionData();
    
    // 更新输入上下文
    void onUpdate();
    
    // 激活选择任务
    void activateSelectionTask(SelectionMode mode);
};

} // namespace tch
