#pragma once

// C++ 标准库
#include <memory>
#include <string>
#include <vector>

// 第三方库
#include <glm/glm.hpp>

// 项目头文件
#include "CommonTypes.h"
#include "SelectionTask.h"
#include "Task.h"

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
    
    // 点输入相关
    glm::dvec3 m_pickedPoint; // 拾取获取输入的点
    bool m_bHasBasePoint; // 是否有基点
    glm::dvec3 m_basePoint; // 点输入的基点，由带基点版本waitForPoint传入
    
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
    // TODO: 暂时占位，实体具体细节还未实现
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
    
    // 处理Enter/Space/Esc输入
    void handleEnterSpace(const std::string& input); // 处理Enter/Space输入
    void handleEscape(const std::string& input); // 处理Escape输入
    
    // 等待点输入（带基点）
    void waitForPoint(const std::string& prompt, const glm::dvec3& basePoint, const std::vector<std::string>& keywords);
    
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
