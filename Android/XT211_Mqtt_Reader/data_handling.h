#pragma once
#include "config.h"

void addToQueue(const MeterData& data);
void pruneOldData();
void publishQueue();
