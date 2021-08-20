/**
 * This file is part of the sensino library.
 *
 * Defines macros to simplify debugging.
 *
 */

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

// # MACRO info_print()
#define info_print(...)                                                        \
  do {                                                                         \
    if (DEBUG_TEST) {                                                          \
      Serial.print("INFO : (");                                                \
      Serial.print(__LINE__);                                                  \
      Serial.print(") : ");                                                    \
      Serial.println(__VA_ARGS__);                                             \
    }                                                                          \
  } while (0)

// # MACRO info_print_var()
#define info_print_var(NAMEVAR, VALUE)                                         \
  do {                                                                         \
    if (DEBUG_TEST) {                                                          \
      Serial.print("INFO : (");                                                \
      Serial.print(__LINE__);                                                  \
      Serial.print(") : ");                                                    \
      Serial.print(NAMEVAR);                                                   \
      Serial.print(" = ");                                                     \
      Serial.println(VALUE);                                                   \
    }                                                                          \
  } while (0)