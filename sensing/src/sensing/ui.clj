(ns sensing.ui
  (:gen-class)
  (:require
    [clojure [set :as set]]
    [seesaw [core :as s] [keystroke :as k]]
    [clj-time.core :as t])
  (:import
    (org.jfree.chart ChartPanel)
    (org.joda.time Hours))
  (:use
    [sensing.data :only [sensors locations locations-with-sensor]]
    [sensing.charts :only [make-initial-chart make-chart add-location]]
    [sensing.misc :only [period]]))

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

(defn sensor-action [e id]
  (let [a (into #{} (keys locations))
        w (locations-with-sensor id)
        wo (set/difference a w)
        r (s/to-root e)
        enable! (fn [bool coll]
                  (dorun (map #(s/config! (s/select r [(keyword (str "#" %))]) :enabled? bool) coll)))]
    (enable! false wo)
    (enable! true w)
    (make-plot (reset! curr-sensor id) (reset! curr-locations (set/difference @curr-locations wo)) @curr-period)))

(defn period-action [p]
  (make-plot @curr-sensor @curr-locations (reset! curr-period p)))

(defn prev-action []
  (let [[start dur] @curr-period]
    (period-action [(.minus start dur) dur])))

(defn next-action []
  (let [[start dur] @curr-period
        n (.plus start dur)]
    (if (.isBeforeNow n)
      (period-action [n dur]))))

(defn zoom-out-action []
  (let [[o d] @curr-period]
    (period-action [o (.plus d d)])))

(defn -main [& _]
  (let [enabled-locations (locations-with-sensor @curr-sensor)]
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
                                                         :handler (fn [_] (System/exit 0)))])
                               (s/menu :text "Sensor"
                                       :mnemonic \S
                                       :items (let [g (s/button-group)]
                                                (map (fn [[k v]]
                                                       (s/radio-menu-item :text v
                                                                          :group g
                                                                          :selected? (= k @curr-sensor)
                                                                          :listen [:action (fn [e] (sensor-action e k))]))
                                                     sensors)))
                               (s/menu :text "Location"
                                       :mnemonic \L
                                       :items (map (fn [[k v]]
                                                     (s/checkbox-menu-item :text (first v)
                                                                           :id (str k)
                                                                           :enabled? (contains? enabled-locations k)
                                                                           :selected? (contains? @curr-locations k)
                                                                           :listen [:action (fn [_] (location-action k))]))
                                                   locations))
                               (s/menu :text "Period"
                                       :mnemonic \P
                                       :items (let [g (s/button-group)]
                                                (concat
                                                  (map (fn [[k v]]
                                                         (s/radio-menu-item :text k
                                                                            :group g
                                                                            :selected? (= v @curr-period)
                                                                            :listen [:action (fn [_] (period-action v))]))
                                                       periods)
                                                  [(s/separator)
                                                   (s/action :name "Prev"
                                                             :mnemonic \P
                                                             :key (k/keystroke "LEFT")
                                                             :handler (fn [_] (prev-action)))
                                                   (s/action :name "Next"
                                                             :mnemonic \N
                                                             :key (k/keystroke "RIGHT")
                                                             :handler (fn [_] (next-action)))
                                                   (s/action :name "Out"
                                                             :mnemonic \O
                                                             :key (k/keystroke "DOWN")
                                                             :handler (fn [_] (zoom-out-action)))
                                                   (s/action :name "Now"
                                                             :mnemonic \W
                                                             :key (k/keystroke "END")
                                                             :handler (fn [_] (period-action (periods "6h"))))
                                                   (s/action :name "Refresh"
                                                             :mnemonic \R
                                                             :key (k/keystroke "F5")
                                                             :handler (fn [_] (make-plot @curr-sensor @curr-locations @curr-period)))])))]))
          s/pack!
          s/show!))))
