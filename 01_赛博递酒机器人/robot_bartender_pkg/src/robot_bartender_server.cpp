
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "robot_interface/action/robot_msg.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

using RobotMsg = robot_interface::action::RobotMsg;
using GoalHandleRobotMsg = rclcpp_action::ServerGoalHandle<RobotMsg>;

class RobotBartenderNode : public rclcpp::Node{
private:
    rclcpp_action::Server<RobotMsg>::SharedPtr server_;

    rclcpp_action::GoalResponse handle_goal(
        const rclcpp_action::GoalUUID & uuid,
        std::shared_ptr<const RobotMsg::Goal> goal
    ){
        int target_coordinate[] = {goal->target[]}
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