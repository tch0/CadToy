#pragma once
#include <memory>
#include <vector>
#include <string>
#include "Geometry.h"
#include "Layer.h"
#include <glm/glm.hpp>

namespace tch {

// 操作接口
class Operation {
public:
    virtual ~Operation() = default;
    
    // 执行操作
    virtual void execute() = 0;
    
    // 撤销操作
    virtual void undo() = 0;
    
    // 重做操作
    virtual void redo() = 0;
    
    // 获取操作名称
    virtual std::string getName() const = 0;
};

// 绘制操作
class DrawOperation : public Operation {
public:
    DrawOperation(const std::shared_ptr<Shape>& shape) : m_shape(shape) {}
    
    void execute() override {
        // 绘制操作在创建时已经执行
    }
    
    void undo() override {
        // 从图层中移除图形
        if (m_shape) {
            int layerId = m_shape->getLayer();
            auto layer = LayerManager::getInstance().getLayer(layerId);
            if (layer) {
                layer->removeShape(m_shape);
            }
        }
    }
    
    void redo() override {
        // 添加图形到图层
        if (m_shape) {
            int layerId = m_shape->getLayer();
            auto layer = LayerManager::getInstance().getLayer(layerId);
            if (layer) {
                layer->addShape(m_shape);
            }
        }
    }
    
    std::string getName() const override {
        return "Draw";
    }

private:
    std::shared_ptr<Shape> m_shape;
};

// 平移操作
class TranslateOperation : public Operation {
public:
    TranslateOperation(const std::shared_ptr<Shape>& shape, const glm::vec2& delta)
        : m_shape(shape), m_delta(delta) {}
    
    void execute() override {
        if (m_shape) {
            m_shape->translate(m_delta);
        }
    }
    
    void undo() override {
        if (m_shape) {
            m_shape->translate(-m_delta);
        }
    }
    
    void redo() override {
        execute();
    }
    
    std::string getName() const override {
        return "Translate";
    }

private:
    std::shared_ptr<Shape> m_shape;
    glm::vec2 m_delta;
};

// 旋转操作
class RotateOperation : public Operation {
public:
    RotateOperation(const std::shared_ptr<Shape>& shape, float angle, const glm::vec2& center)
        : m_shape(shape), m_angle(angle), m_center(center) {}
    
    void execute() override {
        if (m_shape) {
            m_shape->rotate(m_angle, m_center);
        }
    }
    
    void undo() override {
        if (m_shape) {
            m_shape->rotate(-m_angle, m_center);
        }
    }
    
    void redo() override {
        execute();
    }
    
    std::string getName() const override {
        return "Rotate";
    }

private:
    std::shared_ptr<Shape> m_shape;
    float m_angle;
    glm::vec2 m_center;
};

// 缩放操作
class ScaleOperation : public Operation {
public:
    ScaleOperation(const std::shared_ptr<Shape>& shape, float factor, const glm::vec2& center)
        : m_shape(shape), m_factor(factor), m_center(center) {}
    
    void execute() override {
        if (m_shape) {
            m_shape->scale(m_factor, m_center);
        }
    }
    
    void undo() override {
        if (m_shape) {
            m_shape->scale(1.0f / m_factor, m_center);
        }
    }
    
    void redo() override {
        execute();
    }
    
    std::string getName() const override {
        return "Scale";
    }

private:
    std::shared_ptr<Shape> m_shape;
    float m_factor;
    glm::vec2 m_center;
};

// 撤销/重做管理器
class UndoRedoManager {
public:
    // 获取单例实例
    static UndoRedoManager& getInstance();
    
    // 禁止拷贝和赋值
    UndoRedoManager(const UndoRedoManager&) = delete;
    UndoRedoManager& operator=(const UndoRedoManager&) = delete;
    
    // 添加操作
    void addOperation(const std::shared_ptr<Operation>& operation);
    
    // 撤销
    bool undo();
    
    // 重做
    bool redo();
    
    // 清空操作历史
    void clear();
    
    // 检查是否可以撤销
    bool canUndo() const;
    
    // 检查是否可以重做
    bool canRedo() const;
    
    // 获取撤销栈大小
    size_t getUndoStackSize() const;
    
    // 获取重做栈大小
    size_t getRedoStackSize() const;

private:
    // 私有构造函数
    UndoRedoManager() = default;
    
    std::vector<std::shared_ptr<Operation>> m_undoStack;
    std::vector<std::shared_ptr<Operation>> m_redoStack;
    const size_t m_maxStackSize = 100; // 最大栈大小
};

} // namespace tch