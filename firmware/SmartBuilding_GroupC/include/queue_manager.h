#pragma once
#include <Arduino.h>
bool queueInit();
void queueRecover();
bool queueSave(const String& id,const String& payload);
bool queuePeek(String& path,String& payload);
bool queueRemove(const String& path);
size_t queueCount();
bool queueHealthy();

bool queueReject(const String& path);
