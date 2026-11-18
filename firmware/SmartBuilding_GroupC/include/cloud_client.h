#pragma once
#include <Arduino.h>
enum class SendResult { ACK, RETRY, PERMANENT, TOKEN_MISSING };
SendResult cloudSend(const String& record);
