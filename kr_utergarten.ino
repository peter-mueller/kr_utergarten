
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

  static const uint8_t A0  = 17;

}


namespace devices {

  struct Waterpump {
    int pin = -1;
    
    void turnOn() {
      digitalWrite(pin, LOW);
    }
    
    void turnOff() {
      digitalWrite(pin, HIGH);
    }
    
    errors::Error init() {
      if (pin == -1) {
        return errors::Error("Kein Pin für Waterpump konfiguriert");
      }
      pinMode(pin, OUTPUT);
      digitalWrite(pin, HIGH);
      return errors::ok();
    }
  };
  
  struct CapacativeSoilMoistureSensor {
    int pin = -1;
    
    int analogValueInWater = 400; // 400
    int analogValueInAir = 800; // 800
    int wetTresholdPercentage = 60;

    CapacativeSoilMoistureSensor() {
            logger::info("ctor CapacativeSoilMoistureSensor");
    };

    
    
    virtual int getWettnessPercent() {
      const int value = analogRead(pin);
      const int percentage = map(value, analogValueInAir, analogValueInWater, 0, 100);
      const int rangedPercentage = constrain(percentage, 0, 100);
      logger::info("Read Wetness: " + String(rangedPercentage) + "% [t:" + String(wetTresholdPercentage) + "%] " + "Analog value: " + String(value) + "[" + analogValueInAir + "," +  analogValueInWater +"]");
      return rangedPercentage;
    } 
    
    boolean isWetOrInAir() {
      const int percentage = getWettnessPercent();
      return percentage >= wetTresholdPercentage || percentage <= 5;
    }
    
    virtual errors::Error init() {
      if (pin == -1) {
        return errors::Error("Kein Pin für CapacativeSoilMoistureSensor konfiguriert");
      }
      return errors::ok();
    }
  };

  struct MultiplexedCapacativeSoilMoistureSensor : CapacativeSoilMoistureSensor {
    int activatePin = -1;

    MultiplexedCapacativeSoilMoistureSensor() {
                  logger::info("ctor MultiplexedCapacativeSoilMoistureSensor");
    };

    int getWettnessPercent() override {
      digitalWrite(activatePin, HIGH);
      delay(100);
      const int percentage = CapacativeSoilMoistureSensor::getWettnessPercent();
      digitalWrite(activatePin, LOW);
      delay(100);
      return percentage;
    }

    errors::Error init() override {
      logger::info("init called");
      if (activatePin == -1) {
        return errors::Error("Kein Pin für das aktivieren des  MultiplexedCapacativeSoilMoistureSensor konfiguriert");
      }
      pinMode(activatePin, OUTPUT);
      digitalWrite(activatePin, LOW);
      return CapacativeSoilMoistureSensor::init();
    }
  };

}


namespace garten {
  class PflanzenWasserVersorgung {
    public:
      String name;
      devices::Waterpump waterpump;
      devices::MultiplexedCapacativeSoilMoistureSensor moistureSensor;
      errors::Error abortError = errors::ok();

      PflanzenWasserVersorgung(String name);
      boolean isActive();
      errors::Error checkCycle();
      errors::Error init();
      
    private:
      timer::Timer t;
  };

  
      
  PflanzenWasserVersorgung::PflanzenWasserVersorgung(String name) {
     name = name;
  }

  boolean PflanzenWasserVersorgung::isActive() {
     return t.isActive();
  }
  
  errors::Error PflanzenWasserVersorgung::checkCycle() {
    if (!abortError.isOk()) {
      waterpump.turnOff();
      return abortError;
    }

    
    if (moistureSensor.isWetOrInAir()) {
      waterpump.turnOff();
      t = timer::TimerInaktiv;
    } else {
      if (!t.isActive()) {
        t = timer::Timer(10);
      }
      if (t.isActive() && t.isExpired()) {
        waterpump.turnOff();
        abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
        return abortError;
      }
      
      waterpump.turnOn();
    }
    return errors::ok();
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
    return errors::ok();
  }
}

garten::PflanzenWasserVersorgung thymian("Thymian");
garten::PflanzenWasserVersorgung basilikum("Basilikum");
garten::PflanzenWasserVersorgung schnittlauch("Schnittlauch");
garten::PflanzenWasserVersorgung koriander("Koriander");

storage::PersitentStorage deepSleepStorage;
timer::Timer deepSleepAfterTimer(60);
struct AbortetState {
    bool thymianAborted = false;
    bool basilikumAborted = false;
    bool schnittlauchAborted = false;
    bool korianderAborted = false;
    
    uint32_t toUint32Data() {
      uint32_t mask = 0b0000;
      if (thymianAborted)      {mask |= 0b0001;}
      if (basilikumAborted)    {mask |= 0b0010;}
      if (schnittlauchAborted) {mask |= 0b0100;}
      if (korianderAborted)    {mask |= 0b1000;}
      return mask;
    }

    uint32_t parseUint32Data(uint32_t data) {
      thymianAborted =      (data & 0b0001) > 0;
      basilikumAborted =    (data & 0b0010) > 0;
      schnittlauchAborted = (data & 0b0100) > 0;
      korianderAborted =    (data & 0b1000) > 0;
    }
    
};
AbortetState abortetState;


void setup() {
  logger::init();
  deepSleepStorage.restore();
  abortetState.parseUint32Data(deepSleepStorage.getData());

  while (!Serial) {}

  
  // put your setup code here, to run once:
  thymian.moistureSensor.pin = nodemcu::A0;
  thymian.moistureSensor.activatePin = D5;
  thymian.moistureSensor.analogValueInWater = 382;
  thymian.moistureSensor.analogValueInAir = 797;
  thymian.waterpump.pin = nodemcu::D4;
  if (abortetState.thymianAborted) {
    thymian.abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
  }
  errors::logIfError(thymian.init());
  
  basilikum.moistureSensor.pin = nodemcu::A0;
  basilikum.moistureSensor.activatePin = D6;
  basilikum.moistureSensor.analogValueInWater = 379;
  basilikum.moistureSensor.analogValueInAir = 739;
  basilikum.waterpump.pin = nodemcu::D3;
  if (abortetState.basilikumAborted) {
    basilikum.abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
  }
  errors::logIfError(basilikum.init());

  schnittlauch.moistureSensor.pin = nodemcu::A0;
  schnittlauch.moistureSensor.activatePin = D7;
  schnittlauch.moistureSensor.analogValueInWater = 379;
  schnittlauch.moistureSensor.analogValueInAir = 745;
  schnittlauch.waterpump.pin = nodemcu::D2;
  if (abortetState.schnittlauchAborted) {
    schnittlauch.abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
  }
  errors::logIfError(schnittlauch.init());

  koriander.moistureSensor.pin = nodemcu::A0;
  koriander.moistureSensor.activatePin = D8;
  koriander.moistureSensor.analogValueInWater = 379;
  koriander.moistureSensor.analogValueInAir = 710;
  koriander.waterpump.pin = nodemcu::D1;
  if (abortetState.korianderAborted) {
    koriander.abortError = errors::Error("PflanzenWasserVersorgung wurde zur Sicherheit deaktiviert, da die Pumpe länger als eine Minute am Stück lief.");
  }
  errors::logIfError(koriander.init());
}

void loop() {  
  int delayDuration = 2000;
  bool anyActive = thymian.isActive() || basilikum.isActive() || schnittlauch.isActive() || koriander.isActive();
  if (!anyActive && deepSleepAfterTimer.isExpired()) {
    deepSleepStorage.store(abortetState.toUint32Data());
    ESP.deepSleep(10/*min*/ * 60/*sec*/ * 1000000);
  }
  if (anyActive) {
      // check more frequently
      delayDuration = 250;
  }
  // put your main code here, to run repeatedly: 
  delay(delayDuration);
  
  errors::logIfError(thymian.checkCycle());
  abortetState.thymianAborted = !thymian.abortError.isOk();

  errors::logIfError(basilikum.checkCycle());
  abortetState.basilikumAborted = !basilikum.abortError.isOk();

  errors::logIfError(schnittlauch.checkCycle());
  abortetState.schnittlauchAborted = !schnittlauch.abortError.isOk();

  errors::logIfError(koriander.checkCycle());
  abortetState.korianderAborted = !koriander.abortError.isOk();
  
}
