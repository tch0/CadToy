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
#include "DocManager.h"
#include "Database.h"
#include "DbLine.h"
#include "UndoManager.h"


namespace tch {

CommandLine::CommandLine() :
    m_state(kStartPointEntry),
    m_startPoint(0, 0, 0),
    m_currentPoint(0, 0, 0),
    m_lineIds(),
    m_pDb(DocManager::getCurrentDocument().getDatabase()) {
}

// 创建新线段并入库
void CommandLine::createNewLine(const glm::dvec3& start, const glm::dvec3& end) {
    auto line = std::make_unique<DbLine>(start, end);
    line->setPropertiesFromDb();
    
    ObjectId id = m_pDb->addObject(std::move(line));
    m_lineIds.push_back(id);
    
    // 记录添加操作到undo栈
    UndoManager::getInstance().recordAdd(id);
}

// 更新最后一条线段的终点
void CommandLine::updateLastLineEnd(const glm::dvec3& end) {
    if (m_lineIds.empty()) {
        return;
    }
    
    ObjectId lastId = m_lineIds.back();
    auto* pEntity = m_pDb->getEntity(lastId);
    if (!pEntity) {
        return;
    }
    
    auto* pLine = pEntity->as<DbLine>();
    if (pLine) {
        pLine->setEnd(end);
    }
}

// 删除最后一条线段（U操作）
void CommandLine::removeLastLine() {
    if (m_lineIds.empty()) {
        return;
    }
    
    ObjectId lastId = m_lineIds.back();
    m_pDb->removeObject(lastId);
    m_lineIds.pop_back();
    // 记录删除操作到undo栈
    UndoManager::getInstance().recordRemove(lastId);
    
    // 更新起点为新的最后一条线的终点
    if (!m_lineIds.empty()) {
        ObjectId newLastId = m_lineIds.back();
        auto* pEntity = m_pDb->getEntity(newLastId);
        if (!pEntity) {
            return;
        }
        
        auto* pLine = pEntity->as<DbLine>();
        if (pLine) {
            m_startPoint = pLine->end();
        }
    }
}

// 删除最后一条预览线段（命令结束时，最后一段总是预览）
void CommandLine::finalizeLastLine() {
    if (m_lineIds.empty()) {
        return;
    }
    
    // 最后一条始终是预览线段，直接删除
    ObjectId lastId = m_lineIds.back();
    m_pDb->removeObject(lastId);
    m_lineIds.pop_back();
    // 记录删除操作到undo栈
    UndoManager::getInstance().recordRemove(lastId);
}

void CommandLine::onUpdate() {
    auto& ctx = InputContext::getInstance();
    auto& loc = LocalizationManager::getInstance();
    
    // 检查是否已经完成
    if (isCompleted()) {
        return;
    }
    
    // 数据库为空，结束命令
    if (m_pDb == nullptr) {
        finish();
        return;
    }
    
    switch (m_state) {
        case kStartPointEntry: {
            // 等待起点输入
            ctx.waitForPoint(loc.get("command.line.startPoint"));
            m_state = kStartPointQuery;
            break;
        }
            
        case kStartPointQuery: {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，直接返回
            if (status == InputStatus::kNone) {
                break;
            }
            // Esc/Enter/Space，结束命令
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput) {
                m_state = kCompleted;
            }
            // 获取第一点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_startPoint)) {
                    m_firstPoint = m_startPoint;
                    m_currentPoint = m_startPoint;
                    // 创建第一条线（起点=终点，作为预览）
                    createNewLine(m_startPoint, m_startPoint);
                    m_state = kNextPointEntry;
                }
            }
            break;
        }
            
        case kNextPointEntry: {
            // 等待下一点输入
            std::vector<std::string> keywords = {"U"};
            std::string prompt;
            if (m_lineIds.size() >= 2) {
                prompt = loc.get("command.line.nextPointWithClose");
                keywords.push_back("C");
            } else {
                prompt = loc.get("command.line.nextPoint");
            }
            ctx.waitForPoint(prompt, m_startPoint, keywords);
            m_state = kNextPointQuery;
            break;
        }
            
        case kNextPointQuery: {
            // 检查输入状态
            InputStatus status = ctx.getCurrentStatus();
            
            // 无输入，更新预览
            if (status == InputStatus::kNone) {
                // 获取鼠标当前位置更新预览
                m_currentPoint = ctx.getPreviewPoint();
                updateLastLineEnd(m_currentPoint);
                break;
            }
            // Esc、Enter，进入结束状态
            else if (status == InputStatus::kCanceled || status == InputStatus::kEnterInput) {
                finalizeLastLine();
                m_state = kCompleted;
            }
            else if (status == InputStatus::kKeywordInput) {
                std::string keyword;
                ctx.getKeyword(keyword);
                
                if (keyword == "C") {
                    // 闭合：连接到第一条线的起点
                    if (m_lineIds.size() >= 2) {
                        updateLastLineEnd(m_firstPoint);
                        m_state = kCompleted;
                    }
                }
                else if (keyword == "U") {
                    // 撤销最后一条线
                    removeLastLine();
                    
                    if (m_lineIds.empty()) {
                        // 所有线都删完了，回到起点输入
                        m_state = kStartPointEntry;
                        Utils::cmdLinePrint(loc.get("command.line.abandonedAll"));
                    } else {
                        // 还有线，继续输入下一点
                        m_state = kNextPointEntry;
                    }
                }
            }
            // 获取点输入
            else if (status == InputStatus::kPointInput) {
                if (ctx.getPickedPoint(m_currentPoint)) {
                    // 更新最后一条线的终点
                    updateLastLineEnd(m_currentPoint);
                    m_startPoint = m_currentPoint;
                    
                    // 创建新的预览线
                    createNewLine(m_startPoint, m_startPoint);
                    m_state = kNextPointEntry;
                }
            }
            break;
        }
        
        case kCompleted: {
            // 执行统一的结束操作
            finish();
            break;
        }
    }
}

} // namespace tch
