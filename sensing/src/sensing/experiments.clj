(ns sensing.experiments
  (:require
    [clj-time [core :as t] [local :as l] [coerce :as c]]
    [incanter [charts :as charts] [core :as i] [stats :as stats]])
  (:import
    [org.joda.time Hours]
    [java.util Date])
  (:use
    [sensing.data :only [sensors locations query-location query-weather]]
    [sensing.misc :only [get-time smooth period]]))

(def month (period t/weeks 4))

(def wthr (query-weather 0 month))

(def bedroom-temp (query-location 30 :temperature [(c/from-long (.getTime (:time (first wthr)))) (second month)]))

; returns a vector of indices in w for times in s
(defn windex [w s]
  (let [now (Date.)]
    (loop [s s v [] i 0]
      (if (empty? s)
        v
        (let [s1 (:time (first s))
              i1 (inc i)
              w1 (if (= i1 (count w)) now (:time (w i1)))
              ni (if (.after w1 s1) i i1)]
          (recur (rest s) (conj v ni) ni))))))

(def wi (windex wthr bedroom-temp))

(defn select [key wthr wi]
  (map #(key (nth wthr %)) wi))

(def t (get-time bedroom-temp))
(def bt (map :temperature bedroom-temp))
(def ot (select :temperature wthr wi))

(def c (charts/time-series-plot t bt :x-label "Time" :y-label "Temperature" :series-label "Bedroom" :legend true))
(def c (charts/add-lines c t ot :series-label "Outside"))

(i/view c)

(def lm (stats/linear-model bt ot))

; 0.166
(:r-square lm)

; 0.408
(stats/correlation bt ot)

(def wc (select :feels_like wthr wi))

; 0.5428222860036505
(stats/correlation bt wc)

(def owt (select (fn [w] (if (> (:direction w) 180) (:feels_like w) (:temperature w))) wthr wi))

; 0.5435896426329951
(stats/correlation bt owt)

(def wind (select (fn [w]
                    (let [d (- (:direction w) 180)
                          s (:speed w)]
                      (if (neg? d)
                        0
                        (* s (Math/sin (/ (* Math/PI d) 180))))))
                  wthr wi))

(def hum (select :humidity wthr wi))
(def pre (select :pressure wthr wi))

(def m (i/trans (i/matrix [ot wind wc hum pre])))

; 0.7051798366700187
(Math/sqrt (:r-square (stats/linear-model bt m)))
