/*
 * profile.h
 *
 *  Created on: Aug 3, 2021
 *      Author: Linh
 */

#ifndef SRC_INCLUDE_PROFILE_H_
#define SRC_INCLUDE_PROFILE_H_

#include <chrono>
#include <iostream>

//#define PROFILE_ENABLE

#ifdef PROFILE_ENABLE
#define __TIC__(tag) \
  auto __##tag##_start_time = std::chrono::steady_clock::now();

#define __TOC__(tag)                                                  \
  auto __##tag##_end_time = std::chrono::steady_clock::now();         \
  std::cout << #tag << " : "                                          \
            << std::chrono::duration_cast<std::chrono::microseconds>( \
                   __##tag##_end_time - __##tag##_start_time)         \
                   .count()                                           \
            << "us" << std::endl;

#define __TIC_SUM__(tag)                 \
  static auto __##tag##_total_time = 0U; \
  auto __##tag##_start_time = std::chrono::steady_clock::now();

#define __TOC_SUM__(tag)                                      \
  auto __##tag##_end_time = std::chrono::steady_clock::now(); \
  __##tag##_total_time +=                                     \
      std::chrono::duration_cast<std::chrono::microseconds>(  \
          __##tag##_end_time - __##tag##_start_time)          \
          .count();                                           \
  std::cout << #tag << " : " << __##tag##_total_time << "us" << std::endl;

#define __TIC_GLOBAL__(tag) \
		start_time = std::chrono::steady_clock::now();

#define __TOC_GLOBAL__(tag) \
		end_time = std::chrono::steady_clock::now(); 				\
		std::cout << #tag << " : " 					 				\
		<< std::chrono::duration_cast<std::chrono::microseconds>(  	\
		          end_time - start_time)          					\
		          .count()                                          \
		<< "us" << std::endl;
#else
#define __TIC__(tag)
#define __TOC__(tag)
#define __TIC_SUM__(tag)
#define __TOC_SUM__(tag)
#define __TIC_GLOBAL__(tag)
#define __TOC_GLOBAL__(tag)
#endif


#endif /* SRC_INCLUDE_PROFILE_H_ */
