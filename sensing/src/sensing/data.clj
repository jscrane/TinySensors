(ns sensing.data
  (:require
    (korma (db :as db) (core :as k))
    (incanter (charts :as charts))
    (clj-time (core :as t) (local :as local) (coerce :as coerce))))

(db/defdb sensordb (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensordb))
(k/defentity nodes (k/database sensordb))
(k/defentity weather (k/database sensordb))

(def dataq (-> (k/select* sensordata)
               (k/fields :time :battery :light :humidity :temperature)
               (k/order :time)))
(def nodeq (-> (k/select* nodes) (k/fields :id :location)))
(def weatherq (-> (k/select* weather)
                  (k/fields :temperature :humidity :visibility :pressure :feels-like :direction :speed :gust :icon :time)
                  (k/order :time)))

(defn- unixtime [d]
  (long (/ (coerce/to-long d) 1000)))

(defn- query-window [q [off dur]]
  (let [now (local/local-now)
        start (t/minus now off)
        end (t/plus start dur)]
    (-> q
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

(def sensors {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})

(def locations (reduce (fn [m {:keys [id location]}] (assoc m id location)) {} (k/select nodeq)))

(defn query-location [id time-window]
  (query-window (-> dataq (k/where {:node_id [= id]})) time-window))

(defn query-weather [time-window]
  (query-window weatherq time-window))

(defn make-chart [key data]
  (let [valid {:light [0 255], :battery [0 1.5], :humidity [0 100], :temperature [0 30]}]
    (charts/time-series-plot
      (map #(.getTime (:time %)) data)
      (avg (map key data) (valid key))
      :x-label "Time" :y-label (sensors key))))
