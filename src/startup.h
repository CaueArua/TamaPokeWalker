#pragma once

/** Initializes serial, the I2C bus, and the display hardware. */
void initializeHardware();

/** Initializes the touch, clock, battery, and motion sensors. */
void initializeSensors();

/** Initializes game state, storage, power services, and the user interface. */
void initializeProgram();
