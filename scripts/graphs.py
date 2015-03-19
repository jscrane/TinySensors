#!/usr/bin/env python

import web
import sensordb

render = web.template.render('templates/')

urls = (
	# e.g., '/battery/' or '/battery/1' or '/b/1'
	'/(.*)/(.*)', 'sensor'
)

f = { 'a': 'all', 't': 'temperature', 'b': 'battery', 'h': 'humidity' }

db = sensordb.connect("/home/pi/TinySensors/auth")
sensors = sensordb.sensors(db)

i = {}
for id,s in sensors.items():
	i[s['short']] = id

class sensor:
	def GET(self, feature, sensor):
		if feature in f:
			feature = f[feature]
		if sensor == "":
			sensor = "all"
		elif sensor in i:
			sensor = i[sensor]
		return render.sensor(feature, sensor)

if __name__ == "__main__":
	app = web.application(urls, globals())
	app.run()
