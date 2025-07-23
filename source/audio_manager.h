#pragma once

bool audioManagerInit();
bool audioManagerPlay(const char *path);
void audioManagerStop();
void audioManagerExit();