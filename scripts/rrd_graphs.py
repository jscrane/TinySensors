#!/usr/bin/env python

import rrdtool
import sensordb

rrds = '/var/lib/rrd/'
pngs = '/var/www/static/'

db = sensordb.connect("/home/pi/TinySensors/auth")
sensors = sensordb.sensors(db)
types = sensordb.types(db)
units = sensordb.units
db.close()

def graph_one(column, units, interval, sensor_id, description, colour):
	sensor = rrds + 'sensor-' + sensor_id + '.rrd'
	png = pngs+column+'_'+sensor_id+'-'+interval+'.png'
	rrdtool.graph(png, '--imgformat', 'PNG',
			'--start', '-1'+interval,
			'--title', description+' :: last '+interval,
			'--height', '80', '--width', '600',
			'--vertical-label', units, '--slope-mode',
			'DEF:'+column+'='+sensor+':'+column+':AVERAGE',
			'DEF:min='+sensor+':'+column+':MIN',
			'DEF:max='+sensor+':'+column+':MAX',
			'LINE2:'+column+colour+':'+column,
			'GPRINT:'+column+':MAX:    Max\\: %4.1lf',
			'GPRINT:'+column+':MIN:    Min\\: %4.1lf',
			'GPRINT:'+column+':AVERAGE: Avg\\: %4.1lf',
			'GPRINT:'+column+':LAST:    Now\\: %4.1lf')

def has_feature(sensor, column):
	return column in types[sensor['type']]['features']

def graph_all(column, interval):
	for id,s in sensors.items():
		if has_feature(s, column):
			graph_one(column, units[column], interval, id, s['name'], s['colour'])

def graph_combined(column, interval):
	defs = []
	lines = []
	for id,s in sensors.items():
		if has_feature(s, column):
			name = s['name']
			short = s['short']
			colour = s['colour']
			rrd = rrds+'sensor-'+id+'.rrd'
			defs.append('DEF:'+short+'='+rrd+':'+column+':AVERAGE')
			lines.append('LINE2:'+short+colour+':'+name)
	png = pngs + column + '_all-' + interval + '.png'
	rrdtool.graph(png, '--imgformat', 'PNG',
			'--start', '-1'+interval,
			'--title', 'Everywhere :: last '+interval,
			'--height', '80', '--width', '600',
			'--vertical-label', units[column], '--slope-mode',
			defs, lines)

for u in units:
	graph_all(u, 'day')
	graph_combined(u, 'day')
	graph_all(u, 'month')
	graph_combined(u, 'month')
	graph_all(u, 'year')
	graph_combined(u, 'year')
