/**
 * This file is part of the sensino library.
 *
 */
#pragma once

#include <ESP_EEPROM.h>

namespace sensino {

/**
 * Persistent storage for ESP using the flash as EEPROM.
 *
 * The content attribute can be use to access the cached values
 * Use read to update the cache from the persistent storage,
 * and write to do the opposite.
 *
 * It is generic over:
 * - S content: a struct indicates which persistent information is used.
 */
template <typename S> class Memory {

private:
  int _address;

public:
  S content;

  Memory(int address) : _address(address) {
    EEPROM.begin(sizeof(S));
    this->read();
  };

  void read() { EEPROM.get(_address, content); }

  bool write() {
    EEPROM.put(_address, content);
    return EEPROM.commit();
  }
};
} // namespace sensino
