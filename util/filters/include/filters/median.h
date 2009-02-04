/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FILTERS_MEDIAN_H_
#define FILTERS_MEDIAN_H_

#include <stdint.h>
#include <sstream>
#include "filters/filter_base.h"
#include "ros/assert.h"
/*
 * Algorithm from N. Wirth's book, implementation by N. Devillard.
 * This code in public domain.
 */
#define ELEM_SWAP(a,b) { register elem_type t=(a);(a)=(b);(b)=t; }

namespace filters
{
/*---------------------------------------------------------------------------
  Function : kth_smallest()
  In : array of elements, # of elements in the array, rank k
  Out : one element
  Job : find the kth smallest element in the array
  Notice : use the median() macro defined below to get the median.
  Reference:
  Author: Wirth, Niklaus
  Title: Algorithms + data structures = programs
  Publisher: Englewood Cliffs: Prentice-Hall, 1976
  Physical description: 366 p.
  Series: Prentice-Hall Series in Automatic Computation
  ---------------------------------------------------------------------------*/
template <typename elem_type>
elem_type kth_smallest(elem_type a[], int n, int k)
{
  register int i,j,l,m ;
  register elem_type x ;
  l=0 ; m=n-1 ;
  while (l<m) {
    x=a[k] ;
    i=l ;
    j=m ;
    do {
      while (a[i]<x) i++ ;
      while (x<a[j]) j-- ;
      if (i<=j) {
        ELEM_SWAP(a[i],a[j]) ;
        i++ ; j-- ;
      }
    } while (i<=j) ;
    if (j<k) l=i ;
    if (k<i) m=j ;
  }
  return a[k] ;
}
#define median(a,n) kth_smallest(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))
#undef ELEM_SWAP

/** \brief A median filter which works on arrays.
 *
 */
template <typename T>
class MedianFilter: public filters::FilterBase <T>
{
public:
  /** \brief Construct the filter with the expected width and height */
  MedianFilter();

  /** \brief Destructor to clean up
   */
  ~MedianFilter();

  virtual bool configure(unsigned int elements, const std::string& arguments);

  /** \brief Update the filter and return the data seperately
   * \param data_in double array with length width
   * \param data_out double array with length width
   */
  virtual bool update(const T& data_in, T& data_out);

protected:
  ///\todo change to vector
  T temp_storage_;                       ///< Preallocated storage for the list to sort
  T data_storage_;                       ///< Storage for data between updates

  uint32_t last_updated_row_;                   ///< The last row to have been updated by the filter
  uint32_t iterations_;                         ///< Number of iterations up to number of observations

  uint32_t number_of_observations_;             ///< Number of observations over which to filter
  uint32_t width_;           ///< Number of elements per observation

  bool configured_;  ///< Whether the filter has been configured

};

template <typename T>
MedianFilter<T>::MedianFilter():
  last_updated_row_(0),
  iterations_(0),
  number_of_observations_(0),
  width_(0),
  configured_(false)
{
  
};

template <typename T>
MedianFilter<T>::~MedianFilter()
{
};


template <typename T>
bool MedianFilter<T>::configure(uint32_t width, const std::string& number_of_observations)
{
  if (configured_)
    return false;
  width_ = width;
  std::stringstream ss;
  ss << number_of_observations;
  ss >> number_of_observations_;
  data_storage_.resize(number_of_observations_ * width_);
  temp_storage_.resize(width_);
  last_updated_row_ = number_of_observations_;
  configured_ = true;
  return true;
};

template <typename T>
bool MedianFilter<T>::update(const T& data_in, T& data_out)
{
  //  printf("Expecting width %d, got %d and %d\n", width_, data_in.size(),data_out.size());
  //  ROS_ASSERT(data_in.size() == width_);
  // ROS_ASSERT(data_out.size() == width_);
  if (data_in.size() != width_ || data_out.size() != width_)
    return false;
  if (!configured_)
    return false;
  //update active row
  if (last_updated_row_ >= number_of_observations_ - 1)
    last_updated_row_ = 0;
  else
    last_updated_row_++;

  //copy incoming data into perminant storage
  memcpy(&data_storage_[width_ * last_updated_row_],
         &data_in[0],
         sizeof(data_in[0]) * width_);

  //Return values

  //keep track of number of rows used while starting up
  uint32_t length;
  if (iterations_ < number_of_observations_ )
  {
    iterations_++;
    length = iterations_;
  }
  else //all rows are allocated
  {
    length = number_of_observations_;
  }

  //Return each value
  for (uint32_t i = 0; i < width_; i++)
  {
    for (uint32_t row = 0; row < length; row ++)
    {
      temp_storage_[row] = data_storage_[i + row * width_];
    }
    data_out[i] = median(&temp_storage_[0], length);
  }

  return true;
};

ROS_REGISTER_FILTER(MedianFilter, std_vector_double)
ROS_REGISTER_FILTER(MedianFilter, std_vector_float)
}


#endif //#ifndef FILTERS_MEDIAN_H_
