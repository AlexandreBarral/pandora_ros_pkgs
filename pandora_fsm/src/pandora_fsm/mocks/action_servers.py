#! /usr/bin/env python

"""
    Mock action servers.
"""

from rospy import loginfo, logwarn, sleep, Subscriber
import roslib
roslib.load_manifest('pandora_fsm')
from std_msgs.msg import String, Bool

from actionlib import SimpleActionServer as ActionServer

# Messages
from .msgs import create_victim_info, create_world_model
import move_base_msgs.msg
from move_base_msgs.msg import MoveBaseAction
from pandora_data_fusion_msgs.msg import (ChooseVictimAction,
                                          ChooseVictimResult,
                                          ValidateVictimAction,
                                          ValidateVictimResult)
from pandora_gui_msgs.msg import ValidateVictimGUIResult


class MockActionServer(object):

    """ MockActionServer base class """

    def __init__(self, name, topic, action_type):
        """ Creating a custom mock action server."""

        self._topic = topic
        self._name = name
        self._action_type = action_type
        self.timeout = 5
        self.action_result = None

        Subscriber('mock/' + name, String, self.receive_commands)
        Subscriber('mock/gui_result', Bool, self.set_gui_result)
        self._server = ActionServer(self._topic, self._action_type,
                                    self.success, False)
        self._server.start()
        loginfo('>>> Starting ' + self._name)

    def receive_commands(self, msg):
        """ Decides the result of the next call. """

        callback, timeout = msg.data.split(':')
        self.timeout = float(timeout)
        self._server.execute_callback = getattr(self, callback)
        logwarn('>>> ' + self._name + ': Current callback -> ' + callback)
        sleep(1)

    def abort(self, goal):
        """ Aborts any incoming goal. """

        logwarn('>>> ' + self._name + ': This goal will be aborted.')
        sleep(self.timeout)
        self._server.set_aborted()

    def success(self, goal):
        """ Succeeds any incoming goal. """

        logwarn('>>> ' + self._name + ': This goal will succeed.')
        sleep(self.timeout)
        self._server.set_succeeded(self.action_result)

    def preempt(self, goal):
        """ Preempts any incoming goal. """

        logwarn('>>> ' + self._name + ': This goal will be preempted.')
        sleep(self.timeout)
        self._server.set_preempted()

    def set_gui_result(self, msg):
        """ Sets the result of the goal. """

        self.action_result = ValidateVictimGUIResult()
        logwarn('>>> The gui response will be: ' + str(msg.data))
        self.action_result.victimValid = msg.data


class DataFusionServer(object):

    def __init__(self, name, topic, action_type, result_type):
        self.name = name
        self.result = result_type()
        self.topic = topic
        self.action_type = action_type
        self.timeout = 5
        Subscriber('mock/' + name, String, self.receive_commands)
        self.server = ActionServer(self.topic, self.action_type, self.success,
                                   False)
        self.server.start()
        loginfo('>>> Starting ' + self.name)

    def receive_commands(self, msg):

        callback, timeout = msg.data.split(':')
        self.timeout = float(timeout)
        self.server.execute_callback = getattr(self, callback)
        logwarn('>>> ' + self.name + ': Current callback -> ' + callback)
        sleep(1)

    def create_random_world_model(self):
        victims = [create_victim_info(i + 1) for i in range(2)]
        visited = [create_victim_info(i + 1) for i in range(4)]
        self.result.worldModel = create_world_model(victims, visited)

    def success(self, goal):
        self.create_random_world_model()
        logwarn('>>> ' + self.name + ': This goal will succeed.')
        sleep(self.timeout)
        self.server.set_succeeded(result=self.result)

    def abort(self, goal):
        self.create_random_world_model()
        logwarn('>>> ' + self.name + ': This goal will be aborted.')
        sleep(self.timeout)
        self.server.set_aborted(result=self.result)


class MoveBaseServer(MockActionServer):

    def __init__(self, name, topic):
        self._name = name
        self.action_result = None
        self._feedback = move_base_msgs.msg.MoveBaseFeedback()
        self._result = move_base_msgs.msg.MoveBaseResult()

        self.abort_with_feedback = False
        self._topic = topic
        Subscriber('mock/feedback_' + name, String, self.receive_commands)
        self._server = ActionServer(self._topic, MoveBaseAction, self.execute,
                                    False)
        self._server.start()
        loginfo('>>> Starting ' + self._name + ' with feedback.')

    def receive_commands(self, msg):
        callback, timeout = msg.data.split(':')
        self.timeout = float(timeout)
        if callback == 'abort_feedback':
            self.abort_with_feedback = True
            self._server.execute_callback = getattr(self, 'execute')
        else:
            self.abort_with_feedback = False
            self._server.execute_callback = getattr(self, callback)
            logwarn('>>> ' + self._name + ': Current callback -> ' + callback)
        sleep(1)

    def execute(self, goal):
        success = True
        start_x = 0
        start_y = 0
        goal_x = goal.target_pose.pose.position.x
        goal_y = goal.target_pose.pose.position.y
        loginfo('Goal -> x(%d), y(%d).', goal_x, goal_y)
        if self.abort_with_feedback:
            sleep(self.timeout)
            self._feedback.base_position.pose.position.x = goal_x + 0.05
            self._feedback.base_position.pose.position.y = goal_y + 0.05
            self._server.publish_feedback(self._feedback)
            sleep(self.timeout)
            self._server.set_aborted()
            return
        while True:
            # Make a move.
            if start_x < goal_x:
                start_x += 0.5
            if start_y < goal_y:
                start_y += 0.5

            # Send feedback
            self._feedback.base_position.pose.position.x = start_x
            self._feedback.base_position.pose.position.y = start_y
            self._server.publish_feedback(self._feedback)

            if start_x >= goal_x and start_y >= goal_y:
                break
            sleep(self.timeout)

        if success:
            loginfo('MoveBaseAction: Goal succeeded...')
            _result = self._feedback
            self._server.set_succeeded(_result)
