#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "command/Command.h"

namespace tch {

// 补全项：用于UI渲染与排序
struct CommandCompletionItem {
    std::string key;        // 匹配键(别名或全称)
    std::string fullName;   // 命令全名
    bool isAlias;           // 权重标签

    // 优先级：别名 > 字母顺序，越小排越前，优先级越高
    bool operator<(const CommandCompletionItem& other) const {
        if (isAlias != other.isAlias) {
            return isAlias;
        }
        return key < other.key;
    }
    // 举例: {"L", "LINE", true} 排于 {"LINE", "LINE", false}前，输入L时同时匹配。
};

// 命令管理器类
class CommandManager {
public:
    // 命令创建器类型
    using CommandCreator = std::function<std::unique_ptr<Command>()>;
    
    // 构造函数
    CommandManager();
    
    // 获取单例实例
    static CommandManager& getInstance();
    
    // 注册命令
    template<typename T>
    void registerCommand(const std::string& fullName, const std::vector<std::string>& aliases = {});
    
    // 重建补全候选池
    void rebuildCommandCompletionPool();
    
    // 获取匹配的补全候选列表，按照优先级排序
    std::vector<CommandCompletionItem> getCompletionCandidates(const std::string& partial) const;
    
    // 检查是否有活动命令
    bool hasActiveCommand();
    
    // 获取活动命令（用于预览）
    std::shared_ptr<Command> getActiveCommand();
    
    // 执行命令
    void executeCommand(const std::string& command);
    
    // 取消当前执行的命令，用于比如文档切换、关闭文档、快捷键命令等需要先取消命令的场景
    void cancelCurrentCommand();
    
    // 取消当前正在执行的命令并执行新命令
    void cancelCurrentCommandAndExecute(const std::string& command);
    
    // 运行命令循环
    void runCommandLoop();
    
    // 获取当前运行命令的名称，有命令运行时命令输入框前会显示命令名称
    std::string getRunningCommandName();
    
private:
    // 静态实例
    static std::shared_ptr<CommandManager> s_instance;
    
    // 活动命令
    std::shared_ptr<Command> m_activeCommand;
    
    // 当前正在运行的命令全名
    std::string m_currentCommandName;
    
    // 命令创建器映射表
    std::unordered_map<std::string, CommandCreator> m_creators;
    // 别名映射表
    std::unordered_map<std::string, std::string> m_aliasMap;
    // 命令补全候选池
    std::vector<CommandCompletionItem> m_commandCompletionPool;
    
    // 内部方法
    std::string findFullCommandName(const std::string& input) const;
};

} // namespace tch
