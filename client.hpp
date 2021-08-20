/**
 * This file is part of the sensino library.
 *
 * implement the Client class which handles server-client communication.
 *
 * TODO: It would be nice to abstract it over the network protocol.
 *
 */
#pragma once

#include <Arduino.h>

// change next line to use with another board/shield
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <CircularBuffer.h>
#include <PolledTimeout.h>
#include <Vector.h>

#include <ArduinoJson.h>

#include "HTTPTimeClient.hpp"

#include "common.h"

namespace sensino {

// Use this to select if a NTP client or a custom HTTP client is used
// to sync the clock.
// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP);
HTTPTimeClient timeClient;

/**
 * Server client communication.
 *
 * With a period specified in measurePeriodMs the following callbacks:
 * - beforeMeasure
 * - onMeasure
 * - afterMeasure
 *
 * The resulting record is stored in the buffer and contains:
 * - uptime: current uptime given the arduino device
 * - timestamp: current timestamp, synced with an NTP server
 * - userRecord: the result of onMeasure callback
 *
 * These records are sent to the server as JSON:
 * - uptime: see record.uptime
 * - timestamp: see record.timestamp
 * - ntpEpoch: epoch synced by the NTP server, can be used to monitor the
 * status.
 * - bootID: a random number generated on device startup, can be used to
 * indicate the session.
 * - userRecord: the result of onMeasure callback.
 *
 *
 * Another method is available to send device info to the server:
 * - WiFi.macAddress: mac address of the arduino board.
 * - userDeviceInfo: result of calling fillDeviceInfo callback
 *
 *
 * Additionally, the following information is sent using the headers:
 * - SNO-API-KEY: can be used to filter calls to the server (defined on client
 * instantiation).
 * - SNO-SERIAL-NUMBER: can be used to identify the device (defined on client
 * instantiation).
 * - SNO-ACQ-PERIOD: acquisition period.
 * - SNO-METHOD: used to call different methods in the server
 *      0: sendRecord
 *      1: sendDeviceInfo
 * - SNO-USER-*: items in userConfig.
 *
 * The server must response a json with optionally the following items:
 * - acqPeriod: an unsigned long that indicates the desired acquisition period
 * (ms).
 * - devInfoCheck: a boolean used to ask the client to send device info.
 * - userServerPayload: the result is sent to onUserServerPayload and can be
 * used to modify user settings.
 *
 *
 * It is generic over:
 * - UR userRecord: measure information is sent to the client.
 * - UC userConfig: configuration information.
 * - BS bufferSize: how many elements are stored
 *                  in the buffer before sending.
 *
 */
template <typename UR, typename UC, size_t BS> class Client {

  typedef std::function<std::pair<UR, bool>()> THandlerFunction_Measure;
  typedef std::function<bool(const JsonObject &doc)> THandlerFunction_Read;
  typedef std::function<bool(JsonObject &doc)> THandlerFunction_Write;
  typedef std::function<void()> THandlerFunction_BeforeAfter;

private:
  // Random number generated when initialized.
  // Can be used to identify the session.
  long _bootID;

  MEASURE_STATE _measure_state = MEASURE_STATE::IDLE;
  SEND_STATE _send_state = SEND_STATE::IDLE;

  Record<UR> _lastRecord;

  // URL of the server.
  const char *_endpoint;

  // Serial number of the device.
  unsigned int _serialNumber;

  // API KEY sent to the server.
  const char *_apiKey;

  // Seconds needed to warm up.
  unsigned int _requiredWarmUp = 0;

  // true if the warmup has finished.
  bool _isReady = false;

  // Buffer where measurements are stored until sent to the server.
  CircularBuffer<Record<UR>, BS> _buffer;

  // Ticker
  esp8266::polledTimeout::periodicMs _acqTicker;

  // Handlers to
  THandlerFunction_BeforeAfter _beforeMeasure = nullptr;
  THandlerFunction_Measure _onMeasure = nullptr;
  THandlerFunction_BeforeAfter _afterMeasure = nullptr;
  THandlerFunction_Read _onUserServerPayload = nullptr;
  THandlerFunction_Write _fillDeviceInfo = nullptr;

public:
  UC userConfig;

  Client(const char *endpoint, unsigned int serialNumber, const char *apiKey,
         unsigned long measurePeriodMs)
      : _acqTicker(0), _endpoint(endpoint), _apiKey(apiKey),
        _serialNumber(serialNumber) {

    this->userConfig = UC();

    this->_acqTicker.reset(measurePeriodMs);
  }

  void beforeMeasureTick(THandlerFunction_BeforeAfter fn) {
    this->_beforeMeasure = fn;
  }

  void afterMeasureTick(THandlerFunction_BeforeAfter fn) {
    this->_afterMeasure = fn;
  }

  void onMeasureTick(THandlerFunction_Measure fn) { this->_onMeasure = fn; }

  void onUserServerPayload(THandlerFunction_Read fn) {
    this->_onUserServerPayload = fn;
  }

  void fillDeviceInfo(THandlerFunction_Write fn) { this->_fillDeviceInfo = fn; }

  // Call this method in your setup
  void setup(char *ssid, char *passphrase) {
    randomSeed(analogRead(0));
    this->_bootID = random(2147483647);

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, passphrase);
    delay(600);
    yield();
    WiFi.setAutoReconnect(true);
    timeClient.begin();
  }

  // Call this method in your loop
  void loop() {

    this->_measure_state = MEASURE_STATE::IDLE;

    auto meas = this->measure();
    if (meas.second) {
      this->_lastRecord = meas.first;
      this->_measure_state = MEASURE_STATE::SUCCESS;
      if (this->_acqTicker) {
        if (this->_buffer.isFull()) {
          this->_measure_state = MEASURE_STATE::BUFFER_FULL;
        } else {
          this->_measure_state = MEASURE_STATE::STORE;
          this->_buffer.push(this->_lastRecord);
        }
      }
    } else {
      this->_measure_state = MEASURE_STATE::ERROR;
    }

    if (this->_buffer.isEmpty()) {
      this->_send_state = SEND_STATE::IDLE;
    } else {
      if (this->sendPending(1)) {
        this->_send_state = SEND_STATE::SUCCESS;
      } else {
        this->_send_state = SEND_STATE::ERROR;
      }
    }

    timeClient.update();
  }

  // Send n records in the buffer to the server.
  bool sendPending(uint n) {
    bool success = true;
    while (!this->_buffer.isEmpty() && n > 0) {
      if (this->sendRecord(this->_buffer.first())) {
        this->_buffer.shift();
      } else {
        success = false;
      }
      n--;
    }
    return success;
  }

  // Send a record to the server
  // return success state.
  bool sendRecord(Record<UR> record) {

    // Serialize the record to JSON.
    char body[300];
    {
      DynamicJsonDocument doc(300);

      // Time since the device was booted
      doc["uptime"] = record.uptime;
      // Time in which the client has synced with the NTP Server.
      // Useful for debugging
      doc["ntpEpoch"] = timeClient.getCurrentEpoch();
      // Current time in UTC.
      doc["timestamp"] = record.timestamp;
      // Unique identifier for a boot session.
      doc["bootID"] = this->_bootID;

      auto docur = doc.createNestedObject("userRecord");
      record.userRecord.fill(docur);
      serializeJson(doc, body);
    }

    return this->_send(body, 0);
  }

  // Send device information to the server
  // return success state.
  bool sendDeviceInfo() {

    // Serialize the record to JSON.
    char body[300];
    {
      DynamicJsonDocument doc(300);

      // Arduino mad address.
      doc["WiFi.macAddress"] = WiFi.macAddress();

      if (this->_fillDeviceInfo != nullptr) {
        auto docur = doc.createNestedObject("userDeviceInfo");
        this->_fillDeviceInfo(docur);
      }

      serializeJson(doc, body);
    }

    return this->_send(body, 1);
  }

  bool _send(const char *jsonString, const int method) {

    HTTPClient http;
    http.begin(this->_endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("SNO-API-KEY", this->_apiKey);
    http.addHeader("SNO-SERIAL-NUMBER", String(this->_serialNumber));
    http.addHeader("SNO-ACQ-PERIOD", String(this->_acqTicker.getTimeout()));
    http.addHeader("SNO-METHOD", String(method));

    // Serialize the UserConfig to HTTP headers.
    {
      DynamicJsonDocument docConfig(300);
      this->userConfig.fill(docConfig);

      JsonObject root = docConfig.as<JsonObject>();
      for (JsonPair item : root) {
        http.addHeader("SNO-USER-" + String(item.key().c_str()),
                       item.value().as<String>());
      }
    }

    int httpCode = http.POST(jsonString);
    if (httpCode != 200) {
      http.end();
      return false;
    }

    String payload = http.getString();
    http.end();

    // Deserialize the Payload
    {
      DynamicJsonDocument docPayload(300);
      deserializeJson(docPayload, payload);
      if (docPayload.containsKey("acqPeriod")) {
        this->_acqTicker.reset(docPayload["acqPeriod"].as<unsigned long>());
      }
      if (docPayload.containsKey("devInfoCheck")) {
        this->sendDeviceInfo();
      }
      if (docPayload.containsKey("userServerPayload")) {
        this->_onUserServerPayload(docPayload["userServerPayload"]);
      }
    }
    return true;
  }

  //
  std::pair<Record<UR>, bool> measure() const {
    Record<UR> rec;
    if (this->_beforeMeasure != nullptr) {
      this->_beforeMeasure();
    }
    rec.uptime = millis();
    rec.timestamp = timeClient.getEpochTime();
    auto meas = _onMeasure();
    if (!meas.second) {
      return std::make_pair(rec, false);
    }
    rec.userRecord = meas.first;
    if (this->_afterMeasure != nullptr) {
      this->_afterMeasure();
    }
    return std::make_pair(rec, true);
  }

  bool isBufferFull() const { return this->_buffer.isFull(); }

  Record<UR> getLastRecord() const { return this->_lastRecord; }

  MEASURE_STATE getMeasureState() const { return this->_measure_state; }

  SEND_STATE getSendState() const { return this->_send_state; }

  void setReady() { this->_isReady = true; }

  // Seconds required to Warm Up
  void setRequiredWarmUp(unsigned int value) { this->_requiredWarmUp = value; }

  unsigned int getSerialNumber() const { return this->_serialNumber; }

  long getPendingWarmUp() const {
    if (this->_isReady) {
      return 0;
    }
    return (60000 - (long)millis()) / 1000;
  }
};
} // namespace sensino
