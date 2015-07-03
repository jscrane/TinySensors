(defproject sensors "0.0.1-SNAPSHOT"
  :description "Remote sensing"

  :global-vars {*warn-on-reflection* true}

  :dependencies [[org.clojure/clojure "1.7.0"]
                 [korma "0.4.0"]
                 [mysql/mysql-connector-java "5.1.27"]
                 [log4j "1.2.17" :exclusions [javax.mail/mail
                                             javax.jms/jms
                                             com.sun.jdmk/jmxtools
                                             com.sun.jmx/jmxri]]
                 [incanter/incanter-core "1.5.4"]
                 [incanter/incanter-charts "1.5.4"]
                 [seesaw "1.4.5"]
                 [clj-time "0.6.0"]]

  :main sensing.ui
  :aot [sensing.ui])
