#include <Arduino.h>

namespace logger {
  void init() { 
    Serial.begin(9600);
  }
  String timeString() {
    const long totalSeconds = millis() / 1000;
    const long seconds = totalSeconds % 60;
    const long minutes = (totalSeconds/60) % 60;
    const long hours = (totalSeconds/60/60) % 24;
    const long days = (totalSeconds/60/60/24);
    char logString[11];
    sprintf(logString, "%04dT%02d:%02d:%02d", days, hours, minutes, seconds);
    return logString;
  }
  
  void info(String message) {
    Serial.println(timeString() + " INFO: " + message);
  }

  void error(String message) {
    Serial.println(timeString() + " ERROR: " + message);
  }
}

namespace errors {
  
  struct Error {
    String message;
    bool ok;
    
    Error(): message("<err>"), ok(false) {} 
    Error(String message): message(message) {} 
    Error(String message, Error error): message(message + ": " + error.toString()) {} 

    bool isOk() const {
      return ok;
    }

    String toString() {
      return message;
    }
  };

  
  Error ok() {
    Error ok("ok");
    ok.ok = true;
    return ok;
  }

  void logIfError(Error e) {
    if (!e.isOk()) {
      logger::error(e.toString());
    }
  }
}

namespace timer {
  class Timer {
    public:
      Timer();
      Timer(int);
      boolean isActive();
      boolean isExpired();
      void setTimer(int);
      

    private:
      unsigned long startTime;
      unsigned long seconds; 
  };

  const Timer TimerInaktiv = Timer();

  Timer::Timer(){
    setTimer(0);
  }
  

  Timer::Timer(int aSeconds){
    setTimer(aSeconds);
  }
  
  void Timer::setTimer(int aSeconds){
    seconds = aSeconds;
    startTime = millis();
  }

  boolean Timer::isActive() {
    return seconds != 0;
  }
  
  boolean Timer::isExpired(){
    if (!isActive()) {
      return false;
    }
    if(millis() - startTime >= seconds * 1000){
      return true;
    }
    return false;
  }

}

namespace storage {

  extern "C" {
    #include "user_interface.h" // this is for the RTC memory read/write functions
  }

  
  class PersitentStorage {
    public:
      PersitentStorage(): rtcPostion(65) {};
      PersitentStorage(int pos): rtcPostion(pos) {};
      void restore();
      void store(uint32_t data);
      uint32_t getData();
      
    private:
      uint32_t data = 0;
      const int rtcPostion;
  };

  uint32_t PersitentStorage::getData() {
     return data;
  }
  


  void PersitentStorage::store(uint32_t data) {
     data = data;
     system_rtc_mem_write(rtcPostion, &data, 4);
  }
  
  void PersitentStorage::restore() {
     if (ESP.getResetReason() == String("Deep-Sleep Wake")) {
       system_rtc_mem_read(rtcPostion, &data, 4);
     }
  }
}
