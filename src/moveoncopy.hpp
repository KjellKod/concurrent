/** ==========================================================================
 * 2013 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 * ============================================================================*/

#pragma once

#include "std2_type_traits.hpp"

// From g3log: https://bitbucket.org/KjellKod/g3log
// A straightforward technique to move around packaged_tasks or anything else that is not copyable
//  Instances of std::packaged_task are MoveConstructible and MoveAssignable, but
//  not CopyConstructible or CopyAssignable. To put them in a std container they need
//  to be wrapped and their internals "moved" when tried to be copied.
template<typename Moveable>
struct MoveOnCopy {
   mutable Moveable _move_only;

   static constexpr bool is_nothrow_move_constructible{std2::is_nothrow_move_constructible_v<Moveable>};
   static constexpr bool is_nothrow_move_assignable{std2::is_nothrow_move_assignable_v<Moveable>};

   explicit MoveOnCopy(Moveable&& m) noexcept(is_nothrow_move_constructible) : _move_only(std::move(m)) { }
   MoveOnCopy(MoveOnCopy const& t) noexcept(is_nothrow_move_constructible) : _move_only(std::move(t._move_only)) { }
   MoveOnCopy(MoveOnCopy&& t) noexcept(is_nothrow_move_constructible) : _move_only(std::move(t._move_only)) { }

   MoveOnCopy& operator=(MoveOnCopy const& other) noexcept(is_nothrow_move_assignable) {
      _move_only = std::move(other._move_only);
      return *this;
   }

   MoveOnCopy& operator=(MoveOnCopy&& other) noexcept(is_nothrow_move_assignable) {
      _move_only = std::move(other._move_only);
      return *this;
   }

   void operator()() { _move_only(); }

   Moveable& get() {return _move_only; }

   Moveable release() {return std::move(_move_only); }
};


