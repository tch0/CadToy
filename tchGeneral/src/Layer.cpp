#include "Layer.h"

namespace tch {

// 图层类实现

// 添加图形
void Layer::addShape(const std::shared_ptr<Shape>& shape) {
    shape->setLayer(m_id);
    m_shapes.push_back(shape);
}

// 移除图形
void Layer::removeShape(const std::shared_ptr<Shape>& shape) {
    auto it = std::find(m_shapes.begin(), m_shapes.end(), shape);
    if (it != m_shapes.end()) {
        m_shapes.erase(it);
    }
}

// 清空图形
void Layer::clearShapes() {
    m_shapes.clear();
}

// 获取图形列表
const std::vector<std::shared_ptr<Shape>>& Layer::getShapes() const {
    return m_shapes;
}

// 获取图层ID
int Layer::getId() const {
    return m_id;
}

// 获取图层名称
std::string Layer::getName() const {
    return m_name;
}

// 设置图层名称
void Layer::setName(const std::string& name) {
    m_name = name;
}

// 获取图层可见性
bool Layer::isVisible() const {
    return m_visible;
}

// 设置图层可见性
void Layer::setVisible(bool visible) {
    m_visible = visible;
}

// 绘制图层上的图形
void Layer::draw() const {
    if (!m_visible) {
        return;
    }
    
    for (const auto& shape : m_shapes) {
        shape->draw();
    }
}

// 图层管理器实现

// 私有构造函数
LayerManager::LayerManager() : m_nextLayerId(0), m_currentLayerId(-1) {
    // 创建默认图层
    createLayer("Default");
}

// 获取单例实例
LayerManager& LayerManager::getInstance() {
    static LayerManager instance;
    return instance;
}

// 创建新图层
int LayerManager::createLayer(const std::string& name) {
    int layerId = m_nextLayerId++;
    m_layers[layerId] = std::make_unique<Layer>(layerId, name);
    
    if (m_currentLayerId == -1) {
        m_currentLayerId = layerId;
    }
    
    return layerId;
}

// 删除图层
void LayerManager::deleteLayer(int layerId) {
    if (layerId == m_currentLayerId && m_layers.size() > 1) {
        // 如果删除的是当前图层，切换到第一个图层
        for (const auto& pair : m_layers) {
            if (pair.first != layerId) {
                m_currentLayerId = pair.first;
                break;
            }
        }
    }
    
    m_layers.erase(layerId);
    
    if (m_layers.empty()) {
        // 如果没有图层了，创建一个默认图层
        createLayer("Default");
    }
}

// 获取图层
Layer* LayerManager::getLayer(int layerId) {
    auto it = m_layers.find(layerId);
    if (it != m_layers.end()) {
        return it->second.get();
    }
    return nullptr;
}

// 获取图层
Layer* LayerManager::getLayer(const std::string& name) {
    for (const auto& pair : m_layers) {
        if (pair.second->getName() == name) {
            return pair.second.get();
        }
    }
    return nullptr;
}

// 获取当前图层
Layer* LayerManager::getCurrentLayer() {
    return getLayer(m_currentLayerId);
}

// 设置当前图层
void LayerManager::setCurrentLayer(int layerId) {
    if (m_layers.find(layerId) != m_layers.end()) {
        m_currentLayerId = layerId;
    }
}

// 获取所有图层
const std::unordered_map<int, std::unique_ptr<Layer>>& LayerManager::getLayers() const {
    return m_layers;
}

// 绘制所有可见图层
void LayerManager::draw() const {
    for (const auto& pair : m_layers) {
        pair.second->draw();
    }
}

// 清空所有图层
void LayerManager::clearAllLayers() {
    m_layers.clear();
    m_nextLayerId = 0;
    m_currentLayerId = -1;
    createLayer("Default");
}

} // namespace tch