
#include <string>
#include <thread>
#include <queue>
#include <cmath> // 引入 std::sqrt 和 std::abs

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

        if(target_x <= 7 and target_y <= 7){
            RCLCPP_INFO(this->get_logger(), 
            "收到命令！目标位置：[ %i , %i ],需要运送的物品：%s",
            target_x,target_y,item_name.c_str());

            goal_queue_.push(goal);//将goal完整的放进容器

            RCLCPP_INFO(this->get_logger(),"已将目标储存进队列中")
            return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        }else{
            RCLCPP_INFO(this->get_logger(), "坐标无效，请重新输入");
            return rclcpp_action::GoalResponse::REJECT;
        }

    }

    rclcpp_action::CancelResponse handle_cancel(
		const std::shared_ptr<GoalHandleRobotMsg> goal_handle 
	){
    	RCLCPP_INFO(this->get_logger(), "收到指令，回到吧台，清除所有任务,进入休眠状态");
        goal_queue_ = queue<std::shared_ptr<RobotMsg::Goal>>();
        return rclcpp_action::CancelResponse::ACCEPT; // 同意取消
	}

    
	void handle_accepted(const std::shared_ptr<GoalHandleRobotMsg> goal_handle){

		std::thread{
			[this,goal_handle](){this->start_delivering_alcohol(goal_handle);}
		}.detach();
		 
	}

    void start_delivering_alcohol(const std::shared_ptr<GoalHandleRobotMsg> goal_handle){
        RCLCPP_INFO(this->get_logger(), "送酒任务开始");

        //实例化feedback信息
        std::string current_mode;
        int current_x = 0;
        int current_y = 0;
        int remaining_power = 100;

        //实例化result信息
        bool is_finish = false;
        int total_duration = 0;

        //使用枚举控制状态机状态
        enum RobotMode{
            Idle,
            Delivering,
            LowBattery,
            Charging
        };

        RobotMode robot_status = Idle;

        //队列不是空，则循环
        while(! goal_queue_.empty()){

            if (goal_handle->is_canceling()) {
                RCLCPP_INFO(this->get_logger(), "收到指令，任务取消，回到充电桩");
                result->is_finish = false;
                result->total_duration = total_duration;
                goal_handle->canceled(result);
                return; 
            }

            auto goal_data = goal_queue_.front();
            //将队列首位塞入变量
            //提取goal信息
            int target_x = goal_data->target_x;
            int target_y = goal_data->target_y;
            std::string item_name = goal_data->item_name;
            
            //判断当前状态
            RobotMode robot_status = (remaining_power <= 20) ? LowBattery : Delivering;
            target_x = (RobotMode robot_status = Delivering) ? target_x : 0;

        }

        //成功完成任务
        if(result->is_finish){
            goal_handle->succeed(result);
            RCLCPP_INFO(this->get_logger(), "成功完成任务，总用时：%i 分钟",total_duration);
        }else{
            goal_handle->abort(result);
            RCLCPP_INFO(this->get_logger(), "任务成功取消，总用时：%i 分钟",total_duration);
        }
    	

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