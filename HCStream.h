
// Copyright (c) 2015-16 Tom Deakin, Simon McIntosh-Smith,
// University of Bristol HPC
//
// For full license terms please see the LICENSE file distributed with this
// source code

#pragma once

#include <iostream>
#include <stdexcept>
#include <sstream>

//#include "Stream.h"
#include "hc.hpp"

#define IMPLEMENTATION_STRING "HC"

#define startA (0.1)
#define startC (0.2)

template <class T>
class HCStream //: public Stream<T>
{
protected:
  // Size of arrays
  unsigned int array_size;
  // Device side pointers to arrays
  T* d_a;
  T* d_c;


public:

  HCStream(const unsigned int, const int);
  ~HCStream();

  virtual void copy() override;
  virtual void init_arrays(T initA, T initC) override;

};

void listDevices(void);
std::string getDeviceName(const int);
std::string getDeviceDriver(const int);
