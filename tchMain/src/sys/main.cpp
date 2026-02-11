// glad/gl.h必须包含在glfw/glfw3.h之前，否则可能会导致奇怪无法找到定义的报错，而报错全然与glad/glfw无关，令人迷惑
#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include "input/InputHandler.h"
#include "render/Renderer.h"
#include "command/CommandParser.h"
#include "debug/Logger.h"
#include <glm/glm.hpp>
#include <iostream>

using namespace tch;

int main() {
    // 测试Logger功能
    LOG_INFO("Starting CadToy...");
    
    // 测试不同级别的日志
    LOG_TRACE("This is a trace message");
    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARNING("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_FATAL("This is a fatal message");
    
    // 测试格式化日志
    LOG_INFO("Testing formatted log: {} + {} = {}", 1, 2, 3);
    LOG_INFO("Testing string formatting: Hello, {}", "world");
    
    // 测试异步日志
    LOG_INFO("Testing async logging...");
    auto logger = &globalLogger();
    logger->setAsyncLogging(true);
    LOG_INFO("Async logging enabled");
    
    // 测试日志文件
    LOG_INFO("Testing log file...");
    
    // 初始化GLFW
    LOG_INFO("Initializing GLFW...");
    if (!glfwInit()) {
        LOG_ERROR("Failed to initialize GLFW!");
        return -1;
    }
    LOG_INFO("GLFW initialized successfully!");
    
    // 创建窗口
    LOG_INFO("Creating window...");
    GLFWwindow* window = glfwCreateWindow(800, 600, "CadToy", NULL, NULL);
    if (!window) {
        LOG_ERROR("Failed to create window!");
        glfwTerminate();
        return -1;
    }
    LOG_INFO("Window created successfully!");
    
    // 设置当前上下文
    LOG_INFO("Setting GLFW context...");
    glfwMakeContextCurrent(window);
    LOG_INFO("Context set successfully!");
    
    // 初始化OpenGL函数指针（使用glad）
    LOG_INFO("Initializing glad...");
    if (!gladLoadGL(glfwGetProcAddress)) {
        LOG_ERROR("Failed to initialize glad!");
        glfwTerminate();
        return -1;
    }
    LOG_INFO("glad initialized successfully!");
    
    // 输出OpenGL版本
    LOG_INFO("OpenGL version: {}", (const char*)glGetString(GL_VERSION));
    
    // 初始化输入处理器
    LOG_INFO("Initializing InputHandler...");
    InputHandler::initialize(window);
    LOG_INFO("InputHandler initialized successfully!");
    
    // 初始化渲染器
    LOG_INFO("Initializing Renderer...");
    Renderer::initialize(window);
    LOG_INFO("Renderer initialized successfully!");
    
    // 注册命令输入回调
    LOG_INFO("Registering command input callback...");
    InputHandler::registerCallback(InputEventType::COMMAND_ENTERED, []() {
        std::string command = InputHandler::getCommandInput();
        LOG_INFO("Executing command: {}", command);
        CommandParser::parseCommand(command);
    });
    LOG_INFO("Callback registered successfully!");
    
    // 主循环
    LOG_INFO("Entering main loop...");
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
    LOG_INFO("Cleaning up resources...");
    Renderer::cleanup();
    
    // 销毁窗口
    LOG_INFO("Destroying window...");
    glfwDestroyWindow(window);
    
    // 终止GLFW
    LOG_INFO("Terminating GLFW...");
    glfwTerminate();
    
    LOG_INFO("CadToy exited successfully!");
    return 0;
}