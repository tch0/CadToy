#pragma once

// C++ 标准库

// 第三方库

// 项目头文件

namespace tch {

// 命令基类
class Command {
protected:
    // 完成标志
    bool m_completed;

public:
    Command() : m_completed(false) {}
    virtual ~Command() = default;
    
    /**
    每一帧调用，进入此方法更新命令状态，具体命令类中重写
    注意：这个函数必须要是可重入的
        多次调用的结果应该是配合交互完成命令，而不能因为多次调用产生副作用
        在等待交互、或者命令已经完成的情况下，调用这个函数不应该修改任何状态
    标准实现：
        函数开头判断是否已经结束，结束则返回
            if (isCompleted()) {
                return;
            }
        无论何种方式结束命令，finish都必须得到调用
        函数内部不需要状态切换、不需要进行交互时：
            doYourThings();
            finish();
        命令包含交互逻辑时，需要自己管理状态切换：
            需要跨状态传递的信息全部使用成员变量保存
            注意用于等待交互输入的状态在没有输入时必须无任何副作用，状态开始就查询交互输入，如果是kNone就立刻返回，其他输入则继续执行命令流程。
            有副作用不可多次调用的状态执行后状态即应该完成命令切换。
            命令完成状态必须单独定义为一个状态，取消、完成命令可以都进入这个状态，这个状态末尾必须调用finish()。
    */
    virtual void onUpdate() = 0;
    
    // 命令是否完成
    bool isCompleted() const;
    
    // 宣告命令结束，每个命令在完成所有流程后都应该调用此方法，允许多次重复调用，无任何副作用
    void finish();
    
    // 绘制预览（基类方法，派生类可重写）
    virtual void drawPreview() {} 
};

} // namespace tch
