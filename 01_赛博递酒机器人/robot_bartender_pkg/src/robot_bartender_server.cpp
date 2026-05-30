
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

        if(target_x <= 10 and target_y <= 10){
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

        bool can_get_goal = true;
        //创建一个控制目标读取的bool

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

            if(robot_status == Idle and can_get_goal){
                can_get_goal = false;
                //先将读取模式关闭
                auto goal_data = goal_queue_.front();
                //将队列首位塞入变量
                //提取goal信息
                int target_x = goal_data->target_x;
                int target_y = goal_data->target_y;
                std::string item_name = goal_data->item_name;
                RCLCPP_INFO(this->get_logger(), "成功接到目标，目标信息：[ %s ]( %i , %i )",
                                                            item_name.c_str(), target_x,target_y);

                auto goal_data = goal_queue_.pop();
                //提取数据后删除
            }
            
            if( ! robot_status == Charging){
                //工作模式

                //判断当前状态
                robot_status = (remaining_power <= 20) ? LowBattery : Delivering;
                //电量小于等于20时，状态为LowBattery模式
                feedback->current_mode = (remaining_power <= 20) ? "LowBattery" : "Delivering";
                //发送当前状态

                target_x = (robot_status == Delivering) ? target_x : 0;
                target_y = (robot_status == Delivering) ? target_y : 0;
                //机器人状态为delivering时，目标坐标为goal中的坐标，否则为0点
            
                //运动算法
                double dx = std::fabs(target_x - current_x); //剩余距离
                double dy = std::fabs(target_y - current_y);
                //取绝对值，为了求平方根做准备

                double distance =std::sqrt( dx*dx + dy*dy); //斜边长度
                double step = (robot_status = LowBattery) ? 3.0 : 1.0;

                if(distance <= step){
                    current_x = target_x;
                    current_y = target_y;
                    //防止走过头
                    //抵达目标点

                    robot_status = (robot_status == LowBattery) ? Charging : Delivering;
                    //如果是低电量模式到达了目标点，则将状态改为充电模式
                    feedback->current_mode = (robot_status == LowBattery) ? "Charging" : "Delivering";
                    RCLCPP_INFO(this->get_logger(), "当前模式为 %s ",feedback->current_mode.c_str());
                    if(robot_status == Delivering){ //送货成功
                        RCLCPP_INFO(this->get_logger(), "成功将酒送到指定位置");
                        is_finish = true;
                        robot_status == Idle; //将状态设置为空闲
                        can_get_goal = true; //可以获取下一个目标了
                        RCLCPP_INFO(this->get_logger(), "准备接取下一个目标");
                        result->total_duration += total_duration;
                        result->is_finish = true;
                    }
                }else{
                    current_x = (robot_status == Delivering) ? 
                    current_x + (step/distance) * dx : current_x - (step/distance) * dx;
                    //如果以0点为目标，则当前坐标大于目标坐标，所以需要判断加减
                    current_y = (robot_status == Delivering) ? 
                    current_y + (step/distance) * dy : current_y - (step/distance) * dy;
                }
                //在运动模块结束后回报当前状态
                feedback->current_x = current_x;
                feedback->current_y = current_y;
                feedback->remaining_power = remaining_power;

            }else{
                //充电模式
            }

            //这一步走完后，电量-5% ，时间+1
            remaining_power -= 5;
            total_duration += 1;
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