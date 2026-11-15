#pragma once

/** Reloads the active species sprite when its species, form, or SD state changes. */
void ensureMon();

/** Runs the periodic pet, hardware, and display updates for one firmware loop. */
void runAppLoop();
