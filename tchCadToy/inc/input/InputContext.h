#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace tch {

// 输入状态枚举
enum class InputStatus {
    kNone,          // 无输入
    kCanceled,      // 取消输入（Esc键）
    kIntegerInput,  // 整数输入
    kFloatInput,    // 浮点数输入
    kStringInput,   // 字符串输入
    kKeywordInput,  // 关键字输入
    kPointInput     // 点坐标输入
};

// 输入类型枚举
enum class InputType {
    kInteger,   // 整数输入
    kFloat,     // 浮点数输入
    kString,    // 字符串输入
    kKeyword,   // 关键字输入
    kPoint      // 点坐标输入
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
    
    // 是否需要终止命令
    bool m_shouldAbortCommand;
    
    // 当前输入状态
    InputStatus m_currentStatus;
    
    // 允许的输入类型列表
    std::vector<InputType> m_allowedTypes;
    
    // 提示信息
    std::string m_prompt;
    
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

public:
    // 获取单例实例
    static InputContext& getInstance();
    
    // 命令执行状态管理
    void setInCommandExecution(bool inExecution);
    bool isInCommandExecution() const;
    
    // 提示信息相关
    void setPrompt(const std::string& prompt);
    const std::string& getPrompt() const;
    
    // 输入状态管理
    InputStatus getCurrentStatus() const;
    void resetStatus();
    
    // 允许的输入类型管理
    void setAllowedTypes(const std::vector<InputType>& types);
    const std::vector<InputType>& getAllowedTypes() const;
    
    // 点拾取相关
    void setPickedPoint(const glm::dvec3& point);
    bool getPickedPoint(glm::dvec3& point);
    
    // 处理鼠标左键点击（由InputHandler调用）
    void handleLeftMouseClick(const glm::vec2& screenPos);
    
    // 处理鼠标右键点击（由InputHandler调用）
    void handleRightMouseClick(const glm::vec2& screenPos);
    
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
    
    // 输入解析
    void parseInput(const std::string& input);
    
    // 取消操作
    void cancel();
    bool isCanceled() const;
    
    // 中止操作（强制取消整个命令）
    void abort();
    bool shouldAbortCommand() const;
    
    // 预览功能（暂时空实现）
    void drawRubberBand(const glm::dvec3& startPoint);
    
    // 处理命令输入
    void handleCommandInput(const std::string& input);
};

} // namespace tch
