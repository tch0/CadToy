// 对应头文件
#include "CommandLine.h"

// C++ 标准库

// 第三方库
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

// 项目头文件
#include "InputContext.h"
#include "GlobalUtils.h"
#include "LocalizationManager.h"


namespace tch {

CommandLine::CommandLine() : 
    m_state(CommandLineState::kStartPointEntry),
    m_startPoint(glm::dvec3(0, 0, 0)),
    m_currentPoint(glm::dvec3(0, 0, 0)),
    m_points() {
}

void CommandLine::onUpdate() {
    auto& ctx = InputContext::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    switch (m_state) {
        case CommandLineState::kStartPointEntry:
        {
            // 等待起点输入
            ctx.waitForPoint(loc.get("command.line.startPoint")); // 指定起点:
            m_state = CommandLineState::kStartPointQuery;
            break;
        }
            
        case CommandLineState::kStartPointQuery:
        {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，直接返回
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc/Enter/Space，结束命令
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput) {
                m_state = CommandLineState::kCompleted;
            }
            // 获取第一点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_startPoint)) {
                    m_currentPoint = m_startPoint;
                    m_points.push_back(m_startPoint); // 保存起点
                    m_state = CommandLineState::kNextPointEntry;
                }
            }
            break;
        }
            
        case CommandLineState::kNextPointEntry:
        {
            // 等待下一点输入
            std::vector<std::string> keywords = {"U"};
            std::string prompt;
            if (m_points.size() >= 3) {
                prompt = loc.get("command.line.nextPointWithClose"); // 指定下一点或 [闭合(C)/放弃(U)]:
                keywords = {"C", "U"};
            } else {
                prompt = loc.get("command.line.nextPoint"); // 指定下一点或 [放弃(U)]:
                keywords = {"U"};
            }
            ctx.waitForPoint(prompt, m_startPoint, keywords);
            m_state = CommandLineState::kNextPointQuery;
            break;
        }
            
        case CommandLineState::kNextPointQuery:
        {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，直接返回
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc、Enter，进入结束状态
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput) {
                m_state = CommandLineState::kCompleted;
            }
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                if (keyword == "C") {
                    if (m_points.size() >= 3) {
                        m_points.push_back(m_points.front());
                        m_state = CommandLineState::kCompleted;
                    }
                }
                else if (keyword == "U") {
                    // 只有一个点，放弃第一点
                    if (m_points.size() == 1) {
                        m_points.pop_back();
                        m_state = CommandLineState::kStartPointEntry;
                        Utils::cmdLinePrint(loc.get("command.line.abandonedAll")); // 已放弃所有线段。
                    }
                    else if (m_points.size() >= 2) {
                        m_points.pop_back();
                        m_state = CommandLineState::kNextPointEntry;
                    }
                }
            }
            // 获取点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_currentPoint)) {
                    m_points.push_back(m_currentPoint); // 保存下一点
                    m_startPoint = m_currentPoint;
                    m_state = CommandLineState::kNextPointEntry;
                }
            }
            break;
        }
        
        case CommandLineState::kCompleted:
        {
            // 执行统一的结束操作
            finish();
            break;
        }
    }
}

} // namespace tch