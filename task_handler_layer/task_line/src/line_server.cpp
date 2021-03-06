// Copyright 2016 AUV-IITK
#include <ros/ros.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Int32.h>
#include <std_msgs/String.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/Bool.h>
#include <actionlib/server/simple_action_server.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <task_commons/lineAction.h>
#include <motion_commons/ForwardAction.h>
#include <motion_commons/SidewardAction.h>
#include <motion_commons/TurnAction.h>
#include <motion_commons/SidewardActionFeedback.h>
#include <motion_commons/ForwardActionFeedback.h>
#include <motion_commons/TurnActionFeedback.h>
#include <motion_commons/SidewardActionResult.h>
#include <motion_commons/ForwardActionResult.h>
#include <motion_commons/TurnActionResult.h>
#include <string>

typedef actionlib::SimpleActionServer<task_commons::lineAction> Server;
typedef actionlib::SimpleActionClient<motion_commons::ForwardAction> Client_Forward;
typedef actionlib::SimpleActionClient<motion_commons::SidewardAction> Client_Sideward;
typedef actionlib::SimpleActionClient<motion_commons::TurnAction> Client_Turn;

class TaskLineInnerClass
{
private:
  ros::NodeHandle nh_;
  Server line_server_;
  std::string action_name_;
  task_commons::lineFeedback feedback_;
  task_commons::lineResult result_;
  ros::Subscriber detection_data;
  ros::Subscriber yaw_sub;
  ros::Subscriber centralize_data;
  ros::Subscriber angle_data;
  ros::Publisher switch_centralize;
  ros::Publisher switch_angle;
  ros::Publisher switch_detection;
  ros::Publisher present_X_;
  ros::Publisher present_Y_;
  ros::Publisher yaw_pub_;
  Client_Forward ForwardClient_;
  Client_Sideward SidewardClient_;
  Client_Turn TurnClient_;
  motion_commons::ForwardGoal forwardgoal;
  motion_commons::SidewardGoal sidewardgoal;
  motion_commons::TurnGoal turngoal;
  std_msgs::Float64 data_X_;
  std_msgs::Float64 data_Y_;
  std_msgs::Float64 angle_goal;
  bool isOrange, success, FrontCenter, SideCenter, LineAlign;

public:
  TaskLineInnerClass(std::string name, std::string node, std::string node1,
                     std::string node2)
    :  // here we are defining the server, third argument is optional
    line_server_(nh_, name, boost::bind(&TaskLineInnerClass::analysisCB, this, _1), false)
    , action_name_(name)
    , ForwardClient_(node)
    , TurnClient_(node1)
    , SidewardClient_(node2)
  {
    ROS_INFO("inside constructor");
    line_server_.registerPreemptCallback(boost::bind(&TaskLineInnerClass::preemptCB, this));
    present_X_ = nh_.advertise<std_msgs::Float64>("/varun/motion/y_distance", 1000);
    present_Y_ = nh_.advertise<std_msgs::Float64>("/varun/motion/x_distance", 1000);
    switch_detection = nh_.advertise<std_msgs::Bool>("line_detection_switch", 1000);
    switch_angle = nh_.advertise<std_msgs::Bool>("line_angle_switch", 1000);
    switch_centralize = nh_.advertise<std_msgs::Bool>("line_centralize_switch", 1000);
    yaw_pub_ = nh_.advertise<std_msgs::Float64>("/varun/motion/yaw", 1000);

    detection_data = nh_.subscribe<std_msgs::Bool>("/varun/ip/line_detection", 1000,
                                                   &TaskLineInnerClass::lineDetectedListener, this);
    yaw_sub = nh_.subscribe<std_msgs::Float64>("/varun/sensors/imu/yaw", 1000, &TaskLineInnerClass::yawCB, this);
    centralize_data = nh_.subscribe<std_msgs::Float64MultiArray>("/varun/ip/line_centralize", 1000,
                                                                 &TaskLineInnerClass::lineCentralizeListener, this);
    angle_data =
        nh_.subscribe<std_msgs::Float64>("/varun/ip/line_angle", 1000, &TaskLineInnerClass::lineAngleListener, this);

    line_server_.start();
  }

  ~TaskLineInnerClass(void)
  {
  }

  void yawCB(std_msgs::Float64 imu_data)
  {
    yaw_pub_.publish(imu_data);
  }

  void lineDetectedListener(std_msgs::Bool msg)
  {
    if (msg.data)
      isOrange = true;
    else
      isOrange = false;
  }

  void lineCentralizeListener(std_msgs::Float64MultiArray array)
  {
    data_X_.data = array.data[0];
    data_Y_.data = array.data[1];
    present_X_.publish(data_X_);
    present_Y_.publish(data_Y_);
  }

  void lineAngleListener(std_msgs::Float64 msg)
  {
    angle_goal.data = msg.data;
  }

  void spinThreadSidewardCamera()
  {
    Client_Sideward &tempSideward = SidewardClient_;
    tempSideward.waitForResult();
    SideCenter = (*(tempSideward.getResult())).Result;
    if (SideCenter)
    {
      ROS_INFO("Bot is at side center");
    }
    else
    {
      ROS_INFO("Bot is not at side center, something went wrong");
      ros::shutdown();
    }
  }

  void spinThreadForwardCamera()
  {
    Client_Forward &tempForward = ForwardClient_;
    tempForward.waitForResult();
    FrontCenter = (*(tempForward.getResult())).Result;
    if (FrontCenter)
    {
      ROS_INFO("Bot is at Front center");
    }
    else
    {
      ROS_INFO("Bot is not at Front center, something went wrong");
      ros::shutdown();
    }
  }

  void spinThreadTurnCamera()
  {
    Client_Turn &tempTurn = TurnClient_;
    tempTurn.waitForResult();
    LineAlign = (*(tempTurn.getResult())).Result;
    if (LineAlign)
    {
      ROS_INFO("Bot is at side center");
    }
  }

  void preemptCB(void)
  {
    // Not actually preempting the goal because Shibhansh did it in analysisCB
    ROS_INFO("Called when preempted from the client");
  }

  void analysisCB(const task_commons::lineGoalConstPtr goal)
  {
    ROS_INFO("Inside analysisCB");
    success = true;
    isOrange = false;
    LineAlign = false;
    FrontCenter = false;
    SideCenter = false;
    ros::Rate looprate(12);

    if (!line_server_.isActive())
      return;

    ROS_INFO("Waiting for Forward server to start.");
    ForwardClient_.waitForServer();
    SidewardClient_.waitForServer();
    TurnClient_.waitForServer();

    TaskLineInnerClass::detection_switch_on();

    // Stabilization of yaw
    turngoal.AngleToTurn = 0;
    turngoal.loop = 100000;
    TurnClient_.sendGoal(turngoal);
    // start moving forward.
    // send initial data to forward.
    forwardgoal.Goal = 100;
    forwardgoal.loop = 10;
    ForwardClient_.sendGoal(forwardgoal);

    while (goal->order)
    {
      if (line_server_.isPreemptRequested() || !ros::ok())
      {
        ROS_INFO("%s: Preempted", action_name_.c_str());
        // set the action state to preempted
        line_server_.setPreempted();
        success = false;
        break;
      }
      looprate.sleep();

      if (isOrange)
      {
        // stop will happen outside while loop so if preempted then too it will
        // stop
        break;
      }
      // publish the feedback

      feedback_.AngleRemaining = angle_goal.data;
      line_server_.publishFeedback(feedback_);
      ROS_INFO("searching line");
      ros::spinOnce();
    }

    ForwardClient_.cancelGoal();  // stop motion here
    TaskLineInnerClass::detection_switch_off();

    TaskLineInnerClass::centralize_switch_on();

    sidewardgoal.Goal = 0;
    sidewardgoal.loop = 10;
    SidewardClient_.sendGoal(sidewardgoal);
    boost::thread spin_thread_sideward_camera(&TaskLineInnerClass::spinThreadSidewardCamera, this);

    forwardgoal.Goal = 0;
    forwardgoal.loop = 10;
    ForwardClient_.sendGoal(forwardgoal);
    boost::thread spin_thread_forward_camera(&TaskLineInnerClass::spinThreadForwardCamera, this);

    while (goal->order)
    {
      if (line_server_.isPreemptRequested() || !ros::ok())
      {
        ROS_INFO("%s: Preempted", action_name_.c_str());
        // set the action state to preempted
        line_server_.setPreempted();
        success = false;
        break;
      }
      looprate.sleep();
      if (FrontCenter && SideCenter)
      {
        ROS_INFO("Line has been centralized.");
        break;
      }
      // publish the feedback
      feedback_.AngleRemaining = angle_goal.data;
      line_server_.publishFeedback(feedback_);
      ROS_INFO("x = %f, y = %f", data_X_.data, data_Y_.data);
      ros::spinOnce();
    }

    TaskLineInnerClass::centralize_switch_off();

    TaskLineInnerClass::angle_switch_on();

    TurnClient_.cancelGoal();  // stopped stablisation

    turngoal.AngleToTurn = angle_goal.data;
    turngoal.loop = 10;
    TurnClient_.sendGoal(turngoal);
    boost::thread spin_thread_turn_camera(&TaskLineInnerClass::spinThreadTurnCamera, this);

    while (goal->order)
    {
      if (line_server_.isPreemptRequested() || !ros::ok())
      {
        ROS_INFO("%s: Preempted", action_name_.c_str());
        // set the action state to preempted
        line_server_.setPreempted();
        success = false;
        break;
      }
      looprate.sleep();
      if (LineAlign)
      {
        if (angle_goal.data <= 5.0 && angle_goal.data >= -5.0)
        {
          ROS_INFO("line is aligned.");
          break;
        }
        else
        {
          ROS_INFO("resending the angle goal.");
          turngoal.AngleToTurn = angle_goal.data;
          turngoal.loop = 10;
          TurnClient_.sendGoal(turngoal);
        }
      }
      // publish the feedback
      feedback_.AngleRemaining = angle_goal.data;
      line_server_.publishFeedback(feedback_);
      ROS_INFO("angle remaining = %f", angle_goal.data);
      ros::spinOnce();
    }

    if (success)
    {
      isOrange = false;
      result_.MotionCompleted = success;
      ROS_INFO("%s: Succeeded", action_name_.c_str());
      // set the action state to succeeded
      line_server_.setSucceeded(result_);
    }
  }

  void detection_switch_on()
  {
    std_msgs::Bool msg;
    msg.data = false;
    switch_detection.publish(msg);
  }

  void detection_switch_off()
  {
    std_msgs::Bool msg;
    msg.data = true;
    switch_detection.publish(msg);
  }

  void angle_switch_on()
  {
    std_msgs::Bool msg;
    msg.data = false;
    switch_angle.publish(msg);
  }

  void angle_switch_off()
  {
    std_msgs::Bool msg;
    msg.data = true;
    switch_angle.publish(msg);
  }

  void centralize_switch_on()
  {
    std_msgs::Bool msg;
    msg.data = false;
    switch_centralize.publish(msg);
  }

  void centralize_switch_off()
  {
    std_msgs::Bool msg;
    msg.data = true;
    switch_centralize.publish(msg);
  }
};
//***********************************************************************************//

int main(int argc, char **argv)
{
  ros::init(argc, argv, "line_server");

  ROS_INFO("Waiting for Goal");

  TaskLineInnerClass taskLineObject(ros::this_node::getName(), "forward", "turningXY", "sideward");
  // ROS_INFO("Waiting for master command");

  ros::spin();
  return 0;
}
