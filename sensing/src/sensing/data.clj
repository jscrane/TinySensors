(ns sensing.data
  (:require
      (korma (db :as db) (core :as k))
      (incanter (charts :as charts))
      (clj-time (core :as t) (local :as local) (coerce :as coerce))))

(db/defdb sensors (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensors))
(k/defentity nodes (k/database sensors))

(def dataq (-> (k/select* sensordata)
               (k/fields :time :battery :light :humidity :temperature)
               (k/order :time)))
(def nodeq (-> (k/select* nodes) (k/fields :id :location)))

(defn- unixtime [d]
  (long (/ (coerce/to-long d) 1000)))

(defn query-window [id [off dur]]
  (let [now (local/local-now)
        start (t/minus now off)
        end (t/plus start dur)]
    (-> dataq
        (k/where {:node_id [= id]})
        (k/where {:time [> (k/sqlfn from_unixtime (unixtime start))]})
        (k/where {:time [< (k/sqlfn from_unixtime (unixtime end))]})
        (k/select))))

; computes a rolling average of the data filtering data outside range
(defn- avg
  ([n data [lo hi]]
   (let [f (first data)]
     (first
       (reduce (fn [[r w s] d]
                 (let [a (/ s n)
                       d (if (and (>= d lo) (<= d hi)) d a)]
                   [(conj r a) (conj (subvec w 1) d) (+ s d (- (w 0)))]))
               [[] (vec (repeat n f)) (* n f)] (drop n data)))))
  ([data range]
   (avg (int (/ (count data) 200)) data range)))

(def sensor-names {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})
(def valid {:light [0 255], :battery [0 1.5], :humidity [0 100], :temperature [0 30]})

(def nodes (k/select nodeq))

(defn make-chart [key data]
  (charts/time-series-plot
    (map #(.getTime (:time %)) data)
    (avg (map key data) (valid key))
    :x-label "Time" :y-label (sensor-names key)))
