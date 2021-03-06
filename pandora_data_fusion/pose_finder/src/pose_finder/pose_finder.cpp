/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014, P.A.N.D.O.R.A. Team.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the P.A.N.D.O.R.A. Team nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *   Tsirigotis Christos <tsirif@gmail.com>
 *********************************************************************/

#include <utility>
#include <limits>
#include <vector>
#include <string>

#include <geometry_msgs/Point.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Pose.h>
#include <tf/LinearMath/Vector3.h>
#include <tf/LinearMath/Quaternion.h>
#include <tf/LinearMath/Transform.h>

#include "pandora_data_fusion_utils/exceptions.h"
#include "pose_finder/pose_finder.h"


namespace pandora_data_fusion
{

using pandora_data_fusion_utils::Utils;
using pandora_data_fusion_utils::AlertException;
using pandora_data_fusion_utils::MapException;

namespace pose_finder
{

  PoseFinder::PoseFinder(const std::string& mapType)
  {
    listener_.reset(pandora_data_fusion_utils::TfFinder::newTfListener(mapType));
  }
  PoseFinder::~PoseFinder() {}

  void PoseFinder::updateMap(const MapConstPtr& mapPtr)
  {
    mapPtr_ = mapPtr;
  }

  void PoseFinder::updateParams(float occupiedCellThres,
      float heightHighThres, float heightLowThres, float orientationCircle)
  {
    OCCUPIED_CELL_THRES = occupiedCellThres;
    HEIGHT_HIGH_THRES = heightHighThres;
    HEIGHT_LOW_THRES = heightLowThres;
    ORIENTATION_CIRCLE = orientationCircle;
  }

  geometry_msgs::Point PoseFinder::findAlertPosition(double alertYaw, double alertPitch,
      const tf::Transform& tfTransform)
  {
    geometry_msgs::Point outPosition;

    tf::Quaternion alertOrientation, sensorOrientation;
    tfTransform.getBasis().getRotation(sensorOrientation);
    tf::Vector3 origin = tfTransform.getOrigin();
    geometry_msgs::Point framePosition = Utils::vector3ToPoint(origin);

    alertOrientation.setRPY(0, alertPitch, alertYaw);
    tf::Transform newTf(sensorOrientation * alertOrientation, origin);

    geometry_msgs::Point position = positionOnWall(newTf);
    float distFromAlert = Utils::distanceBetweenPoints2D(position, framePosition);
    float height = calcHeight(newTf, distFromAlert);
    outPosition = Utils::point2DAndHeight2Point3D(position, height);

    return outPosition;
  }

  geometry_msgs::Pose PoseFinder::findAlertPose(double alertYaw, double alertPitch,
      const tf::Transform& tfTransform)
  {
    geometry_msgs::Pose outPose;
    tf::Vector3 origin = tfTransform.getOrigin();
    geometry_msgs::Point framePosition = Utils::vector3ToPoint(origin);
    outPose.position = findAlertPosition(alertYaw, alertPitch, tfTransform);
    outPose.orientation = findAppropriateOrientation(framePosition, outPose.position);

    return outPose;
  }

  geometry_msgs::Pose PoseFinder::findPoseFromPoints(
      const boost::array<double, 4>& pointsYaw,
      const boost::array<double, 4>& pointsPitch,
      const boost::array<double, 4>& pointsDepth,
      const tf::Transform& tfTransform, double* length)
  {
    geometry_msgs::Point leftPoint = projectAlertPosition(pointsYaw[3],
        pointsPitch[3], pointsDepth[3], tfTransform);
    geometry_msgs::Point rightPoint = projectAlertPosition(pointsYaw[1],
        pointsPitch[1], pointsDepth[1], tfTransform);
    geometry_msgs::Point lowPoint = projectAlertPosition(pointsYaw[2],
        pointsPitch[2], pointsDepth[2], tfTransform);
    geometry_msgs::Point highPoint = projectAlertPosition(pointsYaw[0],
        pointsPitch[0], pointsDepth[0], tfTransform);
    if (lowPoint.z > 1.20 || highPoint.z < -0.1)
      throw AlertException("Obstacle either too low or two high");
    // Absolute depth and depth difference are evaluated at
    // pandora_vision_obstacle
    // geometry_msgs::Point lowPoint = findAlertPosition(pointsYaw[3],
    //     pointsPitch[3], tfTransform);
    // geometry_msgs::Point rightPoint = findAlertPosition(pointsYaw[1],
    //     pointsPitch[1], tfTransform);
    geometry_msgs::Quaternion orientation = Utils::calculateQuaternion(leftPoint, rightPoint);
    *length = Utils::distanceBetweenPoints2D(leftPoint, rightPoint);
    geometry_msgs::Pose outPose;
    outPose.orientation = orientation;
    outPose.position.x = (leftPoint.x + rightPoint.x) / 2;
    outPose.position.y = (leftPoint.y + rightPoint.y) / 2;
    return outPose;
  }

  geometry_msgs::Point PoseFinder::projectAlertPosition(
      double alertYaw, double alertPitch,
      double depth, const tf::Transform& tfTransform)
  {
    tf::Quaternion alertOrientation, sensorOrientation;
    tfTransform.getBasis().getRotation(sensorOrientation);
    tf::Vector3 origin = tfTransform.getOrigin();

    alertOrientation.setRPY(0, alertPitch, alertYaw);
    tf::Transform newTf(sensorOrientation * alertOrientation, origin);

    tf::Vector3 projection = depth * newTf.getBasis().getColumn(0);

    return Utils::vector3ToPoint(origin + projection);
  }

  float PoseFinder::calcHeight(const tf::Transform& transform, float distFromAlert)
  {
    geometry_msgs::Point xDirection = Utils::vector3ToPoint(transform.getBasis().getColumn(0));
    float lengthInXYPlane = sqrt(xDirection.x * xDirection.x + xDirection.y * xDirection.y);
    float alertHeight = xDirection.z / lengthInXYPlane * distFromAlert;

    alertHeight += transform.getOrigin()[2];

    ROS_DEBUG_NAMED("pose_finder",
        "[POSE_FINDER] Height of alert = %f ", alertHeight);
    ROS_DEBUG_NAMED("pose_finder", "[POSE_FINDER] Distance from alert = %f ",
        distFromAlert);

    if (alertHeight > HEIGHT_HIGH_THRES || alertHeight < HEIGHT_LOW_THRES)
      throw AlertException("Alert either too low or two high");

    return alertHeight;
  }

  geometry_msgs::Point PoseFinder::positionOnWall(const tf::Transform& transform)
  {
    if (mapPtr_.get() == NULL)
      throw MapException("Map pointer is uninitialized in PoseFinder");
    if (mapPtr_->data.size() == 0)
      throw MapException("Map size equals zero in PoseFinder");

    const float resolution = mapPtr_->info.resolution;
    float x = 0, y = 0, D = 5 * resolution;
    geometry_msgs::Point xDirection = Utils::vector3ToPoint(transform.getBasis().getColumn(0));

    float currX = transform.getOrigin()[0];
    float currY = transform.getOrigin()[1];

    x = D * xDirection.x + currX;
    y = D * xDirection.y + currY;

    while (CELL(x, y, mapPtr_) < OCCUPIED_CELL_THRES * 100)
    {
      x += resolution * xDirection.x;
      y += resolution * xDirection.y;
    }
    if (CELL(x, y, mapPtr_) > OCCUPIED_CELL_THRES * 100)
    {
      geometry_msgs::Point onWall;
      onWall.x = x;
      onWall.y = y;
      return onWall;
    }
    else
      throw AlertException("Can not find point on wall");
  }

  geometry_msgs::Quaternion PoseFinder::findAppropriateOrientation(
      const geometry_msgs::Point& framePoint, const geometry_msgs::Point& alertPoint)
  {
    if (mapPtr_.get() == NULL)
      throw MapException("Map pointer is uninitialized in PoseFinder");
    if (mapPtr_->data.size() == 0)
      throw MapException("Map size equals zero in PoseFinder");

    std::vector< std::vector<geometry_msgs::Point> > freeArcs;
    float x = 0, y = 0;
    unsigned int i, j;
    bool freeSpace = false;
    const int angle_step = 5;

    for (i = 0; i < 360; i += angle_step) {
      x = alertPoint.x + ORIENTATION_CIRCLE * cos(i * DEGREE);
      y = alertPoint.y + ORIENTATION_CIRCLE * sin(i * DEGREE);

      if (CELL(x, y, mapPtr_) < OCCUPIED_CELL_THRES * 100)
      {
        if (!freeSpace)
        {
          std::vector<geometry_msgs::Point> freeArc;
          freeArcs.push_back(freeArc);
          freeSpace = true;
        }
        geometry_msgs::Point temp;
        temp.x = x;
        temp.y = y;
        freeArcs.back().push_back(temp);
      }
      else
      {
        freeSpace = false;
      }
    }
    i -= angle_step;
    x = alertPoint.x + ORIENTATION_CIRCLE * cos(i * DEGREE);
    y = alertPoint.y + ORIENTATION_CIRCLE * sin(i * DEGREE);
    bool last_point = CELL(x, y, mapPtr_) < OCCUPIED_CELL_THRES * 100;
    i = 0;
    x = alertPoint.x + ORIENTATION_CIRCLE * cos(i * DEGREE);
    y = alertPoint.y + ORIENTATION_CIRCLE * sin(i * DEGREE);
    bool first_point = CELL(x, y, mapPtr_) < OCCUPIED_CELL_THRES * 100;
    if (first_point == true && last_point == true)
    {
      std::vector<geometry_msgs::Point> lastFreeArc = freeArcs.back();
      freeArcs.pop_back();
      freeArcs[0].insert(freeArcs[0].end(), lastFreeArc.begin(), lastFreeArc.end());
    }

    geometry_msgs::Point approachPoint;
    float smallestDistance = std::numeric_limits<float>::max();
    for (i = 0; i < freeArcs.size(); ++i) {
      geometry_msgs::Point middle;
      for (j = 0; j < freeArcs[i].size(); ++j) {
        middle.x += freeArcs[i][j].x / freeArcs[i].size();
        middle.y += freeArcs[i][j].y / freeArcs[i].size();
      }
      float distance = Utils::distanceBetweenPoints2D(framePoint, middle);
      if (distance < smallestDistance)
      {
        approachPoint = middle;
        smallestDistance = distance;
      }
    }

    return Utils::calculateQuaternion(alertPoint, approachPoint);
  }

  tf::Transform PoseFinder::lookupTransformFromWorld(const std::string& globalFrame,
      const std_msgs::Header& header)
  {
    tf::StampedTransform tfTransform;

    listener_->waitForTransform(globalFrame, header.frame_id,
        header.stamp, ros::Duration(1));

    listener_->lookupTransform(globalFrame, header.frame_id,
        header.stamp, tfTransform);

    return tfTransform;
  }

}  // namespace pose_finder
}  // namespace pandora_data_fusion
