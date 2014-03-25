(ns sensing.data
  (:require
    (clojure (string :as str))
    (korma (db :as db) (core :as k))
    (clj-time (core :as t) (local :as local) (coerce :as coerce))))

(db/defdb sensordb (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensordb))
(k/defentity nodes (k/database sensordb))
(k/defentity device_types (k/database sensordb))
(k/defentity weather (k/database sensordb))

(defn- unixtime [d]
  (long (/ (coerce/to-long d) 1000)))

(defn- window [q [off dur]]
  (let [now (local/local-now)
        start (t/minus now off)
        end (t/plus start dur)]
    (-> q
        (k/where {:time [> (k/sqlfn from_unixtime (unixtime start))]})
        (k/where {:time [< (k/sqlfn from_unixtime (unixtime end))]}))))

(defn query-location [loc sensor time-window]
  (-> (k/select* sensordata)
      (k/fields :time sensor)
      (k/where {:node_id [= loc]})
      (k/where {:status 0})
      (k/order :time)
      (window time-window)
      (k/select)))

(defn query-locations [locs sensor time-window]
  (partition-by :node_id
                (-> (k/select* sensordata)
                    (k/fields :time :node_id sensor)
                    (k/where {:node_id [in locs]})
                    (k/where {:status 0})
                    (k/order :node_id)
                    (k/order :time)
                    (window time-window)
                    (k/select))))

(defn query-weather [time-window]
  (-> (k/select* weather)
      (k/fields :temperature :humidity :visibility :pressure :feels_like :direction :speed :gust :icon :time)
      (k/order :time)
      (window time-window)
      (k/select)))

; computes a rolling average of the data
(defn- avg [n data]
  (let [f (take n data)]
    (first
      (reduce (fn [[r w s] d]
                (let [s (+ s d (- (w 0)))]
                  [(conj r (/ s n)) (conj (subvec w 1) d) s]))
              [[] (vec f) (apply + f)]
              (drop n data)))))

(defn- valid [key data]
  (let [ranges {:light [0 255], :battery [0 1.55], :humidity [0 100], :temperature [0 30]}
        [lo hi] (ranges key)]
    (filter (fn [d] (and (>= d lo) (<= d hi))) data)))

(defn get-time [data]
  (map #(.getTime (:time %)) data))

(defn smooth [key data]
  (->> data
       (map key)
       (valid key)
       (avg (inc (int (/ (count data) 250))))))

(def sensors {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})

; map from device_type_id to keyworded-features
(def device-types
  (apply array-map
         (flatten
           (map (fn [{:keys [id features]}]
                  [id (into #{} (map keyword (str/split (.toLowerCase features) #", ")))])
                (k/select device_types (k/fields :id :features))))))

; map from description to node_id
(def locations
  (apply array-map
         (apply concat
           (map (fn [{:keys [id location device_type_id]}]
                  [id [location device_type_id]])
                (k/select nodes (k/fields :id :location :device_type_id) (k/order :device_type_id))))))

(defn- device-types-with-sensor [sens-id]
  (into #{} (map first (filter (fn [[k v]] (contains? v sens-id)) device-types))))

(defn locations-with-sensor [sens-id]
  (let [t (device-types-with-sensor sens-id)]
    (into #{} (map first (filter (fn [[id [name type]]] (contains? t type)) locations)))))
