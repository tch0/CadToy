#include "UndoRedo.h"
#include "Layer.h"

namespace tch {

// 撤销/重做管理器实现

// 获取单例实例
UndoRedoManager& UndoRedoManager::getInstance() {
    static UndoRedoManager instance;
    return instance;
}

// 添加操作
void UndoRedoManager::addOperation(const std::shared_ptr<Operation>& operation) {
    // 执行操作
    operation->execute();
    
    // 添加到撤销栈
    m_undoStack.push_back(operation);
    
    // 限制栈大小
    if (m_undoStack.size() > m_maxStackSize) {
        m_undoStack.erase(m_undoStack.begin());
    }
    
    // 清空重做栈
    m_redoStack.clear();
}

// 撤销
bool UndoRedoManager::undo() {
    if (m_undoStack.empty()) {
        return false;
    }
    
    // 获取栈顶操作
    auto operation = m_undoStack.back();
    m_undoStack.pop_back();
    
    // 撤销操作
    operation->undo();
    
    // 添加到重做栈
    m_redoStack.push_back(operation);
    
    // 限制栈大小
    if (m_redoStack.size() > m_maxStackSize) {
        m_redoStack.erase(m_redoStack.begin());
    }
    
    return true;
}

// 重做
bool UndoRedoManager::redo() {
    if (m_redoStack.empty()) {
        return false;
    }
    
    // 获取栈顶操作
    auto operation = m_redoStack.back();
    m_redoStack.pop_back();
    
    // 重做操作
    operation->redo();
    
    // 添加到撤销栈
    m_undoStack.push_back(operation);
    
    // 限制栈大小
    if (m_undoStack.size() > m_maxStackSize) {
        m_undoStack.erase(m_undoStack.begin());
    }
    
    return true;
}

// 清空操作历史
void UndoRedoManager::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

// 检查是否可以撤销
bool UndoRedoManager::canUndo() const {
    return !m_undoStack.empty();
}

// 检查是否可以重做
bool UndoRedoManager::canRedo() const {
    return !m_redoStack.empty();
}

// 获取撤销栈大小
size_t UndoRedoManager::getUndoStackSize() const {
    return m_undoStack.size();
}

// 获取重做栈大小
size_t UndoRedoManager::getRedoStackSize() const {
    return m_redoStack.size();
}

} // namespace tch