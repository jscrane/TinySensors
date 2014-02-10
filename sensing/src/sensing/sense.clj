(ns sensing.sense
  (:require
    (korma (db :as db) (core :as k))
    (incanter (charts :as charts) (stats :as stats))
    (seesaw (core :as s))
    (clj-time (core :as t) (local :as local) (coerce :as coerce)))
  (:import
    (org.jfree.chart ChartPanel)))

(db/defdb sensors (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensors))
(k/defentity nodes (k/database sensors))

(def dataq (-> (k/select* sensordata)
               (k/fields :time :battery :light :humidity :temperature)
               (k/order :time)))
(def nodeq (-> (k/select* nodes) (k/fields :id :location)))

(defn- unixtime [d]
  (long (/ (coerce/to-long d) 1000)))

(defn query-range
  ([q id start end]
   (-> q
       (k/where {:node_id [= id]})
       (k/where {:time [> (k/sqlfn from_unixtime (unixtime start))]})
       (k/where {:time [< (k/sqlfn from_unixtime (unixtime end))]})
       (k/select)))
  ([q id start]
   (query-range q id start (local/local-now)))
  ([q id]
   (let [now (local/local-now)]
     (query-range q id (t/minus now (t/hours 8)) now))
   ;(-> q (k/select))
   )
  )

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

(def plot-area (atom nil))
(def labels {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})
(def ranges {:light [0 255], :battery [0 1.5], :humidity [0 100], :temperature [0 30]})

(defn make-plot [key data]
  (let [chart (charts/time-series-plot
                (map #(.getTime (:time %)) data)
                (avg (map key data) (ranges key))
                :x-label "Time" :y-label (labels key))]
    (if @plot-area
      (do
        (.setChart @plot-area chart)
        @plot-area)
      (reset! plot-area (ChartPanel. chart)))))

(def sensor (atom 2))
(def parameter (atom :light))

(defn sensor-action [id e]
  (make-plot @parameter (query-range dataq (reset! sensor id))))

(defn view-action [id e]
  (make-plot (reset! parameter id) (query-range dataq @sensor)))

(defn -main [& args]
  (s/invoke-later
    (-> (s/frame :title "Sensors",
                 :content (make-plot @parameter (query-range dataq @sensor)),
                 :on-close :exit
                 :menubar (s/menubar
                            :items
                            [(s/menu :text "File" :items [(s/action :name "Quit" :handler (fn [e] (System/exit 0)))])
                             (s/menu :text "View" :items (map (fn [[k v]]
                                                                (s/action :name v :handler (partial view-action k)))
                                                              labels))
                             (s/menu :text "Sensor" :items (map (fn [{:keys [id location]}]
                                                                  (s/action :name location :handler (partial sensor-action id)))
                                                                (k/select nodeq)))]))
        s/pack!
        s/show!)))