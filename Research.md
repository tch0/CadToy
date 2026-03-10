# CAD程序研究

## 功能列表：已实现与Todo

已实现：
- 主循环
- 日志系统
- 各种界面元素：菜单栏、文件栏、命令栏、属性栏、状态栏
- 画布实现、栅格坐标轴光标绘制
- 各种坐标变换研究与封装：世界坐标系到屏幕、屏幕到世界、视口裁剪等
- 文件管理实现部分
- Options对话框
- 画布与命令栏上的键盘输入导入到命令栏输入框中提交执行
- 注册与识别快捷键
- 不同文档的渲染相关数据保存在在文档中
- 粗糙实现命令执行的基本流程
- 完善命令执行的基本框架
    - 数值、字符串、点输入、分支选择实现
    - 清晰输入处理的职责：
        - InputHandler负责处理所有底层事件：按键、鼠标
        - 遇到字符/BackSpace则转发到Renderer输入框中，Renderer输入框仅处理字符编辑操作
        - Enter/Space/Esc时Renderer将整个字符串作为结果发送到InputContext中处理，命令执行过程中的输入由InputContext处理后保存为自身状态，等待命令执行过程来获取
        - 非命令执行过程中则解析成命令，交由CommandManager解析执行
- 窗口焦点管理：
    - 画布上进行键盘输入时焦点自动切换到命令输入框进行输入交互
    - 实现鼠标移出非模态窗口就自动失去焦点的功能，以实现只要鼠标在画布上就能进行输入（而不是需要点击一下画布将焦点从某个窗口释放才能输入，这种操作会破坏CAD的交互逻辑，因为点击画布是点输入的一种交互）

Todo：
- 取消当前命令，执行其他命令或关闭文档之类操作，流程合理化
- 命令执行框架中加入对透明命令的支持
- 点输入的完整实现：点击屏幕、距离输入、点坐标输入
- 命令简写、快捷键解析为具体的命令，命令框架中引入命令映射表，命令补全，通过命令映射表来查找执行命令而非通过硬编码
- 实体实现，跑通文件流读取保存的流程，探索数据格式设计，以Line为例
- 坐标拾取实现（通过屏幕点击或者坐标输入）
- drag过程实现，以Line为例
- 跑通Line命令
- 图形引擎实现，将复杂实体拆分成简单的片元，打包渲染
- 多文档渲染到多个FBO
- Imgui切换到Docking分支，支持将文档拖出
- 常见实体创建命令实现：xline、pline、circle、arc、rect等
- 捕捉实现、使用空间四叉树/R树来实现
- 选择集研究与实现
- 研究几何库、封装几何操作
- 几何编辑框架与相关命令研究与实现
- 快捷键：
    - F1帮助
    - F2打开新文本窗口
- 命令历史使用InputMultiText实现，使命令历史可选中


## BUG

已修复BUG：

发现的未修复BUG：
- 切换文档后非命令栏输入的第一个字符会被吃掉
    - 因为切换时取消命令，置s_bNeedClearCommandBuffer为true后丢失了输入框丢失焦点没有进入命令输入的回调，下一帧切过去了输入后重新获得焦点才在回调中清空buffer
    - 立即模式下很多东西通过状态、回调来处理，帧与帧之间状态切换没管理好导致的问题，很微妙
    - 切文档看起来需要系统性梳理并重新设置很多状态
    - 暂不修复，命令取消逻辑会移动到输入上下文中系统处理，届时一起处理，现在的逻辑是临时的，没必要去改

## 非模态命令交互研究

在命令的交互过程中，核心交互是拾取点、获取关键字、获取整数浮点数字符串这类非模态交互：
- 获取输入过程中，每一帧的绘图不会停止，命令却是停止运行的。
- 获取到输入后，绘图同样继续进行，命令继续执行。

实现方案1：
- 将命令实现为状态机，将命令分割为一系列由交互隔离的状态：
```C++
class LineCommand : public IncrementalCommand {
    int stage = 0;
    Vec2 p1, p2;

    void OnUpdate(InputContext& ictx) override {
        switch (stage) {
            case 0:
                ictx.SetPrompt("指定第一点:");
                if (ictx.GetPickedPoint(p1)) stage = 1;
                break;
            case 1:
                ictx.SetPrompt("指定下一点:");
                ictx.DrawRubberBand(p1); // 在渲染层画预览线
                if (ictx.GetPickedPoint(p2)) {
                    SubmitLine(p1, p2);
                    p1 = p2; // 循环
                }
                break;
        }
    }
};
```
- 在主循环中，每一帧都会调用命令的update，去检测是否获取到了输入并更新命令的状态。

实现方案2：
- 使用C++20的无栈协程来完成异步逻辑，将命令逻辑和获取输入的逻辑置于不同协程内。
```C++
Task Command_Line(CadContext& ctx) {
    // 挂起并等待输入，主循环每帧会 Check 状态
    Vec2 p1 = co_await ctx.GetPoint("指定第一点");
    
    while (true) {
        // 这里的 p2 在用户移动鼠标时会提供“预览点”
        Vec2 p2 = co_await ctx.GetPoint("指定下一点", p1); // 传入 p1 用于画橡皮筋预览线
        ctx.Database.AddLine(p1, p2);
        p1 = p2; // 连续画线逻辑
    }
}
```
- 问题：C++20有栈协程是侵入式的，具有传染性，命令本身、一系列交互函数以及调用这些的入口都需要是协程。

实现方案3：
- 使用有栈协程，比如Windows的Fiber、Boost.context等。
```C++
Vec2 GetPoint(const char* prompt) {
    g_CommandContext.SetPrompt(prompt);
    
    // 关键：挂起当前命令协程，回到主线程渲染
    g_CommandContext.YieldToMain(); 
    
    // 当主线程检测到输入并 Resume 后，代码从这里继续执行
    return g_CommandContext.GetLastPickedPoint();
}

void LineCommand::Execute() {
    Vec2 p1 = GetPoint("指定第一点:");
    while(true) {
        // 第二次调用 GetPoint 时，主线程会不断更新“预览线”
        Vec2 p2 = GetPoint("指定下一点:"); 
        AddLineToDatabase(p1, p2);
        p1 = p2;
    }
}
```
- 优点：非侵入式、和旧标准代码完美协作。

实现考虑：
- C++20无栈协程侵入性和传染性太强，对架构影响非常大，这里暂时不考虑。
- 先实现为状态机，后续如果合适可以切换为有栈协程。

## 功能实现顺序

AI生成的功能实现参考。

### 第一阶段：核心架构、数据逻辑分离

- 1、几何数据模型（Database/DOM）：
    - 建立一个 Document 类，持有所有的几何图元（Line, Circle, Arc...）。
    - 关键点：使用双精度浮点数 double 或高精度定点数处理坐标，千万不要用单精度 float，否则缩放时会出现严重的精度抖动。
- 2、命令系统（Command Pattern）
    - 每一个修改文档的操作都封装成一个 Command 对象。
    - 实现 UndoStack（撤销栈）。这是 CAD 的命脉，后期补救非常困难
- 3、坐标转换引擎（Transformer）：
    - 实现核心函数：WorldToScreen() 和 ScreenToWorld()。
    - 管理 Camera（持有 Pan 偏移和 Zoom 缩放倍率）

### 第二阶段：渲染管线、画布集成

- 1、FBO视口集成
    - 不要直接在窗口绘图。将几何图形渲染到 OpenGL FBO (Framebuffer Object)，然后通过 ImGui::Image() 显示。
    - 这样你可以轻松实现多文档对比、视口缩放而不干扰 ImGui UI。
- 2、即时绘制（Immediate Layers）：
    - 区分“静态数据渲染”（已保存的线段）和“动态层渲染”（鼠标正在拉出的虚线、捕捉提示）。
    - 利用 ImGui::GetWindowDrawList() 快速实现动态反馈。

### 第三阶段：交互框架、状态机

- 这是 CAD 程序最复杂的部分：非模态交互。
- 1、输入解析器（Tokenizer）：
    - 解析 InputText 传来的字符串（如 L 10,20）
- 2、交互状态机：
    - CAD 处于不同状态：Idle（选择模式）、DrawingLine（等待第一个点）、DrawingLine_Step2（等待第二个点）
    - 输入优先级：当用户按下回车或鼠标点击时，逻辑先发给 ActiveTool。如果 ActiveTool 为空，再发给全局命令解析器。
- 3、坐标捕捉系统（Snapping - 简化版）：
    - 先实现 网格捕捉（Grid Snap）：将鼠标坐标 round 到最近的网格点。这是后续实现复杂对象捕捉（端点、中点）的基础。

### 第四阶段：依次完善功能

- 按照顺序，依次实现

|顺序|	功能模块|	目标|
|:-:|:-:|:-
|1|	视图操作|	鼠标中键平移、滚轮缩放 FBO 内的内容（确保坐标转换无误）。|
|2|	基础解析器|	能够识别 LINE 命令并进入“取点状态”。|
|3|	点输入系统|	支持两种方式取点：鼠标点击、命令行输入 x,y 坐标。|
|4|	图元生成|	执行命令后，真正将 Line 对象加入 Document 并渲染出来。|
|5|	撤销|/重做	确保每画一根线都能被撤销，保证数据一致性。|
|6|	多文档切换|	实现 Tab 或多窗口，验证 Document 类的解耦是否彻底。|
|7|	对象捕捉|	实现端点、交点捕捉，这是从“画图板”进化为“CAD”的标志。|


特别建议：
- 不要在回调里写业务逻辑：InputText 的回调只负责“接收输入”和“清理缓冲区”。真正的绘图逻辑应该由一个 CommandDispatcher 在主循环的 Update 阶段处理。
- 处理 Esc 键：CAD 用户极其依赖 Esc。确保你的状态机能随时响应 Esc 并重置当前所有未完成的命令，同时通过 g_needs_clear 清空输入框。
- 坐标系习惯：OpenGL 默认是Y轴向上，而窗口坐标（ImGui）是Y轴向下。在底层逻辑中，建议统一使用数学笛卡尔坐标系（Y轴向上），仅在最后的渲染阶段进行反转。

## 透明命令

- ZOOM、PAN、GRID 这种不修改几何数据、只改变观察状态的操作，是不应该中断当前正在进行的绘图（如 LINE）或修改（如 MOVE）命令的

三个核心层级的逻辑隔离：
- 为了支持透明操作，主循环输入处理逻辑应该按以下优先级排序：
- 1、系统级/视图级（透明层）
    - 鼠标滚轮/中键：直接修改 Camera 对象的 Zoom 和 Offset
    - 快捷键（如 F7 切换网格）：直接修改全局渲染标志
    - 特点：执行完后，不重置当前命令状态。
- 2、 命令解析层（指令层）
    - InputText 回车：解析字符串。如果是 ZOOM 或 PAN 文本命令，修改 Camera；如果是 LINE，启动状态机。
- 3、当前活动命令层（业务层）
    - 如果当前有一个 ActiveCommand（比如正在画线，已点下第一点），它会“监听”鼠标左键点击。
    - 特点：直到命令完成或按 Esc 才会释放

状态机（State Machine）的设计建议：
- 命令内部采用状态驱动：
- Idle 状态：没有命令执行。命令行输入 LINE -> 实例化 LineTool
- Active 状态：
    - LineTool 内部维护一个子状态（Step 0: 等待起点, Step 1: 等待终点）。
    - 透明性体现：当用户在 Step 1 时滚动鼠标中键，Camera 变了，但 LineTool 依然停留在 Step 1。

CAD框架关键技术点：
- 坐标系解耦：严格区分三种坐标，写好转换函数
    - World Space (世界坐标)：你的几何数据存储位(doudle,doule)
    - Canvas Space (画布/FBO 坐标)：相对于 FBO 左上角的像素坐标。
    - Screen Space (窗口坐标)：ImGui 的全局坐标（用于处理 InputText 和菜单）。
    - world --------Camera------> Canvas -------ImGuiWindowPos-------> Screen
- 渲染分层：
    - 静态层：渲染 Document 中的所有线段。
    - 橡皮筋层 (Rubber Banding)：如果正在画线，实时根据鼠标当前位置画出一根“虚线”到起点。这一步直接在 ImGui::Image() 之后的 ImDrawList 上画，不要去修改 FBO。

