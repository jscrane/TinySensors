import MySQLdb

def connect(info):
	#db = MySQLdb.connect(host="localhost", user="sensors", passwd="s3ns0rs", db="sensors")
	db = MySQLdb.connect(**info)
	return db.cursor()

def types(cur):
	cur.execute("SELECT * FROM device_types")
	types = {}
	for row in cur.fetchall():
		type = {}
		type['name'] = row[1];
		type['features'] = set(row[2].split())
		types[row[0]] = type
	return types

units = {
	'temperature': 'Degrees C',
	'humidity': '%',
	'light': '',
	'battery': 'Volts'
}

def sensors(cur):
	cur.execute("SELECT * FROM nodes")
	sensors = {}
	for row in cur.fetchall():
		sensor = {}
		sensor['name'] = row[1]
		sensor['type'] = row[2]
		sensor['short'] = row[3]
		sensor['colour'] = row[4]
		sensors[str(row[0])] = sensor
	return sensors
