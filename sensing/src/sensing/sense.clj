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

(defn query-window [q id [o d]]
  (let [now (local/local-now)
        start (t/minus now o)
        end (t/plus start d)]
    (-> q
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

(def plot-area (atom nil))
(def sensor-names {:light "Light", :battery "Battery", :humidity "Humidity", :temperature "Temperature"})
(def valid {:light [0 255], :battery [0 1.5], :humidity [0 100], :temperature [0 30]})
(def periods {"6h"  [(t/hours 6) (t/hours 6)],
              "12h" [(t/hours 12) (t/hours 12)],
              "1d"  [(t/days 1) (t/days 1)],
              "2d"  [(t/days 2) (t/days 2)],
              "1w"  [(t/weeks 1) (t/weeks 1)],
              "4w"  [(t/weeks 4) (t/weeks 4)]})

(defn make-plot [key data]
  (let [chart (charts/time-series-plot
                (map #(.getTime (:time %)) data)
                (avg (map key data) (valid key))
                :x-label "Time" :y-label (sensor-names key))]
    (if @plot-area
      (do
        (.setChart @plot-area chart)
        @plot-area)
      (reset! plot-area (ChartPanel. chart)))))

(def curr-location (atom 2))
(def curr-sensor (atom :light))
(def curr-period (atom (periods "12h")))

(defn location-action [id]
  (make-plot @curr-sensor (query-window dataq (reset! curr-location id) @curr-period)))

(defn sensor-action [id]
  (make-plot (reset! curr-sensor id) (query-window dataq @curr-location @curr-period)))

(defn period-action [p]
  (make-plot @curr-sensor (query-window dataq @curr-location (reset! curr-period p))))

(defn prev-action []
  (let [[o d] @curr-period]
    (period-action [(.plus o d) d])))

(defn next-action []
  (let [[o d] @curr-period
        n (.minus o d)]
    (period-action [(if (pos? n) n o) d])))

(defn -main [& args]
  (s/invoke-later
    (-> (s/frame :title "Sensors",
                 :content (make-plot @curr-sensor (query-window dataq @curr-location @curr-period)),
                 :on-close :exit
                 :menubar (s/menubar
                            :items
                            [(s/menu :text "File"
                                     :mnemonic \F
                                     :items [(s/action :name "Quit" :handler (fn [e] (System/exit 0)))])
                             (s/menu :text "Location"
                                     :mnemonic \L
                                     :items (let [g (s/button-group)]
                                              (map (fn [{:keys [id location]}]
                                                     (s/radio-menu-item :text location
                                                                        :group g
                                                                        :selected? (= id @curr-location)
                                                                        :listen [:action (fn [e] (location-action id))]))
                                                   (k/select nodeq))))
                             (s/menu :text "Sensor"
                                     :mnemonic \S
                                     :items (let [g (s/button-group)]
                                              (map (fn [[k v]]
                                                     (s/radio-menu-item :text v
                                                                        :group g
                                                                        :selected? (= k @curr-sensor)
                                                                        :listen [:action (fn [e] (sensor-action k))]))
                                                   sensor-names)))
                             (s/menu :text "Period"
                                     :mnemonic \P
                                     :items (let [g (s/button-group)]
                                              (concat
                                                (map (fn [[k v]]
                                                       (s/radio-menu-item :text k
                                                                          :group g
                                                                          :selected? (= v @curr-period)
                                                                          :listen [:action (fn [e] (period-action v))]))
                                                     periods)
                                                [(s/separator)
                                                 (s/menu-item :text "Prev"
                                                              :mnemonic \P
                                                              :listen [:action (fn [e] (prev-action))])
                                                 (s/menu-item :text "Next"
                                                              :mnemonic \N
                                                              :listen [:action (fn [e] (next-action))])])))]))
        s/pack!
        s/show!)))