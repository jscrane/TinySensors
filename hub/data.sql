LOCK TABLES `device_types` WRITE;
/*!40000 ALTER TABLE `device_types` DISABLE KEYS */;
INSERT INTO `device_types` VALUES (0,'Wireless Sensor v1'),(1,'Relay'),(2,'MS-TL 1-wire'),(3,'T-Sense 1-wire'),(4,'MS_T 1-wire');
/*!40000 ALTER TABLE `device_types` ENABLE KEYS */;
UNLOCK TABLES;

LOCK TABLES `nodes` WRITE;
/*!40000 ALTER TABLE `nodes` DISABLE KEYS */;
INSERT INTO `nodes` VALUES (1,'Relay',1),(2,'Ensuite',0),(3,'Kitchen',0),(4,'Bathroom',0),(5,'Study',0),(9,'Unknown',0),(20,'Bedroom Window',2),(21,'Hall',2),(30,'Bedroom',3),(31,'Study #2',3),(33,'Living Room',3),(34,'Boiler',3),(40,'Hot Press',4);
/*!40000 ALTER TABLE `nodes` ENABLE KEYS */;
UNLOCK TABLES;
