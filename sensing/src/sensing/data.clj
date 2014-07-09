(ns sensing.data
  (:require
    [clojure [string :as str]]
    [korma [db :as db] [core :as k]]
    [clj-time.core :as t]
    [clj-time.coerce :as coerce]))

(db/defdb sensordb (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensordb) (k/table :sensor_data))
(k/defentity nodes (k/database sensordb))
(k/defentity sensortypes (k/database sensordb) (k/table :device_types))
(k/defentity weatherdata (k/database sensordb) (k/table :weather_data))
(k/defentity stations (k/database sensordb))

(defn- unixtime [d]
  (long (/ (coerce/to-long d) 1000)))

(defn- window [q [start dur]]
  (let [end (.plus start dur)]
    (-> q
        (k/where {:time [> (k/sqlfn from_unixtime (unixtime start))]})
        (k/where {:time [< (k/sqlfn from_unixtime (unixtime end))]}))))

(defn query-location [location-id sensor time-window]
  (-> (k/select* sensordata)
      (k/fields :time sensor)
      (k/where {:node_id [= location-id]})
      (k/where {:status 0})
      (k/order :time)
      (window time-window)
      (k/select)))

(defn query-locations [location-ids sensor time-window]
  (partition-by :node_id
                (-> (k/select* sensordata)
                    (k/fields :time :node_id sensor)
                    (k/where {:node_id [in location-ids]})
                    (k/where {:status 0})
                    (k/order :node_id)
                    (k/order :time)
                    (window time-window)
                    (k/select))))

(defn query-weather [station-id time-window]
  (-> (k/select* weatherdata)
      (k/fields :temperature :humidity :visibility :pressure :feels_like :direction :speed :gust :icon :time)
      (k/where {:station_id [= station-id]})
      (k/order :time)
      (window time-window)
      (k/select)))

(def sensors {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})

; map from device_type_id to keyworded-features
(def device-types
  (apply array-map
         (flatten
           (map (fn [{:keys [id features]}]
                  [id (into #{} (map keyword (str/split (.toLowerCase features) #", ")))])
                (k/select sensortypes (k/fields :id :features))))))

; map from description to node_id
(def locations
  (apply array-map
         (apply concat
           (map (fn [{:keys [id location device_type_id]}]
                  [id [location device_type_id]])
                (k/select nodes (k/fields :id :location :device_type_id) (k/order :device_type_id))))))

(defn- device-types-with-sensor [sensor-id]
  (into #{} (map first (filter (fn [[_ v]] (contains? v sensor-id)) device-types))))

(defn locations-with-sensor [sensor-id]
  (let [t (device-types-with-sensor sensor-id)]
    (into #{} (map first (filter (fn [[_ [_ type]]] (contains? t type)) locations)))))
