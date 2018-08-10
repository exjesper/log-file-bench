package main

import (
	"fmt"
        "os"
        "os/signal"
        "time"
)

func log(done chan bool) {
  for {
    select {
      case <- done:
      default:
         time.Sleep(100 * time.Microsecond)
         fmt.Printf("ogihaijaosidjaosidjaoisjdoaisjdoaisjdaoisjdoiajsdoiajsdoiajsdoiasjdoiajsdoiasjdoaisjdaoisdjaoisjsijgoihaoidhaoijaoiaioinaoidnaiaionsd\n")
   }
  }
}

func main() {
        done := make(chan bool)
        c := make(chan os.Signal, 1)
        signal.Notify(c, os.Interrupt)
        go func(){
           for range c {
             done <- true
             done <- true
             done <- true
             done <- true
             done <- true
           }
        }()

	go log(done)
        go log(done)
        go log(done)
        go log(done)
	<- done
}
