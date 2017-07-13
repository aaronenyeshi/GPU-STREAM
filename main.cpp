
// Copyright (c) 2015-16 Tom Deakin, Simon McIntosh-Smith,
// University of Bristol HPC
//
// For full license terms please see the LICENSE file distributed with this
// source code

#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cstring>

#define VERSION_STRING "3.1"

//#include "Stream.h"
#include "HCStream.h"

// Default size of 2^25
unsigned int ARRAY_SIZE = 33554432;
unsigned int num_times = 100;
unsigned int deviceIndex = 0;
bool use_float = false;

template <typename T>
void run();

void parseArguments(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  std::cout
    << "GPU-STREAM" << std::endl
    << "Version: " << VERSION_STRING << std::endl
    << "Implementation: " << IMPLEMENTATION_STRING << std::endl;

  parseArguments(argc, argv);

  if (use_float)
    run<float>();
  else
    run<double>();

}

template <typename T>
void run()
{
  std::cout << "Running kernels " << num_times << " times" << std::endl;

  if (sizeof(T) == sizeof(float))
    std::cout << "Precision: float" << std::endl;
  else
    std::cout << "Precision: double" << std::endl;

  // Create host vectors
  std::vector<T> a(ARRAY_SIZE);
  std::vector<T> c(ARRAY_SIZE);
  std::streamsize ss = std::cout.precision();
  std::cout << std::setprecision(1) << std::fixed
    << "Array size: " << ARRAY_SIZE*sizeof(T)*1.0E-6 << " MB"
    << " (=" << ARRAY_SIZE*sizeof(T)*1.0E-9 << " GB)" << std::endl;
  std::cout << "Total size: " << 3.0*ARRAY_SIZE*sizeof(T)*1.0E-6 << " MB"
    << " (=" << 3.0*ARRAY_SIZE*sizeof(T)*1.0E-9 << " GB)" << std::endl;
  std::cout.precision(ss);

  // Result of the Dot kernel
  T sum;

  HCStream<T> *stream;

 // Use the HC implementation
  stream = new HCStream<T>(ARRAY_SIZE, deviceIndex);

  std::cout << "Entering init arrays" <<std::endl;
  stream->init_arrays(startA, startC);

  // List of times
  std::vector<std::vector<double>> timings(5);

  // Declare timers
  std::chrono::high_resolution_clock::time_point t1, t2;
  std::cout << "Entering Main Loop" <<std::endl;
  // Main loop
  for (unsigned int k = 0; k < num_times; k++)
  {
    //std::cout << "Entering Copy" <<std::endl;
    // Execute Copy
    t1 = std::chrono::high_resolution_clock::now();
    stream->copy();
    t2 = std::chrono::high_resolution_clock::now();
    timings[0].push_back(std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count());
  }

  // Display timing results
  std::cout
    << std::left << std::setw(12) << "Function"
    << std::left << std::setw(12) << "MBytes/sec"
    << std::left << std::setw(12) << "Min (sec)"
    << std::left << std::setw(12) << "Max"
    << std::left << std::setw(12) << "Average" << std::endl;

  std::cout << std::fixed;

  std::string labels[1] = {"Copy"};
  size_t sizes[1] = {
    2 * sizeof(T) * ARRAY_SIZE
  };

  for (int i = 0; i < 1; i++)
  {
    // Get min/max; ignore the first result
    auto minmax = std::minmax_element(timings[i].begin()+1, timings[i].end());

    // Calculate average; ignore the first result
    double average = std::accumulate(timings[i].begin()+1, timings[i].end(), 0.0) / (double)(num_times - 1);

    // Display results
    std::cout
      << std::left << std::setw(16) << labels[i]
      << std::left << std::setw(16) << std::setprecision(3) << 1.0E-6 * sizes[i] / (*minmax.first)
      << std::left << std::setw(16) << std::setprecision(5) << *minmax.first
      << std::left << std::setw(16) << std::setprecision(5) << *minmax.second
      << std::left << std::setw(16) << std::setprecision(5) << average
      << std::endl;

  }

  delete stream;

}

int parseUInt(const char *str, unsigned int *output)
{
  char *next;
  *output = strtoul(str, &next, 10);
  return !strlen(next);
}

void parseArguments(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    if (!std::string("--list").compare(argv[i]))
    {
      listDevices();
      exit(EXIT_SUCCESS);
    }
    else if (!std::string("--device").compare(argv[i]))
    {
      if (++i >= argc || !parseUInt(argv[i], &deviceIndex))
      {
        std::cerr << "Invalid device index." << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    else if (!std::string("--arraysize").compare(argv[i]) ||
             !std::string("-s").compare(argv[i]))
    {
      if (++i >= argc || !parseUInt(argv[i], &ARRAY_SIZE))
      {
        std::cerr << "Invalid array size." << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    else if (!std::string("--numtimes").compare(argv[i]) ||
             !std::string("-n").compare(argv[i]))
    {
      if (++i >= argc || !parseUInt(argv[i], &num_times))
      {
        std::cerr << "Invalid number of times." << std::endl;
        exit(EXIT_FAILURE);
      }
      if (num_times < 2)
      {
        std::cerr << "Number of times must be 2 or more" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    else if (!std::string("--float").compare(argv[i]))
    {
      use_float = true;
    }
    else if (!std::string("--help").compare(argv[i]) ||
             !std::string("-h").compare(argv[i]))
    {
      std::cout << std::endl;
      std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  -h  --help               Print the message" << std::endl;
      std::cout << "      --list               List available devices" << std::endl;
      std::cout << "      --device     INDEX   Select device at INDEX" << std::endl;
      std::cout << "  -s  --arraysize  SIZE    Use SIZE elements in the array" << std::endl;
      std::cout << "  -n  --numtimes   NUM     Run the test NUM times (NUM >= 2)" << std::endl;
      std::cout << "      --float              Use floats (rather than doubles)" << std::endl;
      std::cout << std::endl;
      exit(EXIT_SUCCESS);
    }
    else
    {
      std::cerr << "Unrecognized argument '" << argv[i] << "' (try '--help')"
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
