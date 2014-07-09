(ns sensing.misc
  (:require
    [clj-time [core :as t] [local :as local]]))

; computes a rolling average of the data
(defn avg [n data]
  (let [f (take n data)]
    (first
      (reduce (fn [[r w s] d]
                (let [s (+ s d (- (w 0)))]
                  [(conj r (/ s n)) (conj (subvec w 1) d) s]))
              [[] (vec f) (apply + f)]
              (drop n data)))))

(defn- valid [key data]
  (let [ranges {:light [0 255], :battery [0 1.55], :humidity [0 100], :temperature [0 30]}
        [lo hi] (ranges key)]
    (filter (fn [d] (and (>= d lo) (<= d hi))) data)))

(defn get-time [data]
  (map #(.getTime (:time %)) data))

; selects the key's value from a sequence of maps, checks it for validity and computes a rolling average of it
(defn smooth [key data]
  (->> data
       (map key)
       (valid key)
       (avg (inc (int (/ (count data) 250))))))

(defn period [units length]
  (let [dur (.toStandardSeconds (units length))
        now (local/local-now)]
    [(t/minus now dur) dur]))

