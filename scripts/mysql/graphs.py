#!/usr/bin/env python

import web
import sensordb

render = web.template.render('templates/')

urls = (
	# e.g., '/battery' or '/battery/1' or '/b/1/week'
	'/(.*)/(.*)/(.*)', 'sensor_nfp',
	'/(.*)/(.*)', 'sensor_nf',
	'/(.*)', 'sensor_n'
)

f = { 't': 'temperature', 'b': 'battery', 'h': 'humidity', 'l': 'light' }

db = sensordb.connect("/home/pi/TinySensors/auth")
sensors = sensordb.sensors(db)

ids = {}
for id,s in sensors.items():
	ids[s['short']] = id

p = { 'd': 'day', 'w': 'week', 'm': 'month', 'y': 'year' }

def map_node(node):
	if node == "":
		return "all"
	if node in ids:
		return ids[node]
	return node

def map_feature(feature):
	if feature == "":
		return f.values()
	if feature in f:
		return [f[feature]]
	return [feature]

def map_period(period):
	if period == "":
		return p.values()
	if period in p:
		return [p[period]]
	return [period]

class sensor_n:
	def GET(self, node):
		return render.sensor(map_node(node), f.values(), p.values())

class sensor_nf:
	def GET(self, node, feature):
		return render.sensor(map_node(node), map_feature(feature), p.values())

class sensor_nfp:
	def GET(self, node, feature, period):
		return render.sensor(map_node(node), map_feature(feature), map_period(period))

if __name__ == "__main__":
	app = web.application(urls, globals())
	app.run()
