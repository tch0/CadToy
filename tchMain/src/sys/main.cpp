#include <GLFW/glfw3.h>
#include "glad/gl.h"
#include "input/InputHandler.h"
#include "render/Renderer.h"
#include "command/CommandParser.h"
#include <glm/glm.hpp>
#include <iostream>

using namespace tch;

int main() {
    std::cout << "Starting CadToy..." << std::endl;
    
    // 初始化GLFW
    std::cout << "Initializing GLFW..." << std::endl;
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    std::cout << "GLFW initialized successfully!" << std::endl;
    
    // 创建窗口
    std::cout << "Creating window..." << std::endl;
    GLFWwindow* window = glfwCreateWindow(800, 600, "CadToy", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "Window created successfully!" << std::endl;
    
    // 设置当前上下文
    std::cout << "Setting GLFW context..." << std::endl;
    glfwMakeContextCurrent(window);
    std::cout << "Context set successfully!" << std::endl;
    
    // 初始化OpenGL函数指针（使用glad）
    std::cout << "Initializing glad..." << std::endl;
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "Failed to initialize glad!" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "glad initialized successfully!" << std::endl;
    
    // 输出OpenGL版本
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    // 初始化输入处理器
    std::cout << "Initializing InputHandler..." << std::endl;
    InputHandler::initialize(window);
    std::cout << "InputHandler initialized successfully!" << std::endl;
    
    // 初始化渲染器
    std::cout << "Initializing Renderer..." << std::endl;
    Renderer::initialize(window);
    std::cout << "Renderer initialized successfully!" << std::endl;
    
    // 注册命令输入回调
    std::cout << "Registering command input callback..." << std::endl;
    InputHandler::registerCallback(InputEventType::COMMAND_ENTERED, []() {
        std::string command = InputHandler::getCommandInput();
        std::cout << "Executing command: " << command << std::endl;
        CommandParser::parseCommand(command);
    });
    std::cout << "Callback registered successfully!" << std::endl;
    
    // 主循环
    std::cout << "Entering main loop..." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        // 处理事件
        glfwPollEvents();
        
        // 开始渲染
        Renderer::beginRender();
        
        // 绘制所有图形
        Renderer::drawAll();
        
        // 绘制光标
        glm::vec2 mousePos = InputHandler::getMousePosition();
        Renderer::drawCursor(mousePos);
        
        // 结束渲染
        Renderer::endRender();
    }
    
    // 清理资源
    std::cout << "Cleaning up resources..." << std::endl;
    Renderer::cleanup();
    
    // 销毁窗口
    std::cout << "Destroying window..." << std::endl;
    glfwDestroyWindow(window);
    
    // 终止GLFW
    std::cout << "Terminating GLFW..." << std::endl;
    glfwTerminate();
    
    std::cout << "CadToy exited successfully!" << std::endl;
    return 0;
}