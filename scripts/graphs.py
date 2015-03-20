#!/usr/bin/env python

import web
import sensordb

render = web.template.render('templates/')

urls = (
	# e.g., '/battery/' or '/battery/1' or '/b/1'
	'/(.*)/(.*)/(.*)', 'sensor'
)

f = { 't': 'temperature', 'b': 'battery', 'h': 'humidity', 'l': 'light' }

db = sensordb.connect("/home/pi/TinySensors/auth")
sensors = sensordb.sensors(db)

ids = {}
for id,s in sensors.items():
	ids[s['short']] = id

p = { 'd': 'day', 'm': 'month', 'y': 'year' }

class sensor:
	def GET(self, node, feature, period):
		feats = [feature]
		if feature == "":
			feats = f.values()
		elif feature in f:
			feats = [f[feature]]
		if node == "":
			node = "all"
		elif node in ids:
			node = ids[node]
		if period == "":
			period = ['day', 'month', 'year']
		elif period in p:
			period = [p[period]]
		else:
			period = [period]
		return render.sensor(node, feats, period)

if __name__ == "__main__":
	app = web.application(urls, globals())
	app.run()
