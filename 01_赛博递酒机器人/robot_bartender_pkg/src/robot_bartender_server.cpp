
#include <string>
#include <thread>
#include <queue>

#include "rclcpp/rclcpp.hpp"
#include "robot_interface/action/robot_msg.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

using RobotMsg = robot_interface::action::RobotMsg;
using GoalHandleRobotMsg = rclcpp_action::ServerGoalHandle<RobotMsg>;

class RobotBartenderNode : public rclcpp::Node{
private:
    rclcpp_action::Server<RobotMsg>::SharedPtr server_;
    //尝试使用队列来存储多个目标
    std::queue<std::shared_ptr<RobotMsg::Goal>> goal_queue_;

    rclcpp_action::GoalResponse handle_goal(
        const rclcpp_action::GoalUUID & uuid,
        std::shared_ptr<const RobotMsg::Goal> goal
    ){
        int target_x = goal->target_x;
        int target_y = goal->target_y;
        std::string item_name = goal->item_name;
        //获取目标信息

        if(target_x <= 15 and target_y <= 15){
            RCLCPP_INFO(this->get_logger(), 
            "收到命令！目标位置：[ %i , %i ],需要运送的物品：%s",
            target_x,target_y,item_name.c_str());
            return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        }else{
            RCLCPP_INFO(this->get_logger(), "坐标无效，请重新输入");
            return rclcpp_action::GoalResponse::REJECT;
        }

    }

    rclcpp_action::CancelResponse handle_cancel(
		const std::shared_ptr<GoalHandleCleanShrine> goal_handle 
	){
    	RCLCPP_INFO(this->get_logger(), "收到指令，回到吧台，清除所有任务");
    	return rclcpp_action::CancelResponse::ACCEPT; // 同意取消
	}
public:
    RobotBartenderNode() : Node("robot_bartender_node"){
        server_ = rclcpp_action::create_server<RobotMsg>(
            this,
            "robot_bartender",

            [this](const auto& uuid ,const auto& goal){
                return this->handle_goal(uuid,goal);
            },

            [this](const auto& goal_handle) {
				return this->handle_cancel(goal_handle);
			},
			
			[this](const auto& goal_handle) {
				this->handle_accepted(goal_handle);
            }
        );

        RCLCPP_INFO(this->get_logger(), "酒保机器人已启动");
    }

} ;