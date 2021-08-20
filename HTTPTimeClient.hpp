/**
 * This file is part of the sensino library.
 *
 */
#pragma once

// change next line to use with another board/shield
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

namespace sensino {

/**
 * This library is an HTTP based replacement for NTP.
 *
 * Use it ONLY if NTP is unavailable.
 *
 */
class HTTPTimeClient {
private:
  const char *_endpoint; // Default time server
  long _timeOffset = 0;

  unsigned long _updateInterval = 3600000; // In ms

  unsigned long _currentEpoc = 0; // In s
  unsigned long _lastUpdate = 0;  // In ms

  unsigned int _repeats = 9;

public:
  HTTPTimeClient() {}

  HTTPTimeClient(const char *endpoint) { this->_endpoint = endpoint; }

  HTTPTimeClient(const char *endpoint, long timeOffset) {
    this->_endpoint = endpoint;
    this->_timeOffset = timeOffset;
  }

  HTTPTimeClient(const char *endpoint, long timeOffset,
                 unsigned long updateInterval) {
    this->_endpoint = endpoint;
    this->_timeOffset = timeOffset;
    this->_updateInterval = updateInterval;
  }

  void begin() {}

  void begin(const char *endpoint) { this->_endpoint = endpoint; }

  bool forceUpdate() {

#ifdef DEBUG_HTTPTimeClient
    Serial.println("Update from NTP Server");
#endif

    unsigned long minRoundTrip = 4294967295;

    unsigned long tmpCurrentEpoc = 0; // In s
    unsigned long tmpLastUpdate = 0;  // In ms

    for (size_t n = 0; n < this->_repeats; n++) {
      delay(random(40, 230));
      unsigned long t1;
      unsigned long t2;
      unsigned long s1;
      {
        HTTPClient http;
        http.begin(this->_endpoint);

        t1 = millis();
        int httpCode = http.GET();
        if (httpCode != 200) {
          http.end();
          return false;
        }
        String payload = http.getString();
        s1 = atol(payload.c_str());
        t2 = millis();
      }
      if ((t2 - t1) < minRoundTrip) {
        tmpCurrentEpoc = s1;
        tmpLastUpdate = (t1 + t2) / 2;
      }
    }

    this->_lastUpdate = tmpLastUpdate;
    this->_currentEpoc = tmpCurrentEpoc;

    return true;
  }

  bool update() {
    if ((millis() - this->_lastUpdate >=
         this->_updateInterval)      // Update after _updateInterval
        || this->_lastUpdate == 0) { // Update if there was no update yet.
      return this->forceUpdate();
    }
    return true;
  }

  unsigned long getEpochTime() const {
    return this->_timeOffset +  // User offset
           this->_currentEpoc + // Epoc returned by the NTP server
           ((millis() - this->_lastUpdate) / 1000); // Time since last update
  }

  int getDay() const {
    return (((this->getEpochTime() / 86400L) + 4) % 7); // 0 is Sunday
  }

  int getHours() const { return ((this->getEpochTime() % 86400L) / 3600); }

  int getMinutes() const { return ((this->getEpochTime() % 3600) / 60); }

  int getSeconds() const { return (this->getEpochTime() % 60); }

  String getFormattedTime() const {
    unsigned long rawTime = this->getEpochTime();
    unsigned long hours = (rawTime % 86400L) / 3600;
    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

    unsigned long minutes = (rawTime % 3600) / 60;
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    unsigned long seconds = rawTime % 60;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

    return hoursStr + ":" + minuteStr + ":" + secondStr;
  }

  void end() {}

  void setTimeOffset(int timeOffset) { this->_timeOffset = timeOffset; }

  void setUpdateInterval(unsigned long updateInterval) {
    this->_updateInterval = updateInterval;
  }

  void setendpoint(const char *endpoint) { this->_endpoint = endpoint; }

  unsigned long millisToEpoch(unsigned long value) const {
    return this->_timeOffset +  // User offset
           this->_currentEpoc + // Epoc returned by the NTP server
           ((value - this->_lastUpdate) / 1000); // Time since last update
  }

  unsigned long getCurrentEpoch() const { return this->_currentEpoc; }
};
} // namespace sensino
