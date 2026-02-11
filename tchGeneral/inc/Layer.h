#pragma once
#include <Geometry.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace tch {

// 图层类
class Layer {
public:
    Layer(int id, const std::string& name) : m_id(id), m_name(name), m_visible(true) {}
    
    // 添加图形
    void addShape(const std::shared_ptr<Shape>& shape);
    
    // 移除图形
    void removeShape(const std::shared_ptr<Shape>& shape);
    
    // 清空图形
    void clearShapes();
    
    // 获取图形列表
    const std::vector<std::shared_ptr<Shape>>& getShapes() const;
    
    // 获取图层ID
    int getId() const;
    
    // 获取图层名称
    std::string getName() const;
    
    // 设置图层名称
    void setName(const std::string& name);
    
    // 获取图层可见性
    bool isVisible() const;
    
    // 设置图层可见性
    void setVisible(bool visible);
    
    // 绘制图层上的图形
    void draw() const;

private:
    int m_id;
    std::string m_name;
    bool m_visible;
    std::vector<std::shared_ptr<Shape>> m_shapes;
};

// 图层管理器
class LayerManager {
public:
    // 获取单例实例
    static LayerManager& getInstance();
    
    // 禁止拷贝和赋值
    LayerManager(const LayerManager&) = delete;
    LayerManager& operator=(const LayerManager&) = delete;
    
    // 创建新图层
    int createLayer(const std::string& name);
    
    // 删除图层
    void deleteLayer(int layerId);
    
    // 获取图层
    Layer* getLayer(int layerId);
    
    // 获取图层
    Layer* getLayer(const std::string& name);
    
    // 获取当前图层
    Layer* getCurrentLayer();
    
    // 设置当前图层
    void setCurrentLayer(int layerId);
    
    // 获取所有图层
    const std::unordered_map<int, std::unique_ptr<Layer>>& getLayers() const;
    
    // 绘制所有可见图层
    void draw() const;
    
    // 清空所有图层
    void clearAllLayers();

private:
    // 私有构造函数
    LayerManager();
    
    // 图层映射
    std::unordered_map<int, std::unique_ptr<Layer>> m_layers;
    
    // 下一个图层ID
    int m_nextLayerId;
    
    // 当前图层ID
    int m_currentLayerId;
};

} // namespace tch