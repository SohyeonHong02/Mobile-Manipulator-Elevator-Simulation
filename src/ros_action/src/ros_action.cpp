#include "ros/ros.h"
#include "std_msgs/String.h"
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <geometry_msgs/TransformStamped.h>
#include <nav_msgs/Path.h>
#include "actionlib/server/simple_action_server.h"
#include "ros_action/ros_actionAction.h"
#include <vector>
using namespace std;

class RosAction
{
protected:
  ros::NodeHandle nh_;
  actionlib::SimpleActionServer<ros_action::ros_actionAction> as_;

  std::string action_name_;
  ros_action::ros_actionFeedback feedback_;
  ros_action::ros_actionResult result_;

public:
  RosAction();
  RosAction(std::string name) :
    as_(nh_, name, boost::bind(&RosAction::executeCB, this, _1), false),
    action_name_(name)
    {
      ROS_INFO("Start initializtion");
      pose_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/ee_pose_desired", 1);
      trajectory_pub_ = nh_.advertise<nav_msgs::Path>("/trajectory", 1);
      pose_sub_ = nh_.subscribe<geometry_msgs::PoseStamped>("/cartesian_pose_controller/ee_state", 1, &RosAction::cartesianPoseCallBack, this);
      twist_sub_ = nh_.subscribe<geometry_msgs::TwistStamped>("/cartesian_pose_controller/ee_state_dot", 1, &RosAction::cartesianTwistCallBack, this);
      trajectory_cur_.header.frame_id = "ur5e_mount_link";
      as_.start();
      ROS_INFO("Finish initialization");
    }
  ~RosAction(void)
  {

  }
  void cartesianPoseCallBack(const geometry_msgs::PoseStampedConstPtr &msg);
  void cartesianTwistCallBack(const geometry_msgs::TwistStampedConstPtr &msg);
  bool comparePosition(geometry_msgs::PoseStamped &pose_tar, geometry_msgs::PoseStamped &pose_cur);
  void executeCB(const ros_action::ros_actionGoalConstPtr &goal)
  {
    // received request from action client
    // initialization
    ros::Rate loop_rate(1000);
    bool success = true;
    feedback_.x = 0;
    feedback_.y = 0;
    feedback_.z = 0;

    // get target position from ros action goal
    geometry_msgs::PoseStamped pose_tar;
    double execute_duration = 5.0; // execute interpolation for 5 seconds
    pose_tar.header.frame_id = "ee_link";
    pose_tar.header.stamp = ros::Time::now() + ros::Duration(execute_duration);
    // set position and orientation information
    pose_tar.pose.position.x = goal->x;
    pose_tar.pose.position.y = goal->y;
    pose_tar.pose.position.z = goal->z;
    pose_tar.pose.orientation.w = 1;

    // set time and position for interpolation
    ros::Time time_start = ros::Time::now();
    ros::Time time_end = pose_tar.header.stamp;
    geometry_msgs::PoseStamped pose_start = pose_cur_;
    geometry_msgs::TwistStamped twist_start = twist_cur_;
    geometry_msgs::PoseStamped pose_goal; // ros message that will be sent to cartesian pose controller
    pose_goal.pose.orientation = pose_cur_.pose.orientation; // maintain its orientation
    double x_start = pose_start.pose.position.x, y_start = pose_start.pose.position.y, z_start = pose_start.pose.position.z;
    double x_dot_start = twist_start.twist.linear.x, y_dot_start = twist_start.twist.linear.y, z_dot_start = twist_start.twist.linear.z;
    double x_end = pose_tar.pose.position.x, y_end = pose_tar.pose.position.y, z_end = pose_tar.pose.position.z;
    ROS_INFO("start time %f, end time: %f, duration: %f", time_start.toSec(), time_end.toSec(), time_end.toSec() - time_start.toSec());
    ROS_INFO("Current position of the end_effector: x = %f, y = %f, z = %f", pose_cur_.pose.position.x, pose_cur_.pose.position.y, pose_cur_.pose.position.z);
    ROS_INFO("Target position of the end_effector: x = %f, y = %f, z = %f", pose_tar.pose.position.x, pose_tar.pose.position.y, pose_tar.pose.position.z);
    ROS_INFO("Execute callback function for interpolating");
    // loop for request
    while (!comparePosition(pose_tar, pose_cur_))
    {
        ros::Time time_cur = ros::Time::now();
        if ((time_cur.toSec() - time_start.toSec()) <= 0) // if current time is earlier than start time
        {
            pose_goal = pose_cur_;
        }
        
        else if ((time_cur.toSec() - time_end.toSec()) >= 0) // or if interpolate is done
        {
            pose_goal = pose_tar;
            pose_pub_.publish(pose_goal); // publish command
            break; // finish the loop
        }
        
        else
        {
            double x_goal = x_start + x_dot_start * (time_cur.toSec() - time_start.toSec())
                                    + (3.0 * (x_end - x_start) / (std::pow((time_end.toSec() - time_start.toSec()), 2))
                                    - 2.0 * x_dot_start / (time_end.toSec() - time_start.toSec())) * std::pow((time_cur.toSec() - time_start.toSec()), 2)
                                    + (-2.0 * (x_end - x_start) / (std::pow((time_end.toSec() - time_start.toSec()), 3))
                                    + x_dot_start / (std::pow((time_end.toSec() - time_start.toSec()), 2))) * std::pow((time_cur.toSec() - time_start.toSec()), 3);
            double y_goal = y_start + y_dot_start * (time_cur.toSec() - time_start.toSec())
                                    + (3.0 * (y_end - y_start) / (std::pow((time_end.toSec() - time_start.toSec()), 2))
                                    - 2.0 * y_dot_start / (time_end.toSec() - time_start.toSec())) * std::pow((time_cur.toSec() - time_start.toSec()), 2)
                                    + (-2.0 * (y_end - y_start) / (std::pow((time_end.toSec() - time_start.toSec()), 3))
                                    + y_dot_start / (std::pow((time_end.toSec() - time_start.toSec()), 2))) * std::pow((time_cur.toSec() - time_start.toSec()), 3);
            double z_goal = z_start + z_dot_start * (time_cur.toSec() - time_start.toSec())
                                    + (3.0 * (z_end - z_start) / (std::pow((time_end.toSec() - time_start.toSec()), 2))
                                    - 2.0 * z_dot_start / (time_end.toSec() - time_start.toSec())) * std::pow((time_cur.toSec() - time_start.toSec()), 2)
                                    + (-2.0 * (z_end - z_start) / (std::pow((time_end.toSec() - time_start.toSec()), 3))
                                    + z_dot_start / (std::pow((time_end.toSec() - time_start.toSec()), 2))) * std::pow((time_cur.toSec() - time_start.toSec()), 3);
            pose_goal.pose.position.x = x_goal;
            pose_goal.pose.position.y = y_goal;
            pose_goal.pose.position.z = z_goal;
            trajectory_cur_.poses.push_back(pose_goal);
            // cout<<"<"<< pose_goal.pose.position.x << "><" << pose_goal.pose.position.y << "><" << pose_goal.pose.position.z << ">\n";
        }
        //ROS_INFO("Test");

        pose_pub_.publish(pose_goal);
        loop_rate.sleep();
    }
    trajectory_pub_.publish(trajectory_cur_);
    // if finished, rend success message to action client
    ROS_INFO("Finish the interpolation of the manipulator");
    
    if(success)
    {
        result_.success = true;
        ROS_INFO("result_success_TRUE");
        ROS_INFO("Current position of the end_effector: x = %f, y = %f, z = %f", pose_cur_.pose.position.x, pose_cur_.pose.position.y, pose_cur_.pose.position.z);
        as_.setSucceeded(result_);
    }
    
  }

private:
  ros::Publisher pose_pub_, trajectory_pub_; // cartesian pose publisher for controller
  ros::Subscriber pose_sub_, twist_sub_; // cartesian pose subscriber

  geometry_msgs::PoseStamped pose_cur_; // current cartesian pose
  geometry_msgs::TwistStamped twist_cur_; // current cartesian twist
  nav_msgs::Path trajectory_cur_; // current trajectory
};

bool RosAction::comparePosition(geometry_msgs::PoseStamped &pose_tar, geometry_msgs::PoseStamped &pose_cur)
{
    if (pose_tar.pose.position.x != pose_cur.pose.position.x) return false;
    if (pose_tar.pose.position.y != pose_cur.pose.position.y) return false;
    if (pose_tar.pose.position.z != pose_cur.pose.position.z) return false;
    return true;
}

void RosAction::cartesianPoseCallBack(const geometry_msgs::PoseStampedConstPtr &msg)
{
    geometry_msgs::PoseStamped pose_cur = *msg;
    pose_cur_ = pose_cur;
    //cout << pose_cur << endl;
    return;
}

void RosAction::cartesianTwistCallBack(const geometry_msgs::TwistStampedConstPtr &msg)
{
    geometry_msgs::TwistStamped twist_cur = *msg;
    twist_cur_ = twist_cur;
    //cout << pose_cur << endl;
    return;
}

class tf_Box
{
private:
    string _frame_id;
    string _child_frame_id;
    double _x1, _y1, _z1;
public:
    tf_Box(){};
    ~tf_Box(){};
    string pull_child_id(void){return _child_frame_id;}
    void Set_i(double a, double b, double c){ 
        _x1 = a; _y1 = b; _z1 = c;
    }
    double pull_x1(void){return _x1;};
    double pull_y1(void){return _y1;};
    double pull_z1(void){return _z1;};


};

int box_count;
vector<tf_Box> bbox_t;


/*
void SubscribeAndPublish2::xyz_callback(const geometry_msgs::PoseStampedConstPtr& desired_coor)
{
    for(auto i = present_coor->pose; i < desired_coor->pose; i)


}
*/

int main(int argc, char **argv){
    ros::init(argc, argv, "ros_action");
    RosAction server("interpolation");
    ros::spin();

    return 0;


}
