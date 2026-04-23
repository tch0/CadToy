// 对应头文件
#include "UndoManager.h"

// C++ 标准库

// 第三方库

// 项目头文件
#include "DocManager.h"
#include "Document.h"
#include "UndoStack.h"


namespace tch {

UndoManager& UndoManager::getInstance() {
    static UndoManager instance;
    return instance;
}

UndoStack& UndoManager::getCurrentStack() const {
    return DocManager::getCurrentDocument().getUndoStack();
}

void UndoManager::beginGroup(const std::string& name) {
    getCurrentStack().beginGroup(name);
}

void UndoManager::endGroup() {
    getCurrentStack().endGroup();
}

void UndoManager::recordAdd(ObjectId objId) {
    getCurrentStack().recordAdd(objId);
}

void UndoManager::recordRemove(ObjectId objId) {
    getCurrentStack().recordRemove(objId);
}

void UndoManager::recordModify(ObjectId objId) {
    getCurrentStack().recordModify(objId);
}

bool UndoManager::canUndo() const {
    return getCurrentStack().canUndo();
}

bool UndoManager::canRedo() const {
    return getCurrentStack().canRedo();
}

void UndoManager::undo() {
    getCurrentStack().undo();
}

void UndoManager::redo() {
    getCurrentStack().redo();
}

std::string UndoManager::getUndoName() const {
    return getCurrentStack().getUndoName();
}

std::string UndoManager::getRedoName() const {
    return getCurrentStack().getRedoName();
}

void UndoManager::clear() {
    getCurrentStack().clear();
}

} // namespace tch
