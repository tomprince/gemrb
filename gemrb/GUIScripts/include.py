# this file is executed at gemrb startup, for the Console
from ie_stats import *

from twisted.internet import reactor

import web

reactor.startRunning()
GemRB.SetTickHook(lambda: reactor.iterate())
