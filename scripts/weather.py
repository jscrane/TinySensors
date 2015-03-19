#!/usr/bin/python

import pywapi
import pprint
import sensordb

pp = pprint.PrettyPrinter(indent=2)
db = sensordb.connect("/home/pi/TinySensors/auth")

cursor = db.cursor()
cursor.execute("SELECT id,code FROM stations")
stations = cursor.fetchall()

for id, code in stations:
    result = pywapi.get_weather_from_weather_com(code)
    w = result['current_conditions']
    pp.pprint(w)

    cursor.execute("""
	INSERT INTO weather_data(last_updated,temperature,humidity,visibility,pressure,feels_like,direction,speed,gust,icon,station_id)
	VALUES(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)
	""", (w['last_updated'], w['temperature'], w['humidity'], w['visibility'], w['barometer']['reading'], w['feels_like'], w['wind']['direction'], w['wind']['speed'], w['wind']['gust'], w['icon'], id))

cursor.close()
db.commit()
db.close()
