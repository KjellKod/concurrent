/** ==========================================================================
 * 2013 This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 * 
 * These type traits will be in C++17.
 * 
 * PUBLIC DOMAIN and NOT under copywrite protection.
 *
 * ********************************************* */

#ifndef STD2_TYPE_TRAITS_HPP_
#define STD2_TYPE_TRAITS_HPP_

#include <type_traits>

namespace std2 {

#if defined(__cplusplus) && (__cplusplus < 201703L)  // < C++17 so should define these.
   template<typename T>
   constexpr bool is_nothrow_move_constructible_v = std::is_nothrow_move_constructible<T>::value;

   template<typename T>
   constexpr bool is_nothrow_move_assignable_v = std::is_nothrow_move_assignable<T>::value;
#else  // C++17 already has these..
   template<typename T>
   inline constexpr bool is_nothrow_move_constructible_v = std::is_nothrow_move_constructible_v<T>;

   template<typename T >
   inline constexpr bool is_nothrow_move_assignable_v = std::is_nothrow_move_assignable_v<T>;
#endif
}

#endif
