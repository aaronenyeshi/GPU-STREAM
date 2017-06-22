// Copyright (c) 2015-16 Peter Steinbach, MPI CBG Scientific Computing Facility
//
// For full license terms please see the LICENSE file distributed with this
// source code

#include "HCStream.h"

#include <codecvt>
#include <vector>
#include <locale>
#include <numeric>

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
  array_size(ARRAY_SIZE),
  d_a(ARRAY_SIZE),
  d_b(ARRAY_SIZE),
  d_c(ARRAY_SIZE)
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

}


template <class T>
HCStream<T>::~HCStream()
{
}

template <class T>
void HCStream<T>::init_arrays(T _a, T _b, T _c)
{
  hc::array_view<T,1> view_a(this->d_a);
  hc::array_view<T,1> view_b(this->d_b);
  hc::array_view<T,1> view_c(this->d_c);

  hc::completion_future future_a= hc::parallel_for_each(hc::extent<1>(array_size)
                                , [=](hc::index<1> i) [[hc]] {
                                  view_a[i] = _a;
                                });

  hc::completion_future future_b= hc::parallel_for_each(hc::extent<1>(array_size)
                                                        , [=](hc::index<1> i) [[hc]] {
                                                          view_b[i] = _b;
                                                        });
  hc::completion_future future_c= hc::parallel_for_each(hc::extent<1>(array_size)
                                                        , [=](hc::index<1> i) [[hc]] {
                                                          view_c[i] = _c;
                                                        });
  try{
    future_a.wait();
    future_b.wait();
    future_c.wait();
  }
  catch(std::exception& e){
    std::cerr << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::init_arrays " << e.what() << std::endl;
    throw;
  }

}

template <class T>
void HCStream<T>::read_arrays(std::vector<T>& a, std::vector<T>& b, std::vector<T>& c)
{
  hc::copy(d_a,a.begin());
  hc::copy(d_b,b.begin());
  hc::copy(d_c,c.begin());
}


template <class T>
void HCStream<T>::copy()
{

  hc::array_view<T,1> view_a = this->d_a;
  hc::array_view<T,1> view_c = this->d_c;

  try{
    hc::completion_future future_kernel = hc::parallel_for_each(hc::extent<1>(array_size)
                                , [=](hc::index<1> index) [[hc]] {
                                  view_c[index] = view_a[index];
								});
    future_kernel.wait();
  }
  catch(std::exception& e){
    std::cerr << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::copy " << e.what() << std::endl;
    throw;
  }
}

template <class T>
void HCStream<T>::mul()
{

  const T scalar = startScalar;
  hc::array_view<T,1> view_b = this->d_b;
  hc::array_view<T,1> view_c = this->d_c;

  try{
    hc::completion_future future_kernel = hc::parallel_for_each(hc::extent<1>(array_size)
                                , [=](hc::index<1> i) [[hc]] {
                                  view_b[i] = scalar*view_c[i];
								});
    future_kernel.wait();
  }
  catch(std::exception& e){
    std::cerr << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::mul " << e.what() << std::endl;
    throw;
  }
}


template class HCStream<float>;
template class HCStream<double>;
