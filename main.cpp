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

#include "HCStream.h"

// Default size of 2^25
unsigned int ARRAY_SIZE = 33554432;
unsigned int num_times = 100;
unsigned int deviceIndex = 1;

template <typename T>
void run();

void parseArguments(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  std::cout
    << "GPU-STREAM "
    << "Version: " << VERSION_STRING
    << " Implementation: " << IMPLEMENTATION_STRING << std::endl;

  parseArguments(argc, argv);
  run<double>();

}

template <typename T>
void run()
{
  std::cout << "Running kernels " << num_times << " times" << std::endl;

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

  HCStream<T> *stream;
  stream = new HCStream<T>(ARRAY_SIZE, deviceIndex);

  //std::cout << "Entering init arrays" <<std::endl;
  stream->init_arrays(startA, startC);

  std::vector<std::vector<double>> timings(1);
  std::chrono::high_resolution_clock::time_point t1, t2;
  //std::cout << "Entering Main Loop" <<std::endl;
  for (unsigned int k = 0; k < num_times; k++)
  {
    //std::cout << "Entering Copy" <<std::endl;
    t1 = std::chrono::high_resolution_clock::now();
    stream->copy();
    t2 = std::chrono::high_resolution_clock::now();
    timings[0].push_back(std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count());
  }

  std::cout
    << std::left << std::setw(16) << "Function"
    << std::left << std::setw(16) << "MBytes/sec"
    << std::left << std::setw(16) << "Min (sec)"
    << std::left << std::setw(16) << "Max"
    << std::left << std::setw(16) << "Average" << std::endl;

  std::cout << std::fixed;

  std::string label = "Copy";
  size_t size = 2 * sizeof(T) * ARRAY_SIZE;

  // Get min/max; ignore the first result
  auto minmax = std::minmax_element(timings[0].begin()+1, timings[0].end());

  // Calculate average; ignore the first result
  double average = std::accumulate(timings[0].begin()+1, timings[0].end(), 0.0) / (double)(num_times - 1);

  // Display results
  std::cout
    << std::left << std::setw(16) << label
    << std::left << std::setw(16) << std::setprecision(3) << 1.0E-6 * size / (*minmax.first)
    << std::left << std::setw(16) << std::setprecision(5) << *minmax.first
    << std::left << std::setw(16) << std::setprecision(5) << *minmax.second
    << std::left << std::setw(16) << std::setprecision(5) << average
    << std::endl;

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
    if (!std::string("--arraysize").compare(argv[i]) ||
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
    else
    {
      std::cerr << "Unrecognized argument '" << argv[i] << "' (try '--help')"
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}
