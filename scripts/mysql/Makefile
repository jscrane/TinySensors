include ../../auth

dump: schema data

schema:
	mysqldump -u $(USER) -p$(PASSWORD) sensors --compact --no-data > schema.sql

data:
	mysqldump -u $(USER) -p$(PASSWORD) sensors --compact --no-create-info --ignore-table=sensors.weather_data --ignore-table=sensors.sensor_data > data.sql
