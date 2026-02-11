#include <GLFW/glfw3.h>
#include "input/InputHandler.h"
#include "render/Renderer.h"
#include "command/CommandParser.h"

int main() {
    // 初始化GLFW
    if (!glfwInit()) {
        return -1;
    }
    
    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "CadToy", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    // 设置当前上下文
    glfwMakeContextCurrent(window);
    
    // 初始化OpenGL函数指针（如果使用glad）
    // 这里假设已经配置了glad
    
    // 初始化输入处理器
    InputHandler::initialize(window);
    
    // 初始化渲染器
    Renderer::initialize(window);
    
    // 注册命令输入回调
    InputHandler::registerCallback(InputEventType::COMMAND_ENTERED, []() {
        std::string command = InputHandler::getCommandInput();
        CommandParser::parseCommand(command);
    });
    
    // 主循环
    while (!glfwWindowShouldClose(window)) {
        // 处理事件
        glfwPollEvents();
        
        // 开始渲染
        Renderer::beginRender();
        
        // 绘制所有图形
        Renderer::drawAll();
        
        // 结束渲染
        Renderer::endRender();
    }
    
    // 清理资源
    Renderer::cleanup();
    
    // 销毁窗口
    glfwDestroyWindow(window);
    
    // 终止GLFW
    glfwTerminate();
    
    return 0;
}