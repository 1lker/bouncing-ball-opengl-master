#ifndef OBJECTS_H
#define OBJECTS_H

#include "Angel.h"
#include <string>

bool loadBunnyModel(const std::string& filename);
void calculateBunnyNormals();
void initCube();
void initSphere(int subdivisions);

#endif
