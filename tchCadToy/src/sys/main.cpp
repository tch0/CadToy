// 对应头文件

// C++ 标准库

// 第三方库
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

// 项目头文件
#include "CommandManager.h"
#include "Logger.h"
#include "InputHandler.h"
#include "Renderer.h"
#include "Global.h"


using namespace tch;

int main(int argc, char* argv[])
{
    // 系统初始化
    checkOS();
    checkSystemEndian();
    buildCwd(argv[0]);
    checkAndCreateImportantDirs();

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
        glfwTerminate();
        return -1;
    }
    LOG_INFO("GLFW initialized successfully!");
    
    // OpenGL版本3.3，核心模式：移除老式的废弃的OpenGL函数
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
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
    
    // 启用V-Sync(垂直同步)，强制渲染循环与显示器的刷新率同步
    glfwSwapInterval(1);
    
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
    
    // 初始化文件对话框的纹理相关函数(如果调用了ImFileDialog，就必须初始化，否则运行时会调用空函数指针导致崩溃)
    LOG_INFO("Initializing FileDialog...");
    initializeImFileDialog();
    LOG_INFO("FileDialog initialized successfully!");
    
    // 主循环
    LOG_INFO("Entering main loop...");
    while (!glfwWindowShouldClose(window)) {
        // 处理事件
        glfwPollEvents();
        
        // 开始渲染
        Renderer::beginRender();
        
        // 运行命令循环
        // 命令中也会需要打开ImGui窗口，所以命令循环必须放在beginRender(其中调用ImGui::NewFrame())之后
        CommandManager::getInstance().runCommandLoop();
        
        // 绘制菜单栏
        Renderer::drawMenuBar();
        
        // 绘制文件栏
        Renderer::drawFileBar();
        
        // 绘制所有图形
        Renderer::drawAll();
        
        // 绘制命令栏
        Renderer::drawCommandBar();
        
        // 绘制属性栏
        Renderer::drawPropertyBar();
        
        // 绘制状态栏
        Renderer::drawStatusBar();
        
        // 绘制模态对话框
        Renderer::drawModalDialogs();
        
        // 绘制非模态窗口
        Renderer::drawNonModalWindows();
        
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