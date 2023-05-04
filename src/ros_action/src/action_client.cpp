#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <ros_action/ros_actionAction.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  ros::init(argc, argv, "action_client_node");
  if (argc != 4) {ROS_ERROR("usage: x y z"); return false;}
  actionlib::SimpleActionClient<ros_action::ros_actionAction> ac("interpolation", true);
  // send action request to action server with target point
  ros_action::ros_actionGoal action_goal; // initialize a action goal
  action_goal.x =  atof(argv[1]);
  action_goal.y = atof(argv[2]);
  action_goal.z = atof(argv[3]);

  ac.sendGoal(action_goal); // send action to action server
  // wait for 10 seconds until the action return
  bool finished_before_timeout = ac.waitForResult(ros::Duration(10.0));

  if (finished_before_timeout)
  {
    actionlib::SimpleClientGoalState gs = ac.getState();
    ROS_INFO("Task finished: %s", gs.toString().c_str());
  }
  else
    ROS_WARN("Task did not finished before the time out!");
}