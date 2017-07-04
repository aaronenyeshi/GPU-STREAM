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
  d_b = (T*) hc::am_alloc(array_size * sizeof(T), acc, 0);
  d_c = (T*) hc::am_alloc(array_size * sizeof(T), acc, 0);

}


template <class T>
HCStream<T>::~HCStream()
{
  hc::am_free(d_a);
  hc::am_free(d_b);
  hc::am_free(d_c);
}

template <class T>
void HCStream<T>::init_arrays(T _a, T _b, T _c)
{
  T* a = (T*) malloc(array_size * sizeof(T));
  T* b = (T*) malloc(array_size * sizeof(T));
  T* c = (T*) malloc(array_size * sizeof(T));
  for (int i = 0; i < array_size; i++) {
    a[i] = _a;
    b[i] = _b;
    c[i] = _c;
  }
  auto acc = hc::accelerator();
  hc::accelerator_view av = acc.get_default_view();
  av.copy(a, d_a, array_size*sizeof(T));
  av.copy(b, d_b, array_size*sizeof(T));
  av.copy(c, d_c, array_size*sizeof(T));

  //hc::completion_future future_a= hc::copy_async(a, d_a);
  //hc::completion_future future_b= hc::copy_async(b, d_b);
  //hc::completion_future future_c= hc::copy_async(c, d_c);

  free(a);
  free(b);
  free(c);
/*  try{
    future_a.wait();
    future_b.wait();
    future_c.wait();
  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::init_arrays " << e.what() << std::endl;
    throw;
  }*/

}

template <class T>
void HCStream<T>::read_arrays(std::vector<T>& a, std::vector<T>& b, std::vector<T>& c)
{
  //hc::copy(d_a,a.begin());
  //hc::copy(d_b,b.begin());
  //hc::copy(d_c,c.begin());
}


template <class T>
void HCStream<T>::copy()
{
  T* d_cc = d_c;
  T* d_aa = d_a;
  hc::extent<1> e(array_size);
  try{
    //hc::completion_future future_kernel =
    hc::parallel_for_each(
      e,
      [=](hc::index<1> index) [[hc]] {

        d_cc[index[0]] = d_aa[index[0]];
    });

    // create a barrier packet
   // hc::accelerator_view av = hc::accelerator().get_default_view();
    //hc::completion_future fut = av.create_marker();
    // wait on the barrier packet
    //fut.wait();

    //future_kernel.wait();
  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::copy " << e.what() << std::endl;
    throw;
  }
}

template <class T>
void HCStream<T>::mul()
{
  T* d_bb = d_b;
  T* d_cc = d_c;
  const T scalar = startScalar;
  hc::extent<1> e(array_size);

  try{
    //hc::completion_future future_kernel =
    hc::parallel_for_each(
      e,
      [=](hc::index<1> i) [[hc]] {
        d_bb[i[0]] = scalar*d_cc[i[0]];
    });

    // create a barrier packet
    //hc::accelerator_view av = hc::accelerator().get_default_view();
    //hc::completion_future fut = av.create_marker();
    // wait on the barrier packet
    //fut.wait();

    //future_kernel.wait();
  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::mul " << e.what() << std::endl;
    throw;
  }
}

template <class T>
void HCStream<T>::add()
{


  //hc::array_view<T,1> view_a(this->d_a);
  //hc::array_view<T,1> view_b(this->d_b);
  //hc::array_view<T,1> view_c(this->d_c);
  hc::extent<1> e(array_size);
  T* d_cc = d_c;
  T* d_aa = d_a;
  T* d_bb = d_b;

  try{
    //hc::completion_future future_kernel =
    hc::parallel_for_each(
      e,
      [=](hc::index<1> i) [[hc]] {
        d_cc[i[0]] = d_aa[i[0]]+d_bb[i[0]];
    });

    // create a barrier packet
    //hc::accelerator_view av = hc::accelerator().get_default_view();
    //hc::completion_future fut = av.create_marker();
    // wait on the barrier packet
    //fut.wait();

    //future_kernel.wait();
  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::add " << e.what() << std::endl;
    throw;
  }
}

template <class T>
void HCStream<T>::triad()
{

  T scalar = startScalar;
  //hc::array_view<T,1> view_a(this->d_a);
  //hc::array_view<T,1> view_b(this->d_b);
  //hc::array_view<T,1> view_c(this->d_c);
  hc::extent<1> e(array_size);
  T* d_aa = d_a;
  T* d_bb = d_b;
  T* d_cc = d_c;

  try{
    //hc::completion_future future_kernel =
    hc::parallel_for_each(
      e,
      [=](hc::index<1> i) [[hc]] {
        d_aa[i[0]] = d_bb[i[0]] + scalar*d_cc[i[0]];
    });

    // create a barrier packet
    //hc::accelerator_view av = hc::accelerator().get_default_view();
    //hc::completion_future fut = av.create_marker();
    // wait on the barrier packet
    //fut.wait();



    //future_kernel.wait();
  }
  catch(std::exception& e){
    std::cout << __FILE__ << ":" << __LINE__ << "\t HCStream<T>::triad " << e.what() << std::endl;
    throw;
  }
}

template <class T>
T HCStream<T>::dot()
{
   //implementation adapted from
    //https://ampbook.codeplex.com/SourceControl/latest
    // ->Samples/CaseStudies/Reduction
    // ->CascadingReduction.h

    /*static constexpr std::size_t n_tiles = 64;

    const auto& view_a = d_a;
    const auto& view_b = d_b;

    auto ex = view_a.get_extent();
    const auto tiled_ex = hc::extent<1>(n_tiles * TBSIZE).tile(TBSIZE);
    const auto domain_sz = tiled_ex.size();

    hc::array<T, 1> partial(n_tiles);

    hc::parallel_for_each(tiled_ex,
                          [=,
                           &view_a,
                           &view_b,
                           &partial](const hc::tiled_index<1>& tidx) [[hc]] {

                            auto gidx = tidx.global[0];
        T r = T{0}; // Assumes reduction op is addition.
        while (gidx < view_a.get_extent().size()) {
            r += d_a[gidx] * d_b[gidx];
            gidx += domain_sz;
        }

        tile_static T tileData[TBSIZE];
        tileData[tidx.local[0]] = r;

        tidx.barrier.wait_with_tile_static_memory_fence();

        for (auto h = TBSIZE / 2; h; h /= 2) {
            if (tidx.local[0] < h) {
                tileData[tidx.local[0]] += tileData[tidx.local[0] + h];
            }
            tidx.barrier.wait_with_tile_static_memory_fence();
        }

        if (tidx.global == tidx.tile_origin) partial[tidx.tile] = tileData[0];
    });

    try {
        partial.get_accelerator_view().wait();
    }
    catch (std::exception& e) {
        std::cout << __FILE__ << ":" << __LINE__ << "\t  HCStream<T>::dot " << e.what() << std::endl;
        throw;
    }

    std::vector<T> h_partial(n_tiles,0);
    hc::copy(partial,h_partial.begin());

    T result = std::accumulate(h_partial.begin(), h_partial.end(), 0.);

    return result;*/return T(0);

}

template class HCStream<float>;
template class HCStream<double>;
