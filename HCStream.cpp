// Copyright (c) 2015-16 Peter Steinbach, MPI CBG Scientific Computing Facility
//
// For full license terms please see the LICENSE file distributed with this
// source code

#include "HCStream.h"

#include <codecvt>
#include <vector>
#include <locale>
#include <numeric>
#include <hc_am.hpp>
#include <hc.hpp>

#define TBSIZE 1024

std::string getDeviceName(const hc::accelerator& _acc)
{
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::string value = converter.to_bytes(_acc.get_description());
  return value;
}

void listDevices(void)
{
  // Get number of devices
  std::vector<hc::accelerator> accs = hc::accelerator::get_all();

  // Print device names
  if (accs.empty())
  {
    std::cerr << "No devices found." << std::endl;
  }
  else
  {
    std::cout << std::endl;
    std::cout << "Devices:" << std::endl;
    for (int i = 0; i < accs.size(); i++)
    {
      std::cout << i << ": " << getDeviceName(accs[i]) << std::endl;
    }
    std::cout << std::endl;
  }
}


template <class T>
HCStream<T>::HCStream(const unsigned int ARRAY_SIZE, const int device_index):
  array_size(ARRAY_SIZE)
{

  // The array size must be divisible by TBSIZE for kernel launches
  if (ARRAY_SIZE % TBSIZE != 0)
  {
    std::stringstream ss;
    ss << "Array size must be a multiple of " << TBSIZE;
    throw std::runtime_error(ss.str());
  }

  // Set device
  std::vector<hc::accelerator> accs = hc::accelerator::get_all();
  auto current = accs.at(device_index);

  hc::accelerator::set_default(current.get_device_path());

  std::cout << "Using HC device " << getDeviceName(current) << std::endl;
  auto acc = hc::accelerator();
  d_a = (T*) hc::am_alloc(array_size * sizeof(T), acc, 0);
  d_c = (T*) hc::am_alloc(array_size * sizeof(T), acc, 0);

}


template <class T>
HCStream<T>::~HCStream()
{
  hc::am_free(d_a);
  hc::am_free(d_c);
}

template <class T>
void HCStream<T>::init_arrays(T _a, T _c)
{
  T* a = (T*) malloc(array_size * sizeof(T));
  T* c = (T*) malloc(array_size * sizeof(T));
  for (int i = 0; i < array_size; i++) {
    a[i] = _a;
    c[i] = _c;
  }
  auto acc = hc::accelerator();
  hc::accelerator_view av = acc.get_default_view();
  av.copy(a, d_a, array_size*sizeof(T));
  av.copy(c, d_c, array_size*sizeof(T));
  free(a);
  free(c);
}

template <class T>
void HCStream<T>::copy()
{
  T* d_cc = d_c;
  T* d_aa = d_a;
  hc::extent<1> e(array_size);
  try{
    hc::parallel_for_each(e, [=](hc::index<1> index) [[hc]] {
        d_cc[index[0]] = d_aa[index[0]];
    });

  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::copy " << e.what() << std::endl;
    throw;
  }
}

template class HCStream<float>;
template class HCStream<double>;