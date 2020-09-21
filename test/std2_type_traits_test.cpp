#include "std2_type_traits.hpp"

#include <gtest/gtest.h>

namespace {
struct NothrowMoveConstructible {
   NothrowMoveConstructible(NothrowMoveConstructible&&) noexcept = default;
};

struct ThrowMoveConstructible {
   ThrowMoveConstructible(ThrowMoveConstructible&&) {};
};

struct NothrowMoveAssignable {
   NothrowMoveAssignable& operator=(NothrowMoveAssignable&&) noexcept {
      return *this; 
   }
};

struct ThrowMoveAssignable {
   ThrowMoveAssignable& operator=(ThrowMoveAssignable&&) {
      return *this;
   }
};
} // namespace


TEST(TestOfStdTypeTraits, CompilerCheckForNothrowMoveConstructible) {
   static_assert(std2::is_nothrow_move_constructible_v<NothrowMoveConstructible>,
      "NothrowMoveConstructible has no throwing move constructor");
   static_assert(std2::is_nothrow_move_constructible_v<ThrowMoveConstructible> == false,
      "ThrowMoveConstructible has throwing move constructor");
}

TEST(TestOfStdTypeTraits, CompilerCheckForNothrowMoveAssignable) {
   static_assert(std2::is_nothrow_move_assignable_v<NothrowMoveAssignable>,
      "NothrowMoveAssignable has no throwing move assignment operator");
   static_assert(std2::is_nothrow_move_assignable_v<ThrowMoveAssignable> == false,
      "ThrowMoveAssignable has throwing move assignment operator");
}


