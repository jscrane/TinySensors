USER=sensors
PASSWORD=s3ns0rs

dump: schema data

schema:
	mysqldump -u $(USER) -p$(PASSWORD) sensors --compact --no-data > schema.sql

data:
	mysqldump -u $(USER) -p$(PASSWORD) sensors --compact --no-create-info --ignore-table=sensors.weather --ignore-table=sensors.sensordata > data.sql
