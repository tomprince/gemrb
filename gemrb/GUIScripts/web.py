from twisted.internet import reactor, task
from twisted.application import service, internet
from twisted.web import static, server, script
from twisted.web.resource import Resource

import GemRB
import GUIClasses
from ie_stats import *

class MainPage(Resource):
	def render_GET(self, ctx):
		html = ["<html><body>"]
		for pc in range(GemRB.GetPartySize()):
			pc = pc + 1
			name = GemRB.GetPlayerName(pc, 0)
			hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
			hpmax = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
			html.append("%s: %s/%s<br>" % (name, hp, hpmax))
		html.append("</body></html>")
		return "".join(html)

import time

class ClockPage(Resource):
	isLeaf = True
	def __init__(self, page):
		self.presence=[]
		self.loopingCall = task.LoopingCall(self.__print_time)
		self.page = page
		Resource.__init__(self)

	def __start(self):
		if not self.loopingCall.running:
			self.loopingCall.start(0.1, False)

	def render_GET(self, request):
		request.setHeader("Content-Type", "multipart/x-mixed-replace; boundary=--")
		request.write("--\nContent-Type: text/html\n\n")
		request.write(self.page.render_GET(request))
		self.presence.append(request)
		d = request.notifyFinish()
		d.addCallback(self.__client_disconnected, request)
		d.addErrback(self.__client_disconnected, request)
		self.__start()
		return server.NOT_DONE_YET
	def __client_disconnected(self, r, req):
		self.presence.remove(req)
		if not self.presence:
			self.loopingCall.stop()

	def __print_time(self):
		for p in self.presence:
			p.write("\n--\nContent-Type: text/html\n\n")
			p.write(self.page.render_GET(p))

class NPCPage(Resource):
	def render_GET(self, ctx):
		html = ["<html><body>"]
		for npc in GemRB.GetNPCs():
			html.append("%s: %s<br>" % (npc.name, npc.scriptname))
		html.append("</body></html>")
		return "".join(html)


class LoadSavePage(Resource):
	def __init__(self, game):
		self.game = game
	def render_GET(self, context):
		GemRB.QuitGame()
		GemRB.LoadGame(self.game)
		GemRB.EnterGame()
		return "Loaded ..."

class SavePage(Resource):
	isLeaf = False
	def render_GET(self, ctx):
		html = ["<html><body>"]
		self.games = GemRB.GetSaveGames()
		for g in self.games:
			html.append("%s - %s<br>" % (g.GetName(), g.GetDate()))
		html.append("</body></html>")
		return "".join(html)
	def getChild(self, name, request):
		i = int(name)
		if (i < len(self.games)):
			return LoadSavePage(self.games[i])

class ActorPage(Resource):
	def __init__(self, actor):
		self.actor = actor
	def render_GET(self, ctc):
		html = ["<html><body>"]
		for k in sorted(self.actor.keys()):
			if k == 'stats':
				import ie_stats_only
				html.append("<ul>")
				for name, stat in sorted(ie_stats_only.stats.items(), key=
						lambda s: s[1]) :
					html.append("<li>%s: %s</li>" % (name, self.actor.stats[stat]))
				html.append("</ul>")
			else:
				html.append("%s: %s<br>" % (k , self.actor[k]))
		html.append("</body></html>")
		return "".join(html)

class ActorsPage(Resource):
	def render_GET(self, ctx):
		html = ["<html><body><ul>"]
		for actor in set(GemRB.GetPCs()).union(GemRB.GetNPCs()):
			html.append('<li><a href="/actor/%s">%s</li>' % (actor.GlobalID, actor.name))
		html.append("</li></body></html>")
		return "".join(html)

	def getChild(self, name, request):
		if not name:
			return self
		actor = GUIClasses.GActor(int(name))
		return ActorPage(actor)

class AreaPage(Resource):
	def __init__(self, area):
		self.area = area
	def render_GET(self, ctc):
		html = ["<html><body>"]
		html += ["<h1>%s</h1>" % self.area.name]
		html.append("<ul>")
		for actor in self.area.actors:
			html.append('<li><a href="/actor/%s">%s</li>' % (actor.GlobalID, actor.name or actor.scriptname))
		html.append("<ul></body></html>")
		return "".join(html)

class AreasPage(Resource):
	def render_GET(self, ctx):
		html = ["<html><body><ul>"]
		for area in GemRB.GetAreas():
			html.append('<li><a href="/area/%s">%s</li>' % (area.ID, area.name))
		html.append("</li></body></html>")
		return "".join(html)

	def getChild(self, name, request):
		if not name:
			return self
		if name == "current":
			actor = GemRB.GetCurrentArea()
		else:
			actor = GUIClasses.GArea(int(name))
		return AreaPage(actor)

try:
	application
except NameError:
	application = service.MultiService() 
	port        = 8080
	root	    = static.File("/Users/cougar/src/gemrb/gemrb/html")
	site        = server.Site(root)
	webService  = internet.TCPServer(port, site)
	webService.setServiceParent(application)
	application.startService()

root.putChild("", MainPage())
root.putChild("save", SavePage())
root.putChild("npc", NPCPage())
root.putChild("time", ClockPage(MainPage()))
root.putChild("actor", ActorsPage())
root.putChild("area", AreasPage())
