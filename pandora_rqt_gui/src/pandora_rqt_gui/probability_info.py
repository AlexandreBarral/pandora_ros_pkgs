# Software License Agreement
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of P.A.N.D.O.R.A. Team nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

__author__ = "Chamzas Konstantinos"
__maintainer__ = "Chamzas Konstantinos"
__email__ = "chamzask@gmail.com"
import os
import rospkg

from python_qt_binding import loadUi
from python_qt_binding.QtCore import QTimer, Slot
from python_qt_binding.QtGui import QWidget

from pandora_data_fusion_msgs.msg import GlobalProbabilitiesMsg
from .widget_info import WidgetInfo

global_propabilities_topic = "/data_fusion/signs_of_life"


class ProbabilityInfoWidget(QWidget):
    """
    ProbabilityInfoWidget.start must be called in order to update topic pane.
    """
    def __init__(self, plugin=None):

        super(ProbabilityInfoWidget, self).__init__()
        self.id_ = "ProbabilityInfo"

        rp = rospkg.RosPack()
        ui_file = os.path.join(rp.get_path('pandora_rqt_gui'), 'resources',
                               'ProbabilityInfo.ui')
        loadUi(ui_file, self)

        # Create the subcribers.
        self.widget_probabilities_info = WidgetInfo(global_propabilities_topic,
                                                    GlobalProbabilitiesMsg)

        # Create and connect the timer.
        self.timer_refresh_widget = QTimer(self)
        self.timer_refresh_widget.timeout.connect(self.refresh_topics)

    def start(self):
        self.widget_probabilities_info.start_monitoring()
        self.timer_refresh_widget.start(100)

    # Connected slot to the timer in order to refresh.
    @Slot()
    def refresh_topics(self):
        message = self.widget_probabilities_info.last_message

        if message is not None:
            self.co2Bar.setValue(message.co2 % 100)
            self.thermalBar.setValue(message.thermal % 100)
            self.motionBar.setValue(message.motion % 100)
            self.soundBar.setValue(message.sound % 100)
            self.victimBar.setValue(message.victim % 100)

    def shutdown(self):
        self.widget_probabilities_info.stop_monitoring()
        self.timer_refresh_widget.stop()
