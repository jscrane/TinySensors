(ns sensing.ui
  (:require
    (seesaw (core :as s) (keystroke :as k))
    (clj-time (core :as t)))
  (:import
    (org.jfree.chart ChartPanel)
    (org.joda.time Hours))
  (:use
    [sensing.data :only [sensors locations]]
    [sensing.charts :only [make-initial-chart make-chart add-location]]))

(defn- period [units length]
  (let [off (.toStandardSeconds (units length))]
    [off off]))

; val is [offset duration]
(def periods {"6h"  (period t/hours 6),
              "12h" (period t/hours 12),
              "1d"  (period t/days 1),
              "2d"  (period t/days 2),
              "1w"  (period t/weeks 1),
              "4w"  (period t/weeks 4)})

(def plot-area (atom nil))
(def curr-locations (atom #{3}))
(def curr-sensor (atom :light))
(def curr-period (atom (periods "6h")))

(defn make-plot [sensor locations period]
  (.setChart @plot-area (make-chart sensor locations period)))

(defn make-initial-plot [sensor locations period]
  (reset! plot-area (ChartPanel. (make-initial-chart sensor (first locations) period))))

(defn location-action [id]
  (let [l @curr-locations]
    (if (contains? l id)
      (make-plot @curr-sensor (reset! curr-locations (disj l id)) @curr-period)
      (let [chart (add-location (.getChart @plot-area) @curr-sensor id @curr-period)
            _ (reset! curr-locations (conj l id))]
        chart))))

(defn sensor-action [id]
  (make-plot (reset! curr-sensor id) @curr-locations @curr-period))

(defn period-action [p]
  (make-plot @curr-sensor @curr-locations (reset! curr-period p)))

(defn prev-action []
  (let [[o d] @curr-period]
    (period-action [(.plus o d) d])))

(defn next-action []
  (let [[o d] @curr-period
        n (.minus o d)]
    (period-action [(if (.isGreaterThan n Hours/ZERO) n o) d])))

(defn zoom-out-action []
  (let [[o d] @curr-period]
    (period-action [o (.plus d d)])))

(defn -main [& args]
  (s/invoke-later
    (-> (s/frame :title "Sensors",
                 :content (make-initial-plot @curr-sensor @curr-locations @curr-period),
                 :on-close :exit
                 :menubar (s/menubar
                            :items
                            [(s/menu :text "File"
                                     :mnemonic \F
                                     :items [(s/action :name "Quit"
                                                       :mnemonic \Q
                                                       :key (k/keystroke "ctrl Q")
                                                       :handler (fn [e] (System/exit 0)))])
                             (s/menu :text "Location"
                                     :mnemonic \L
                                     :items (map (fn [[k v]]
                                                   (s/checkbox-menu-item :text v
                                                                         ;:group g
                                                                         :selected? (contains? @curr-locations k)
                                                                         :listen [:action (fn [e] (location-action k))]))
                                                 locations))
                             (s/menu :text "Sensor"
                                     :mnemonic \S
                                     :items (let [g (s/button-group)]
                                              (map (fn [[k v]]
                                                     (s/radio-menu-item :text v
                                                                        :group g
                                                                        :selected? (= k @curr-sensor)
                                                                        :listen [:action (fn [e] (sensor-action k))]))
                                                   sensors)))
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
                                                 (s/action :name "Prev"
                                                           :mnemonic \P
                                                           :key (k/keystroke "LEFT")
                                                           :handler (fn [e] (prev-action)))
                                                 (s/action :name "Next"
                                                           :mnemonic \N
                                                           :key (k/keystroke "RIGHT")
                                                           :handler (fn [e] (next-action)))
                                                 (s/action :name "Out"
                                                           :mnemonic \O
                                                           :key (k/keystroke "DOWN")
                                                           :handler (fn [e] (zoom-out-action)))
                                                 (s/action :name "Now"
                                                           :mnemonic \W
                                                           :key (k/keystroke "END")
                                                           :handler (fn [e] (period-action (periods "6h"))))])))]))
        s/pack!
        s/show!)))