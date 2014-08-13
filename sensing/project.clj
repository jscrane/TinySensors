(defproject sensors "0.0.1-SNAPSHOT"
  :description "Remote sensing"

  :warn-on-reflection true

  :dependencies [[org.clojure/clojure "1.6.0"]
                 [korma "0.3.3"]
                 [mysql/mysql-connector-java "5.1.27"]
                 [log4j "1.2.15" :exclusions [javax.mail/mail
                                             javax.jms/jms
                                             com.sun.jdmk/jmxtools
                                             com.sun.jmx/jmxri]]
                 [incanter/incanter-core "1.5.4"]
                 [incanter/incanter-charts "1.5.4"]
                 [seesaw "1.4.4"]
                 [clj-time "0.6.0"]]

  :main sensing.ui)
