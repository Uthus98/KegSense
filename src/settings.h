#pragma once

void settingsBegin();

void loadKegSettings(int index);
void saveKegSettings(int index);

bool hasScaleTareOffset(int index);
long loadScaleTareOffset(int index);
void saveScaleTareOffset(int index, long offset);
