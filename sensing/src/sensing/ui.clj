(ns sensing.ui
  (:require
    (seesaw (core :as s) (keystroke :as k))
    (clj-time (core :as t)))
  (:import
    (org.jfree.chart ChartPanel)
    (org.joda.time Hours))
  (:use
    [sensing.data :only (make-chart query-location sensors locations)]))

(def plot-area (atom nil))

(defn make-plot [key data]
  (let [chart (make-chart key data)]
    (if @plot-area
      (do
        (.setChart @plot-area chart)
        @plot-area)
      (reset! plot-area (ChartPanel. chart)))))

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

(def curr-location (atom 3))
(def curr-sensor (atom :light))
(def curr-period (atom (periods "6h")))

(defn location-action [id]
  (make-plot @curr-sensor (query-location (reset! curr-location id) @curr-period)))

(defn sensor-action [id]
  (make-plot (reset! curr-sensor id) (query-location @curr-location @curr-period)))

(defn period-action [p]
  (make-plot @curr-sensor (query-location @curr-location (reset! curr-period p))))

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
                 :content (make-plot @curr-sensor (query-location @curr-location @curr-period)),
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
                                     :items (let [g (s/button-group)]
                                              (map (fn [[k v]]
                                                     (s/radio-menu-item :text v
                                                                        :group g
                                                                        :selected? (= k @curr-location)
                                                                        :listen [:action (fn [e] (location-action k))]))
                                                   locations)))
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
                                                           :key (k/keystroke "DOWN")
                                                           :handler (fn [e] (zoom-out-action)))
                                                 (s/action :name "Now"
                                                           :mnemonic \W
                                                           :key (k/keystroke "END")
                                                           :handler (fn [e] (period-action (periods "6h"))))])))]))
        s/pack!
        s/show!)))