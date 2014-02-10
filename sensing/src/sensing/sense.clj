(ns sensing.sense
  (:require
    (korma (db :as db) (core :as k))
    (incanter (charts :as charts) (stats :as stats))
    (seesaw (core :as s))
    (clj-time (core :as t) (local :as local) (coerce :as coerce)))
  (:import
    (org.jfree.chart ChartPanel)
    ))

(db/defdb sensors (db/mysql {:db "sensors" :user "sensors" :password "s3ns0rs" :host "rho" :delimiters ""}))
(k/defentity sensordata (k/database sensors))

(def b (-> (k/select* sensordata) (k/fields :time :battery :light :humidity :temperature) (k/order :time)))

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

(defn- avg [n data]
  (map stats/mean (partition n data)))

(defn make-plot [key data]
  (ChartPanel. (charts/time-series-plot
                 (avg 60 (map #(.getTime (:time %)) data))
                 (avg 60 (map key data))
                 :x-label "Time" :y-label (str key))))

(defn -main [& args]
  (s/invoke-later
    (-> (s/frame :title "Sensors",
               :content (make-plot :light (query-range b 2)),
               :on-close :exit)
        s/pack!
        s/show!)))