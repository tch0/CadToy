#include "SaveLoad.h"
#include "Layer.h"
#include "Geometry.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace tch {

// 保存图形到文件
bool SaveLoad::saveToFile(const std::string& filePath) {
    try {
        // 创建文档
        rapidjson::Document doc;
        doc.SetObject();
        auto& allocator = doc.GetAllocator();
        
        // 添加版本信息
        doc.AddMember("version", "1.0", allocator);
        
        // 添加图层信息
        rapidjson::Value layers(rapidjson::kArrayType);
        auto& layerManager = LayerManager::getInstance();
        auto& allLayers = layerManager.getLayers();
        
        for (const auto& pair : allLayers) {
            const auto& layer = pair.second;
            rapidjson::Value layerObj(rapidjson::kObjectType);
            
            // 添加图层ID
            layerObj.AddMember("id", layer->getId(), allocator);
            
            // 添加图层名称
            rapidjson::Value name(layer->getName().c_str(), allocator);
            layerObj.AddMember("name", name, allocator);
            
            // 添加图层可见性
            layerObj.AddMember("visible", layer->isVisible(), allocator);
            
            // 添加图形信息
            rapidjson::Value shapes(rapidjson::kArrayType);
            const auto& layerShapes = layer->getShapes();
            
            for (const auto& shape : layerShapes) {
                rapidjson::Value shapeObj(rapidjson::kObjectType);
                
                // 添加图形类型
                std::string typeName;
                switch (shape->getType()) {
                case ShapeType::POINT:
                    typeName = "POINT";
                    break;
                case ShapeType::LINE:
                    typeName = "LINE";
                    break;
                case ShapeType::CIRCLE:
                    typeName = "CIRCLE";
                    break;
                case ShapeType::RECTANGLE:
                    typeName = "RECTANGLE";
                    break;
                default:
                    typeName = "UNKNOWN";
                    break;
                }
                rapidjson::Value typeVal(typeName.c_str(), allocator);
                shapeObj.AddMember("type", typeVal, allocator);
                
                // 添加颜色信息
                auto color = shape->getColor();
                rapidjson::Value colorObj(rapidjson::kObjectType);
                colorObj.AddMember("r", color.r, allocator);
                colorObj.AddMember("g", color.g, allocator);
                colorObj.AddMember("b", color.b, allocator);
                shapeObj.AddMember("color", colorObj, allocator);
                
                // 添加特定图形的属性
                if (auto point = std::dynamic_pointer_cast<Point>(shape)) {
                    auto pos = point->getPosition();
                    rapidjson::Value posObj(rapidjson::kObjectType);
                    posObj.AddMember("x", pos.x, allocator);
                    posObj.AddMember("y", pos.y, allocator);
                    shapeObj.AddMember("position", posObj, allocator);
                } else if (auto line = std::dynamic_pointer_cast<Line>(shape)) {
                    auto start = line->getStart();
                    auto end = line->getEnd();
                    rapidjson::Value startObj(rapidjson::kObjectType);
                    startObj.AddMember("x", start.x, allocator);
                    startObj.AddMember("y", start.y, allocator);
                    shapeObj.AddMember("start", startObj, allocator);
                    
                    rapidjson::Value endObj(rapidjson::kObjectType);
                    endObj.AddMember("x", end.x, allocator);
                    endObj.AddMember("y", end.y, allocator);
                    shapeObj.AddMember("end", endObj, allocator);
                } else if (auto circle = std::dynamic_pointer_cast<Circle>(shape)) {
                    auto center = circle->getCenter();
                    rapidjson::Value centerObj(rapidjson::kObjectType);
                    centerObj.AddMember("x", center.x, allocator);
                    centerObj.AddMember("y", center.y, allocator);
                    shapeObj.AddMember("center", centerObj, allocator);
                    shapeObj.AddMember("radius", circle->getRadius(), allocator);
                } else if (auto rectangle = std::dynamic_pointer_cast<Rectangle>(shape)) {
                    auto pos = rectangle->getPosition();
                    rapidjson::Value posObj(rapidjson::kObjectType);
                    posObj.AddMember("x", pos.x, allocator);
                    posObj.AddMember("y", pos.y, allocator);
                    shapeObj.AddMember("position", posObj, allocator);
                    shapeObj.AddMember("width", rectangle->getWidth(), allocator);
                    shapeObj.AddMember("height", rectangle->getHeight(), allocator);
                }
                
                shapes.PushBack(shapeObj, allocator);
            }
            
            layerObj.AddMember("shapes", shapes, allocator);
            layers.PushBack(layerObj, allocator);
        }
        
        doc.AddMember("layers", layers, allocator);
        
        // 序列化到文件
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << buffer.GetString();
        file.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

// 从文件加载图形
bool SaveLoad::loadFromFile(const std::string& filePath) {
    try {
        // 读取文件
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        // 解析JSON
        rapidjson::Document doc;
        doc.Parse(content.c_str());
        
        if (doc.HasParseError()) {
            return false;
        }
        
        // 清空现有图层
        auto& layerManager = LayerManager::getInstance();
        layerManager.clearAllLayers();
        
        // 加载图层
        if (doc.HasMember("layers") && doc["layers"].IsArray()) {
            const auto& layers = doc["layers"];
            
            for (size_t i = 0; i < layers.Size(); ++i) {
                const auto& layerObj = layers[i];
                
                // 获取图层信息
                int layerId = layerObj["id"].GetInt();
                std::string layerName = layerObj["name"].GetString();
                bool visible = layerObj["visible"].GetBool();
                
                // 创建图层
                int newLayerId = layerManager.createLayer(layerName);
                auto layer = layerManager.getLayer(newLayerId);
                if (layer) {
                    layer->setVisible(visible);
                    
                    // 加载图形
                    if (layerObj.HasMember("shapes") && layerObj["shapes"].IsArray()) {
                        const auto& shapes = layerObj["shapes"];
                        
                        for (size_t j = 0; j < shapes.Size(); ++j) {
                            const auto& shapeObj = shapes[j];
                            
                            // 获取图形类型
                            std::string typeName = shapeObj["type"].GetString();
                            
                            // 获取颜色
                            glm::vec3 color(1.0f, 1.0f, 1.0f);
                            if (shapeObj.HasMember("color")) {
                                const auto& colorObj = shapeObj["color"];
                                color.r = colorObj["r"].GetFloat();
                                color.g = colorObj["g"].GetFloat();
                                color.b = colorObj["b"].GetFloat();
                            }
                            
                            // 创建图形
                            std::shared_ptr<Shape> shape;
                            
                            if (typeName == "POINT") {
                                const auto& posObj = shapeObj["position"];
                                float x = posObj["x"].GetFloat();
                                float y = posObj["y"].GetFloat();
                                shape = std::make_shared<Point>(glm::vec2(x, y));
                            } else if (typeName == "LINE") {
                                const auto& startObj = shapeObj["start"];
                                const auto& endObj = shapeObj["end"];
                                float startX = startObj["x"].GetFloat();
                                float startY = startObj["y"].GetFloat();
                                float endX = endObj["x"].GetFloat();
                                float endY = endObj["y"].GetFloat();
                                shape = std::make_shared<Line>(glm::vec2(startX, startY), glm::vec2(endX, endY));
                            } else if (typeName == "CIRCLE") {
                                const auto& centerObj = shapeObj["center"];
                                float centerX = centerObj["x"].GetFloat();
                                float centerY = centerObj["y"].GetFloat();
                                float radius = shapeObj["radius"].GetFloat();
                                shape = std::make_shared<Circle>(glm::vec2(centerX, centerY), radius);
                            } else if (typeName == "RECTANGLE") {
                                const auto& posObj = shapeObj["position"];
                                float x = posObj["x"].GetFloat();
                                float y = posObj["y"].GetFloat();
                                float width = shapeObj["width"].GetFloat();
                                float height = shapeObj["height"].GetFloat();
                                shape = std::make_shared<Rectangle>(glm::vec2(x, y), width, height);
                            }
                            
                            // 设置颜色和图层
                            if (shape) {
                                shape->setColor(color);
                                shape->setLayer(newLayerId);
                                layer->addShape(shape);
                            }
                        }
                    }
                }
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

// 导出为SVG格式
bool SaveLoad::exportToSVG(const std::string& filePath) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        // 写入SVG头部
        file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
";
        file << "<svg width=\"800\" height=\"600\" xmlns=\"http://www.w3.org/2000/svg\">
";
        
        // 写入图形
        auto& layerManager = LayerManager::getInstance();
        auto& allLayers = layerManager.getLayers();
        
        for (const auto& pair : allLayers) {
            const auto& layer = pair.second;
            if (!layer->isVisible()) {
                continue;
            }
            
            const auto& shapes = layer->getShapes();
            for (const auto& shape : shapes) {
                auto color = shape->getColor();
                std::string colorStr = "rgb(" + std::to_string(static_cast<int>(color.r * 255)) + "," +
                                      std::to_string(static_cast<int>(color.g * 255)) + "," +
                                      std::to_string(static_cast<int>(color.b * 255)) + ")";
                
                if (auto point = std::dynamic_pointer_cast<Point>(shape)) {
                    auto pos = point->getPosition();
                    file << "  <circle cx=\"" << pos.x << "\" cy=\"" << pos.y << "\" r=\"3\" fill=\"" << colorStr << "\" />
";
                } else if (auto line = std::dynamic_pointer_cast<Line>(shape)) {
                    auto start = line->getStart();
                    auto end = line->getEnd();
                    file << "  <line x1=\"" << start.x << "\" y1=\"" << start.y << "\" x2=\"" << end.x << "\" y2=\"" << end.y << "\" stroke=\"" << colorStr << "\" stroke-width=\"2\" />
";
                } else if (auto circle = std::dynamic_pointer_cast<Circle>(shape)) {
                    auto center = circle->getCenter();
                    float radius = circle->getRadius();
                    file << "  <circle cx=\"" << center.x << "\" cy=\"" << center.y << "\" r=\"" << radius << "\" fill=\"none\" stroke=\"" << colorStr << "\" stroke-width=\"2\" />
";
                } else if (auto rectangle = std::dynamic_pointer_cast<Rectangle>(shape)) {
                    auto pos = rectangle->getPosition();
                    float width = rectangle->getWidth();
                    float height = rectangle->getHeight();
                    file << "  <rect x=\"" << pos.x << "\" y=\"" << pos.y << "\" width=\"" << width << "\" height=\"" << height << "\" fill=\"none\" stroke=\"" << colorStr << "\" stroke-width=\"2\" />
";
                }
            }
        }
        
        // 写入SVG尾部
        file << "</svg>
";
        file.close();
        
        return true;
    } catch (...) {
        return false;
    }
}

// 导出为PNG格式
bool SaveLoad::exportToPNG(const std::string& filePath) {
    // 这里简化处理，实际应该使用OpenGL截图或其他库来实现
    // 暂时返回false
    return false;
}

} // namespace tch