// Copyright 2016 AUV-IITK
#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "ros/ros.h"
#include "std_msgs/Float64.h"
#include <fstream>
#include <vector>
#include <cv.h>
#include <highgui.h>
#include "std_msgs/String.h"
#include "std_msgs/Int8.h"
#include <std_msgs/Bool.h>
#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv/highgui.h>
#include <image_transport/image_transport.h>
#include "std_msgs/Float64MultiArray.h"
#include <cv_bridge/cv_bridge.h>
#include <sstream>
#include <string>
#include "std_msgs/Header.h"
using cv::Mat;
using cv::split;
using cv::Size;
using cv::Scalar;
using cv::normalize;
using cv::Point;
using cv::VideoCapture;
using cv::NORM_MINMAX;
using cv::MORPH_ELLIPSE;
using cv::COLOR_BGR2HSV;
using cv::destroyAllWindows;
using cv::getStructuringElement;
using cv::Vec4i;
using cv::namedWindow;
using std::vector;
using std::endl;
using std::cout;



int w = -2, x = -2, y = -2, z = -2;
bool IP = true;
bool flag = false;
bool video = false;
int t1min = 0, t1max = 88, t2min = 89, t2max = 251, t3min = 0, t3max = 255, lineCount = 0;  // Default Params

std_msgs::Float64 msg;
// parameters in param file should be nearly the same as the commented values
// params for an orange strip
int LowH = 0;   // 0
int HighH = 88;  // 88

int LowS = 0;   // 0
int HighS = 251;  // 251

int LowV = 0;   // 0
int HighV = 255;  // 255

// params for hough line transform
int lineThresh = 60;     // 60
int minLineLength = 70;  // 70
int maxLineGap = 10;     // 10
int houghThresh = 15;    // 15

double rho = 0.1;
double finalAngle = -1;
double minDeviation = 0.02;


cv::Mat frame;
cv::Mat newframe, sent_to_callback, imgLines;
int count = 0, count_avg = 0;


double computeMean(vector<double> &newAngles)
{
  double sum = 0;
  for (size_t i = 0; i < newAngles.size(); i++)
  {
    sum = sum + newAngles[i];
  }
  return sum / newAngles.size();
}
// called when few lines are detected
// to remove errors due to any stray results
double computeMode(vector<double> &newAngles)
{
  double mode = newAngles[0];
  int freq = 1;
  int tempFreq;
  double diff;
  for (int i = 0; i < newAngles.size(); i++)
  {
    tempFreq = 1;

    for (int j = i + 1; j < newAngles.size(); j++)
    {
      diff = newAngles[j] - newAngles[i] > 0.0 ? newAngles[j] - newAngles[i] : newAngles[i] - newAngles[j];
      if (diff <= minDeviation)
      {
        tempFreq++;
        newAngles.erase(newAngles.begin() + j);
        j = j - 1;
      }
    }

    if (tempFreq >= freq)
    {
      mode = newAngles[i];
      freq = tempFreq;
    }
  }

  return mode;
}

void callback(int, void *)
{
  vector<Vec4i> lines;
  HoughLinesP(sent_to_callback, lines, 1, CV_PI / 180, lineThresh, minLineLength, maxLineGap);

  frame.copyTo(imgLines);
  imgLines = Scalar(0, 0, 0);
  vector<double> angles(lines.size());

  lineCount = lines.size();
  int j = 0;
  for (size_t i = 0; i < lines.size(); i++)
  {
    Vec4i l = lines[i];
    line(imgLines, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 255, 0), 1, CV_AA);
    if ((l[2] == l[0]) || (l[1] == l[3])) continue;
    angles[j] = atan(static_cast<double>(l[2] - l[0]) / (l[1] - l[3]));
    j++;
  }

  imshow("LINES", imgLines + frame);

  // if num of lines are large than one or two stray lines won't affect the mean
  // much
  // but if they are small in number than mode has to be taken to save the error
  // due to those stray line

  if (lines.size() > 0 && lines.size() < 10)
    finalAngle = computeMode(angles);
  else if (lines.size() > 0)
    finalAngle = computeMean(angles);
}

void lineAngleListener(std_msgs::Bool msg)
{
  IP = msg.data;
}

void imageCallback(const sensor_msgs::ImageConstPtr &msg)
{
  try
  {
    count++;
    newframe = cv_bridge::toCvShare(msg, "bgr8")->image;
    cvNamedWindow("newframe", CV_WINDOW_NORMAL);
    ///////////////////////////// DO NOT REMOVE THIS, IT COULD BE INGERIOUS TO HEALTH /////////////////////
    newframe.copyTo(frame);
    cv::imshow("newframe", newframe);
    ////////////////////////// FATAL ///////////////////////////////////////////////////
  }
  catch (cv_bridge::Exception &e)
  {
    ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
  }
}

int main(int argc, char* argv[])
{
  int height, width, step, channels;  // parameters of the image we are working on
  std::string Video_Name = "Random_Video";
  if (argc >= 2)
    flag = true;
  if (argc == 3)
  {
    video = true;
    std::string avi = ".avi";
    Video_Name = (argv[2]) + avi;
  }

  cv::VideoWriter output_cap(Video_Name, CV_FOURCC('D', 'I', 'V', 'X'), 9, cv::Size(640, 480));

  ros::init(argc, argv, "gate_detection");
  ros::NodeHandle n;

  ros::Publisher pub = n.advertise<std_msgs::Float64>("/varun/ip/line_angle", 1000);
  ros::Subscriber sub = n.subscribe<std_msgs::Bool>("line_angle_switch", 1000, &lineAngleListener);
  ros::Rate loop_rate(10);

  image_transport::ImageTransport it(n);
  image_transport::Subscriber sub1 = it.subscribe("/varun/sensors/bottom_camera/image_raw", 1, imageCallback);

  cvNamedWindow("After Color Filtering", CV_WINDOW_NORMAL);
  cvNamedWindow("Contours", CV_WINDOW_NORMAL);
  cvNamedWindow("LINES", CV_WINDOW_NORMAL);

  if (flag)
  {
    cvNamedWindow("F1", CV_WINDOW_NORMAL);
    cvNamedWindow("F2", CV_WINDOW_NORMAL);
    cvNamedWindow("F3", CV_WINDOW_NORMAL);

    cvCreateTrackbar("t1min", "F1", &t1min, 260, NULL);
    cvCreateTrackbar("t1max", "F1", &t1max, 260, NULL);
    cvCreateTrackbar("t2min", "F2", &t2min, 260, NULL);
    cvCreateTrackbar("t2max", "F2", &t2max, 260, NULL);
    cvCreateTrackbar("t3min", "F3", &t3min, 260, NULL);
    cvCreateTrackbar("t3max", "F3", &t3max, 260, NULL);
  }
  // capture size -
  CvSize size = cvSize(width, height);

  // Initialize different images that are going to be used in the program
  cv::Mat hsv_frame, thresholded, thresholded1, thresholded2, thresholded3, filtered;  // image converted to HSV plane
  // asking for the minimum distance where bwe fire torpedo

  while (ros::ok())
  {
    loop_rate.sleep();
    // Get one frame
    if (frame.empty())
    {
      std::cout << "empty frame \n";
      ros::spinOnce();
      continue;
    }

    if (video)
      output_cap.write(frame);

    // get the image data
    height = frame.rows;
    width = frame.cols;
    step = frame.step;

    // Covert color space to HSV as it is much easier to filter colors in the HSV color-space.
    cv::cvtColor(frame, hsv_frame, CV_BGR2HSV);

    cv::Scalar hsv_min = cv::Scalar(t1min, t2min, t3min, 0);
    cv::Scalar hsv_max = cv::Scalar(t1max, t2max, t3max, 0);

    // Filter out colors which are out of range.
    cv::inRange(hsv_frame, hsv_min, hsv_max, thresholded);
    // Split image into its 3 one dimensional images
    cv::Mat thresholded_hsv[3];
    cv::split(hsv_frame, thresholded_hsv);

    // Filter out colors which are out of range.
    cv::inRange(thresholded_hsv[0], cv::Scalar(t1min, 0, 0, 0), cv::Scalar(t1max, 0, 0, 0), thresholded_hsv[0]);
    cv::inRange(thresholded_hsv[1], cv::Scalar(t2min, 0, 0, 0), cv::Scalar(t2max, 0, 0, 0), thresholded_hsv[1]);
    cv::inRange(thresholded_hsv[2], cv::Scalar(t3min, 0, 0, 0), cv::Scalar(t3max, 0, 0, 0), thresholded_hsv[2]);

    cv::GaussianBlur(thresholded, thresholded, cv::Size(9, 9), 0, 0, 0);
    cv::imshow("After Color Filtering", thresholded);  // The stream after color filtering

    if (flag)
    {
      cv::imshow("F1", thresholded_hsv[0]);              // individual filters
      cv::imshow("F2", thresholded_hsv[1]);
      cv::imshow("F3", thresholded_hsv[2]);
    }

    if ((cvWaitKey(10) & 255) == 27)
      break;

    if (!IP)
    {
      // find contours
      std::vector<std::vector<cv::Point> > contours;
      cv::Mat thresholded_Mat = thresholded;
      findContours(thresholded_Mat, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);  // Find the contours in the image
      double largest_area = 0, largest_contour_index = 0;
      if (contours.empty())
      {
        msg.data = -finalAngle * (180 / 3.14)+90;
        pub.publish(msg);
        ros::spinOnce();
        // If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
        // remove higher bits using AND operator
        if ((cvWaitKey(10) & 255) == 27)
          break;
        continue;
      }

      for (int i = 0; i < contours.size(); i++)  // iterate through each contour.
      {
        double a = contourArea(contours[i], false);  //  Find the area of contour
        if (a > largest_area)
        {
          largest_area = a;
          largest_contour_index = i;  // Store the index of largest contour
        }
      }
      // Convex HULL
      std::vector<std::vector<cv::Point> > hull(contours.size());
      convexHull(cv::Mat(contours[largest_contour_index]), hull[largest_contour_index], false);

      cv::Mat Drawing(thresholded_Mat.rows, thresholded_Mat.cols, CV_8UC1, cv::Scalar::all(0));
      std::vector<cv::Vec4i> hierarchy;
      cv::Scalar color(255, 255, 255);
      drawContours(Drawing, contours, largest_contour_index, color, 2, 8, hierarchy);
      cv::imshow("Contours", Drawing);

      std_msgs::Float64 msg;
      Drawing.copyTo(sent_to_callback);
      /*
      msg.data never takes positive 90
      when the angle is 90 it will show -90
      -------------TO BE CORRECTED-------------
      */
      msg.data = -finalAngle * (180 / 3.14);
      if (lineCount > 0)
      {
        pub.publish(msg);
      }
      callback(0, 0);        // for displaying the thresholded image initially
      ros::spinOnce();
      // loop_rate.sleep();

      // If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
      // remove higher bits using AND operator
      if ((cvWaitKey(10) & 255) == 27)
        break;
    }
    else
    {
       std::cout << "waiting\n";
       ros::spinOnce();
    }
  }
  output_cap.release();
  return 0;
}
