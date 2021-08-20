/**
 * This file is part of the sensino library.
 *
 */
#pragma once

namespace sensino {

/**
 * Handles pressing a button to choose from a menu.
 *
 * Moving through the menu is done by pressing steadily a button
 * and releasing it when the desired menu item is found.
 *
 * It is generic over:
 * - E: enum class that enumerates the different
 *      menu items and must contain a RELEASED element.
 *
 * Callbacks must be provided to act on button pressed or release.
 * - Button pressed callback must return the next state after a
 *   "persistence" number of loops calls has been done.
 * - Button release callback must return the next state upon release.
 */
template <typename E> class Button {

  typedef std::function<E()> THandlerFunction_PressedReleased;

private:
  // Pin number in the board connected to the button.
  unsigned char _button_pin;

  // State
  int _pressed_state;

  // Number of loop iterations to show a menu item.
  unsigned int _persistence = 10;

  // Current state.
  E _state = E::RELEASED;

  // Stores Press and Release callbacks.
  THandlerFunction_PressedReleased _callbacks_pressed[10];
  THandlerFunction_PressedReleased _callbacks_released[10];

public:
  Button(unsigned char button_pin, int pressed_state, unsigned int persistence)
      : _button_pin(button_pin), _pressed_state(pressed_state) {
    if (persistence == 0) {
      this->_persistence = 10;
    } else {
      this->_persistence = persistence;
    }
  }

  // Current state
  E getState() const { return this->_state; }

  // Add state to their corresponding callbacks. These callbacks must return the
  // next state.
  void addState(E _state, THandlerFunction_PressedReleased _fcnPressed,
                THandlerFunction_PressedReleased _fcnReleased) {
    this->_callbacks_pressed[static_cast<unsigned int>(_state)] = _fcnPressed;
    this->_callbacks_released[static_cast<unsigned int>(_state)] = _fcnReleased;
  }

  // Call this in your arduino loop.
  void loop() {
    static unsigned int buttonPressingCount = this->_persistence;
    static E nextState = E::RELEASED;

    if (digitalRead(this->_button_pin) == this->_pressed_state) {
      nextState =
          this->_callbacks_pressed[static_cast<unsigned int>(this->_state)]();
      buttonPressingCount--;
      if (this->_state == E::RELEASED || buttonPressingCount == 0) {
        buttonPressingCount = this->_persistence;
        this->_state = nextState;
      }

    } else {

      this->_state =
          this->_callbacks_released[static_cast<unsigned int>(this->_state)]();
      buttonPressingCount = this->_persistence;
    }
  }
};
} // namespace sensino
