
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/rclcpp_action"
//需要用到的msg

using //将msg的类型重命名
using //将goalhandel重命名 

class RobotBartenderNode() : public rclcpp::Node(){
private:
//实例化一个action目标句柄


public:
    RobotBartenderNode() : Node(){
        //先设定sever端口

        //使用lambda表达式关联3个回调函数
        //1，判断是否接取任务并回报结果
        //2，接收到停止命令的动作
        //3，正式的送酒动作

    }

    //启动送酒函数具体实现逻辑
    //使用thread绑定另一个函数
    //函数结束，不堵塞

    //真正的送酒函数
    //实例化需要的数值与变量
    //进入到while循环，循环结束条件为bool'工作结束'
    //第一步先关注有没有发送取消指令
    //如果有->跳出while循环
    //状态机1：空闲状态

};