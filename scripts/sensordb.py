import MySQLdb

def connect(file):
	auth = {}
	with open(file) as myfile:
		for line in myfile:
        		name, var = line.partition("=")[::2]
			auth[name] = var.strip()
	db = MySQLdb.connect(host='localhost', user=auth['USER'], passwd=auth['PASSWORD'], db=auth['DB'])
	return db

def types(db):
	cur = db.cursor()
	cur.execute("SELECT * FROM device_types")
	types = {}
	for row in cur.fetchall():
		type = {}
		type['name'] = row[1];
		type['features'] = set(row[2].split())
		types[row[0]] = type
	cur.close()
	return types

units = {
	'temperature': 'Degrees C',
	'humidity': '%',
	'light': '',
	'battery': 'Volts'
}

def sensors(db):
	cur = db.cursor()
	cur.execute("SELECT * FROM nodes")
	sensors = {}
	for row in cur.fetchall():
		sensor = {}
		sensor['name'] = row[1]
		sensor['type'] = row[2]
		sensor['short'] = row[3]
		sensor['colour'] = row[4]
		sensors[str(row[0])] = sensor
	cur.close()
	return sensors
