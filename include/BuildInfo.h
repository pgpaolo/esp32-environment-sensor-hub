#pragma once
#define FW_NAME "ESP32 Environment Sensor Hub"
#define FW_VERSION "0.7.1"

// Compatibility overload for ESP32 Arduino toolchain where String::toInt()
// returns long and std::max(int,long) is otherwise ambiguous in WebUi.cpp.
inline long max(int a, long b) { return ((long)a > b) ? (long)a : b; }
