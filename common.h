/**
 * This file is part of the sensino library.
 *
 */
#pragma once

namespace sensino {

enum class MEASURE_STATE {
  IDLE,        // No measurement was done.
  SUCCESS,     // Measurement was successful. (TODO: Remoe this)
  ERROR,       // Error while measuring.
  STORE,       // Measurement was successful and stored in the buffer.
  BUFFER_FULL, // Measurement was successful but the buffer was full.
};
enum class SEND_STATE {
  IDLE,    // No sent was done.
  SUCCESS, // Record was sent.
  ERROR    // Error while sending.
};

template <typename UC> struct Config {
  unsigned long acqPeriod;
  UC userConfig;
};

template <typename UR> struct Record {
  unsigned long uptime;
  unsigned long timestamp;
  UR userRecord;
};

} // namespace sensino
