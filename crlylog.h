#pragma once
#include <string>

// Simple logging bridge used by components to log through the central Logger
// Implementation lives in main.cpp and depends on Logger::instance().
// Usage:
//   CrlyLog("Hook", "EntryPoint hooked successfully");

// Declaration - implemented in main.cpp
void CrlyLog(const char* component, const char* message);
