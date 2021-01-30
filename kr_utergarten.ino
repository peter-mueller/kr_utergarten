
#include "querschnitt.h"

namespace nodemcu {
  // pins
  static const uint8_t D0   = 16;
  static const uint8_t D1   = 5;
  static const uint8_t D2   = 4;
  static const uint8_t D3   = 0;
  static const uint8_t D4   = 2;
  static const uint8_t D5   = 14;
  static const uint8_t D6   = 12;
  static const uint8_t D7   = 13;
  static const uint8_t D8   = 15;
  static const uint8_t D9   = 3;
  static const uint8_t D10  = 1;
}


namespace devices {

  struct Waterpump {
    int pin = -1;
    
    void turnOn() {
      digitalWrite(pin, HIGH);
    }
    
    void turnOff() {
      digitalWrite(pin, LOW);
    }
    
    errors::Error init() {
      if (pin == -1) {
        return errors::Error("Kein Pin für Waterpump konfiguriert");
      }
      pinMode(pin, OUTPUT);
      return errors::Ok();
    }
  };
  
  struct CapacativeSoilMoistureSensor {
    int pin = -1;
    
    int analogValueInWater = 310;
    int analogValueInAir = 620;
    int wetTresholdPercentage = 71;
    
    
    int getWettnessPercent() {
      const int value = analogRead(pin);
      const int percentage = map(value, analogValueInAir, analogValueInWater, 0, 100);
      return constrain(percentage, 0, 100);
    } 
    
    boolean isWet() {
      const int percentage = getWettnessPercent();
      return percentage >= wetTresholdPercentage;
    }
    
    errors::Error init() {
      if (pin == -1) {
        return errors::Error("Kein Pin für CapacativeSoilMoistureSensor konfiguriert");
      }
      pinMode(pin, INPUT); // analog 0 to 1023
      return errors::Ok();
    }
  };

}


namespace garten {
  class PflanzenWasserVersorgung {
    public:
      String name;
      devices::Waterpump waterpump;
      devices::CapacativeSoilMoistureSensor moistureSensor;
      errors::Error abortError = errors::Ok();

      PflanzenWasserVersorgung(String name);
      void checkCycle();
      errors::Error init();
      
    private:
      timer::Timer t;
  };

  
      
  PflanzenWasserVersorgung::PflanzenWasserVersorgung(String name) {
     name = name;
  }
  
  void PflanzenWasserVersorgung::checkCycle() {
    if (!abortError.isOk()) {
      waterpump.turnOff();
      return;
    }

    
    if (moistureSensor.isWet()) {
      waterpump.turnOff();
      t = timer::TimerInaktiv;
    } else {
      if (!t.isActive()) {
        t = timer::Timer(60);
      }
      if (t.isActive() && t.isExpired()) {
        waterpump.turnOff();
        abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
        return;
      }
      
      waterpump.turnOn();
    }
  }
      
  errors::Error PflanzenWasserVersorgung::init() {
    const errors::Error err1 = waterpump.init();
    if (!err1.isOk()) {
      return errors::Error("Fehler beim initialisieren der PflanzenWasserVersorgung '" + name + "' - Pumpe", err1);
    }
    const errors::Error err2 = moistureSensor.init();
    if (!err2.isOk()) {
      return errors::Error("Fehler beim initialisieren der PflanzenWasserVersorgung '" + name + "' - Sensor", err2);
    }
    return errors::Ok();
  }
}

garten::PflanzenWasserVersorgung thymian("Thymian");
garten::PflanzenWasserVersorgung basilikum("Basilikum");
garten::PflanzenWasserVersorgung schnittlauch("Schnittlauch");

void setup() {
  logger::init();
  
  // put your setup code here, to run once:
  thymian.moistureSensor.pin = nodemcu::D3;
  thymian.waterpump.pin = nodemcu::D4;
  errors::logIfError(thymian.init());
  
  basilikum.moistureSensor.pin = nodemcu::D5;
  basilikum.waterpump.pin = nodemcu::D6;
  errors::logIfError(basilikum.init());

  schnittlauch.moistureSensor.pin = nodemcu::D7;
  schnittlauch.waterpump.pin = nodemcu::D8;
  errors::logIfError(schnittlauch.init());
}

void loop() {
  // put your main code here, to run repeatedly: 
  delay(250);
  thymian.checkCycle();
  basilikum.checkCycle();
  schnittlauch.checkCycle();
}
