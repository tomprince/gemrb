from twisted.internet import reactor, task
from twisted.application import service, internet
from twisted.web import static, server, script
from twisted.web.resource import Resource

from jinja2 import Environment, FileSystemLoader

import GemRB
import GUIClasses
from ie_stats import *


#class ClockPage(Resource):
#	isLeaf = True
#	def __init__(self, page):
#		self.presence=[]
#		self.loopingCall = task.LoopingCall(self.__print_time)
#		self.page = page
#		Resource.__init__(self)
#
#	def __start(self):
#		if not self.loopingCall.running:
#			self.loopingCall.start(0.1, False)
#
#	def render_GET(self, request):
#		request.setHeader("Content-Type", "multipart/x-mixed-replace; boundary=--")
#		request.write("--\nContent-Type: text/html\n\n")
#		request.write(self.page.render_GET(request))
#		self.presence.append(request)
#		d = request.notifyFinish()
#		d.addCallback(self.__client_disconnected, request)
#		d.addErrback(self.__client_disconnected, request)
#		self.__start()
#		return server.NOT_DONE_YET
#	def __client_disconnected(self, r, req):
#		self.presence.remove(req)
#		if not self.presence:
#			self.loopingCall.stop()
#
#	def __print_time(self):
#		for p in self.presence:
#			p.write("\n--\nContent-Type: text/html\n\n")
#			p.write(self.page.render_GET(p))

class HTMLResource(Resource):
	def __init__(self, _template):
		Resource.__init__(self)
		self.template = env.get_template(_template)
	def render_GET(self, ctx):
		return self.template.render(**self.variables()).encode('utf-8')

class Reload(Resource):
	def render_GET(self, ctx):
		import web
		reload(web)
		ctx.setResponseCode(205, "Reloaded")
		return ''


class MainPage(HTMLResource):
	def __init__(self):
		HTMLResource.__init__(self, "main.html")
	def variables(self):
		return { 'pages' : [
			("Actors", "/actor/"),
			("Areas", "/area/"),
			("Saves", "/save/"),
			("reload", "/reload"),
			]}

class SavesPage(HTMLResource):
	isLeaf = False
	def __init__(self):
		HTMLResource.__init__(self, "saves.html")
	def variables(self):
		return { 'savegames' : GemRB.GetSaveGames() }
	#def getChild(self, name, request):
	#	i = int(name)
	#	if (i < len(self.games)):
	#		return LoadSavePage(self.games[i])

class ActorPage(HTMLResource):
	def __init__(self, actor):
		HTMLResource.__init__(self, "actor.html")
		self.actor = actor
	def variables(self):
		return { 'actor' : self.actor }

class ActorsPage(HTMLResource):
	def __init__(self):
		HTMLResource.__init__(self, "actors.html")
	def variables(self):
		return { 'actors' : GemRB.GetPCs() }

	def getChild(self, name, request):
		if not name:
			return self
		actor = GUIClasses.GActor(int(name))
		return ActorPage(actor)

class AreaPage(HTMLResource):
	def __init__(self, area):
		HTMLResource.__init__(self, "area.html")
		self. area = area
	def variables(self):
		return { 'area' : self.area }

class AreasPage(HTMLResource):
	def __init__(self):
		HTMLResource.__init__(self, "areas.html")
	def variables(self):
		return { 'areas' : GemRB.GetAreas() }

	def getChild(self, name, request):
		if not name:
			return self
		if name == "current":
			actor = GemRB.GetCurrentArea()
		else:
			actor = GUIClasses.GArea(int(name))
		return AreaPage(actor)

path	    = "/Users/cougar/src/gemrb/gemrb/html/"
try:
	root
except NameError:
	root	    = Resource()

env         = Environment(loader=FileSystemLoader(path), auto_reload=True)
import ie_stats_only
env.globals.update(ie_stats_only.stats)
env.globals['GemRB'] = GemRB
root.putChild("", MainPage())
root.putChild("save", SavesPage())
root.putChild("actor", ActorsPage())
root.putChild("area", AreasPage())
root.putChild("reload", Reload())

try:
	application
except NameError:
	application = service.MultiService() 
	port        = 8080
	site        = server.Site(root)
	webService  = internet.TCPServer(port, site)
	webService.setServiceParent(application)
	application.startService()
