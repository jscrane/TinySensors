(ns sensing.ui
  (:require
    (seesaw (core :as s))
    (clj-time (core :as t)))
  (:import
    (org.jfree.chart ChartPanel))
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

(def periods {"6h"  [(t/hours 6) (t/hours 6)],
              "12h" [(t/hours 12) (t/hours 12)],
              "1d"  [(t/days 1) (t/days 1)],
              "2d"  [(t/days 2) (t/days 2)],
              "1w"  [(t/weeks 1) (t/weeks 1)],
              "4w"  [(t/weeks 4) (t/weeks 4)]})

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
    (period-action [(if (pos? n) n o) d])))

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
                                                           :handler (fn [e] (prev-action)))
                                                 (s/action :name "Next"
                                                           :mnemonic \N
                                                           :handler (fn [e] (next-action)))
                                                 (s/action :name "Now"
                                                           :mnemonic \W
                                                           :handler (fn [e] (period-action (periods "6h"))))])))]))
        s/pack!
        s/show!)))