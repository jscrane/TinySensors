#!/usr/bin/python

import MySQLdb
import pywapi
import pprint

pp = pprint.PrettyPrinter(indent=2)

result = pywapi.get_weather_from_weather_com('EIXX0014')
w = result['current_conditions']
pp.pprint(w)

db = MySQLdb.connect(host='localhost', user='sensors', passwd='s3ns0rs', db='sensors')
cursor = db.cursor()

cursor.execute("""
	INSERT INTO weather(last_updated,temperature,humidity,visibility,pressure,feels_like,direction,speed,gust,icon)
	VALUES(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)
	""", (w['last_updated'], w['temperature'], w['humidity'], w['visibility'], w['barometer']['reading'], w['feels_like'], w['wind']['direction'], w['wind']['speed'], w['wind']['gust'], w['icon']))

cursor.close()
db.commit()
db.close()
