#include "ros/ros.h"
#include "std_msgs/String.h"
#include <darknet_ros_msgs/BoundingBoxes.h>
#include <darknet_ros_msgs/BoundingBox.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <geometry_msgs/PoseStamped.h>
#include <gazebo_msgs/ModelStates.h>
#include <cv_bridge/cv_bridge.h>
#include <vector>
#include <opencv2/highgui/highgui.hpp>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <ros_action/ros_actionAction.h>
#define floor 2
using namespace std;

geometry_msgs::PoseStamped publish_msg (float m1, float m2, float m3);
cv::Point3f Deproject_2D_to_3D(cv::Point2f pixel, float depth);
cv::Point2f yolo_box;
geometry_msgs::PoseStamped m1;


struct quarternion
{
  double w;
  double x;
  double y;
  double z;
};
quarternion robot_quar;


//class
class Box{
protected:
  double x1, x2, y1, y2;
  double size;
  double Gx,Gy;
  double pp;
  string name;
  int bid;
  int valid;
public:
  Box(){};
  ~Box(){};
  void Set_i(string s,double p, double a, double b, double c, double d);
  void Set_id(int t){bid=t;}
  void Get_boundingbox_i(void);
  void Set_valid(void);
  void Test_print(void);
  int pull_bid(void){return bid;}
  double pull_pp(void){return pp;}
  int pull_valid(void){return valid;}
  double pull_Gx(void){return Gx;}
  double pull_Gy(void){return Gy;}
  string pull_name(void){return name;}
};

// class RosAction
// {
//   protected:
//   ros::NodeHandle nh_;
//   actionlib::SimpleActionServer<box_coordiantes::RosAction> as_;

//   std::string action_name_;
//   box_coordinates::RosActionFeedback feedback_;
//   box_coordinates::RosActionResult result_;
//   public:
//   RosAction(std::string name) :
//     as_(nh_, name, boost::bind(&RosAction::executeCB, this, _1), false),
//     action_name_(name)
//     {
//       as_.start();
//     }
//   ~RosAction(void)
//   {

//   }
//   void executeCB(const box_coordinates::RosActionConstPtr &goal)
//   {
//     //ros::Rate r(1);
//     //bool success = true;
//     //feedback_.sequence.clear();
//     //feedback_.sequence.push_back(0);
//     //feedback_.sequence.push_back(1);

//     //ROS_INFO("%s: Executing, creating rosaction sequence of order %i with seeds %i, %i", action_name_.c_str(), goal->order, feedback_.sequence[0], feedback_.sequence[1]);
//     result_.sequence = 
//   }
// }


class SubscribeAndPublish1
{
public:
  SubscribeAndPublish1() :
  ac_("interpolation", true)
  {
    pub_ = nh.advertise<geometry_msgs::PoseStamped>("/ee_pose_desired", 1000);
    //sub3 = nh.subscribe<gazebo_msgs::ModelStates>("/gazebo/model_states", 1000, &SubscribeAndPublish1::States_callback, this);
    sub1 = nh.subscribe("/darknet_ros/bounding_boxes", 1000, &SubscribeAndPublish1::Yolo_callback, this);
    sub2 = nh.subscribe("/realsense/depth/image_rect_raw", 1000, &SubscribeAndPublish1::Depth_callback,this);
    ROS_INFO("Waiting for server to start...");
    ac_.waitForServer();
    ROS_INFO("Start node");
  }
  void Yolo_callback(const darknet_ros_msgs::BoundingBoxesConstPtr& boundingbox);
  void Depth_callback(const sensor_msgs::ImageConstPtr& img);
  //void States_callback(const gazebo_msgs::ModelStates::ConstPtr& state);
  
private:
  ros::NodeHandle nh;
  ros::Publisher pub_;
  ros::Subscriber sub1, sub2;

  actionlib::SimpleActionClient<ros_action::ros_actionAction> ac_;
};

geometry_msgs::PoseStamped publish_msg(float m1, float m2, float m3)
{
  geometry_msgs::PoseStamped robot_msgt;
  robot_msgt.pose.position.x = m3;
  robot_msgt.pose.position.y = -m1;
  robot_msgt.pose.position.z = -m2+0.5;
  robot_msgt.pose.orientation.w = 1;
  

  return robot_msgt;
}


//variable
int box_count;
string vv;
string tt;
vector<Box> bbox_t;
vector<Box> bbox_v;

//
void Box :: Get_boundingbox_i()
{
  double width, height;
  width=x2-x1;
  height=y2-y1;
  size=width*height;
  Gx=(x1+x2)/2;
  Gy=(y1+y2)/2;
}
void Box :: Test_print(){
  cout<< "name : " << name << ", size : " << size << endl;
  cout<< "probability : "<< pp << endl;
  cout<< "Center of Box : ("<< Gx << ", " << Gy << ")" << endl;
  cout<< "id : "<< bid<<endl;
  //cout<< "valid :"<<valid<<endl;
}
void Box :: Set_i(string s,double p, double a, double b, double c, double d){
  name=s;pp=p; x1=a; x2=b; y1=c; y2=d;
  if(name == "one") Set_id(1);
  else if(name == "two") Set_id(2);
  else if(name == "three") Set_id(3);
  else if(name == "four") Set_id(4);
}

void SubscribeAndPublish1::Yolo_callback(const darknet_ros_msgs::BoundingBoxesConstPtr& boundingbox)
{
  box_count=boundingbox->bounding_boxes.size();
  for(int i=0;i<box_count;i++){
      Box B;
      B.Set_i(boundingbox->bounding_boxes[i].Class, boundingbox->bounding_boxes[i].probability,boundingbox->bounding_boxes[i].xmax,boundingbox->bounding_boxes[i].xmin,boundingbox->bounding_boxes[i].ymax,boundingbox->bounding_boxes[i].ymin);
      B.Get_boundingbox_i();
      //B.Set_valid();
      int k=boundingbox->bounding_boxes[i].id;
      
      bbox_t.push_back(B);
    

  }
  /*
  for(int i=0;i<bbox_t.size();i++){
    bbox_t[i].Test_print();
    cout<<endl;
  }
  */
  
  for(int i=0;i<bbox_t.size();i++){
    if(bbox_t[i].pull_bid()== floor) {yolo_box.x = bbox_t[i].pull_Gx(); yolo_box.y = bbox_t[i].pull_Gy();}
    
  }
  //std::cout<<"yolo_node"<<endl<<yolo_box<<std::endl;




  bbox_t.clear();
  box_count=0;
  cout<<"\033[2J\033[1;1H"; //clear terminal
}

cv::Point3f Deproject_2D_to_3D(cv::Point2f pixel, float depth) {
    double fx, fy, ppx, ppy;
    fx = 337.2084410968044;
    fy = 337.2084410968044;
    ppx = 320.5;
    ppy = 240.5;

    double x_s = (pixel.x - ppx) / fx;
    double y_s = (pixel.y - ppy) / fy;

    cv::Point3f point;
    point.x = depth * x_s;
    point.y = depth * y_s;
    point.z = depth;

    return point;
}

void SubscribeAndPublish1::Depth_callback(const sensor_msgs::ImageConstPtr& img){
  cv_bridge::CvImagePtr img_ptr_depth;
  try
  {
    img_ptr_depth = cv_bridge::toCvCopy(*img, sensor_msgs::image_encodings::TYPE_64FC1);
  }
  catch(cv_bridge::Exception& e){
    ROS_ERROR("cv_bridge exception:  %s", e.what());
    return;
  }

  cv::Mat depth_img = img_ptr_depth->image;
  
  

  //double depth = depth_img.at<u_int16_t>(yolo_box.y,yolo_box.x);
  double depth = depth_img.at<double>(yolo_box.y,yolo_box.x);
  std::cout<<yolo_box << "," << depth <<std::endl;

  cv::Point3f desired_coor;
  desired_coor = Deproject_2D_to_3D(yolo_box, depth);
  if(floor == 1) cout << "------1st floor------" <<endl; 
  else if(floor == 2) cout << "------2nd floor------" <<endl; 
  else if(floor == 3) cout << "------3rd floor------" <<endl; 
  else if(floor == 4) cout << "------4th floor------" <<endl; 
  //cout << "--------------------" <<endl;
  
  /*
  cout << desired_coor.x << endl;
  cout << desired_coor.y << endl;
  cout << desired_coor.z << endl;
  */
  //geometry_msgs::PoseStamped m1;
  /*
  m1.pose.position.x = 0.69;
  m1.pose.position.y = 0;
  m1.pose.position.z = 0.3;
  m1.pose.orientation.w = 1;
  cout << m1 <<endl;
  */
  m1=publish_msg(desired_coor.x, desired_coor.y, desired_coor.z);
  
  cout << m1 << endl;

  // send action request to action server with target point
  ros_action::ros_actionGoal action_goal; // initialize a action goal
  action_goal.x = m1.pose.position.x;
  action_goal.y = m1.pose.position.y;
  action_goal.z = m1.pose.position.z;

  ac_.sendGoal(action_goal); // send action to action server
  // wait for 10 seconds until the action return
  bool finished_before_timeout = ac_.waitForResult(ros::Duration(10.0));

  if (finished_before_timeout)
  {
    actionlib::SimpleClientGoalState gs = ac_.getState();
    ROS_INFO("Task finished: %s", gs.toString().c_str());
  }
  else
    ROS_WARN("Task did not finished before the time out!");

  // pub_.publish(m1);
  //ros::shutdown();
  //




}

/*
void SubscribeAndPublish1::States_callback(const gazebo_msgs::ModelStates::ConstPtr& state)
{
  //robot_quar.w = state->pose[1].orientation.w;
  robot_quar.w = 1;
  //robot_quar.x = state->pose[1].orientation.x;
  //robot_quar.y = state->pose[1].orientation.y;
  //robot_quar.z = state->pose[1].orientation.z;
}
*/
int main(int argc, char **argv)
{
  ros::init(argc, argv, "box_coordinates");
  

  SubscribeAndPublish1 nod;
  
  

  /*
  ros::NodeHandle nh1;
  ros::Publisher pub_1 = nh1.advertise<geometry_msgs::PoseStamped>("/ee_pose_desired", 1000);
  geometry_msgs::PoseStamped m2;
  m2.pose.position.x = 0.75;
  m2.pose.position.y = 0;
  m2.pose.position.z = 0.35;
  m2.pose.orientation.w = 1;
  while(ros::ok())
  {
    pub_1.publish(m2);
    //cout << m2 <<endl;
  }
  */

  
  
  
  //ros::Subscriber sub_1 = nh.subscribe("/darknet_ros/bounding_boxes", 1000, Yolo_callback); //choose [boundingbox_callback_test,Algorithm_main]
  //ros::Subscriber sub_2 = nh.subscribe("/realsense/depth/image_rect_raw", 1000, Depth_callback);
  ros::spin();

  return 0;
}

/*
void Box :: Set_valid(void){
  int v=1;
  if(bid==0){
    if(Gy>300) v=0;
    if(Gx>800||Gx<200) v=0;
  }else if(bid==1){
    if(Gx<500) v=0;
  }
  if(pp<0.921)v=0;
  valid=v;
} //일정 probablity 기준으로 쓸만한 data 거름
*/