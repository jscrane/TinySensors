(ns sensing.experiments2)

; http://programmablehardware.blogspot.ie/2014/04/power-consumption.html
(require '(korma (core :as k)))
(require '(incanter (charts :as charts) (core :as i)))
(use 'sensing.data)

(defn get-time [data]
  (map #(.getTime (:time %)) data))

(def d (-> (k/select* sensordata) (k/fields :time :node_ms :msg_id :battery) (k/where (and (= :node_id 2) (>= :id 366470) (<= :id 1982461))) (k/order :time) (k/select)))

(def e (filter #(> 0.75 (:battery %)) d))
(def f (filter #(> 1000 (:msg_id %)) e))
(def t (map #(/ (.getTime (:time %)) 1000) f))
(def td (map #(/ (.getTime (:time %)) 1000) d))
(def te (map #(/ (.getTime (:time %)) 1000) e))
(def dele (map #(- (second %) (first %)) (partition 2 1 te)))
(def deld (map #(- (second %) (first %)) (partition 2 1 td)))
(def del (map #(- (second %) (first %)) (partition 2 1 t)))

(def m (map #(/ (:node_ms %) 1000) f))
(def delm (map #(- (second %) (first %)) (partition 2 1 m)))
(i/view (charts/time-series-plot (get-time f) (take 130 delm) :x-label "Time" :y-label "Inter-Transmission Time (s)"))

; viewing interarrival times from all sensors (to discover max time for watchdog)
(let [q (-> (k/select* sensordata)
            (k/fields :time)
            (k/where {:time [> (k/sqlfn from_unixtime (unixtime "2014-08-10 15:00:00"))]})
            (k/order :time) (k/select))
      counts (->> q
             (map (comp unixtime :time))
             (partition 2 1)
             (map (fn [[f s]] (- s f)))
             (sort)
             (partition-by identity)
             (reduce (fn [m coll] (assoc m (first coll) (count coll))) (sorted-map)))]
  (i/view (charts/bar-chart (keys counts) (vals counts))))
