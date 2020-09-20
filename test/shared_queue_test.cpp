#include "shared_queue.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <type_traits>

TEST(TestOfSharedQueue, CompilerCheckForNoCopyConstructibleAndAssignable) {
   static_assert(std::is_copy_constructible<shared_queue<int>>::value == false,
      "Shared queue can't be copied by constructor");
   static_assert(std::is_copy_assignable<shared_queue<int>>::value == false,
      "Shared queue can't be copied by assignment operator");
}

TEST(TestOfSharedQueue, CompilerCheckForDefaultConstructible) {
   static_assert(std::is_default_constructible<shared_queue<int>>::value,
      "Shared queue is default constructible");
}

TEST(TestOfSharedQueue, EmptyOnCreate) {
   shared_queue<int> queue;

   ASSERT_TRUE(queue.empty());
   ASSERT_EQ(0U, queue.size());
}

TEST(TestOfSharedQueue, NotEmptyAfterPush) {
   shared_queue<int> queue;

   queue.push(12);

   ASSERT_FALSE(queue.empty());
   ASSERT_EQ(1U, queue.size());
}

TEST(TestOfSharedQueue, MultiplePushPopInFifoOrder) {
   shared_queue<int> queue;

   queue.push(12);
   queue.push(35);

   EXPECT_FALSE(queue.empty());
   EXPECT_EQ(2U, queue.size());

   int value{0};
   ASSERT_TRUE(queue.try_and_pop(value));
   ASSERT_EQ(12, value);

   ASSERT_TRUE(queue.try_and_pop(value));
   ASSERT_EQ(35, value);

   ASSERT_TRUE(queue.empty());
   ASSERT_EQ(0U, queue.size());
}

TEST(TestOfSharedQueue, TryAndPopOnEmptyNotChangeValue) {
   shared_queue<int> queue;

   int value{42};

   ASSERT_FALSE(queue.try_and_pop(value));
   ASSERT_EQ(42, value);
   ASSERT_TRUE(queue.empty());
   ASSERT_EQ(0U, queue.size());
}

TEST(TestOfSharedQueue, TryAndPopOnNotEmptyGetValue) {
   shared_queue<int> queue;

   queue.push(42);

   int value{0};

   ASSERT_TRUE(queue.try_and_pop(value));
   ASSERT_EQ(42, value);
   EXPECT_TRUE(queue.empty());
   EXPECT_EQ(0U, queue.size());
}

TEST(TestOfSharedQueue, WaitAndPopWaitsForValue) {
   shared_queue<int> queue;

   constexpr int produced_value{12};

   std::thread producer{[&](shared_queue<int> &storage) {
      using namespace std::chrono_literals;

      std::this_thread::sleep_for(0.5s);
      queue.push(produced_value);
   }, std::ref(queue)};

   int consumed_value{0};

   std::thread consumer{[&](shared_queue<int> &storage) {
     storage.wait_and_pop(consumed_value);
   }, std::ref(queue)};
  
   producer.join();
   consumer.join();

   ASSERT_EQ(produced_value, consumed_value);
   EXPECT_TRUE(queue.empty());
   EXPECT_EQ(0U, queue.size());
}

namespace {
class CopyAndMovable {
public:
   CopyAndMovable() noexcept : moves_count_{0U}, copies_count_{0U} {}
  
   CopyAndMovable(CopyAndMovable &&m) noexcept
      : moves_count_{m.moves_count_ + 1}, copies_count_{m.copies_count_}
   {
      m.moves_count_ = 0;
      m.copies_count_ = 0;
   }
  
   CopyAndMovable& operator=(CopyAndMovable &&m) noexcept
   {
      using std::swap;

      swap(m.moves_count_, moves_count_);
      ++moves_count_;

      swap(m.copies_count_, copies_count_);
      return *this;
   }
  
   CopyAndMovable(const CopyAndMovable &m) noexcept
      : moves_count_{m.moves_count_}, copies_count_{m.copies_count_ + 1}
   {
   }
  
   CopyAndMovable& operator=(const CopyAndMovable &m) noexcept
   {
      moves_count_ = m.moves_count_;
      copies_count_ = m.copies_count_ + 1;
      return *this;
   }

   unsigned MovesCount() const noexcept {
      return moves_count_;
   }

   unsigned CopiesCount() const noexcept {
     return copies_count_;
   }

private:
   unsigned moves_count_;
   unsigned copies_count_;
};
} // namespace

TEST(TestOfSharedQueue, PushPopMovesValueIfPossible) {
   shared_queue<CopyAndMovable> queue;

   queue.push(CopyAndMovable{});
  
   CopyAndMovable cm;

   ASSERT_TRUE(queue.try_and_pop(cm));
   ASSERT_EQ(2U, cm.MovesCount());
   ASSERT_EQ(0U, cm.CopiesCount());
}

TEST(TestOfSharedQueue, PushWaitAndPopMovesValueIfPossible) {
  shared_queue<CopyAndMovable> queue;
  
  queue.push(CopyAndMovable{});
  
  CopyAndMovable cm;
  queue.wait_and_pop(cm);
  
  ASSERT_EQ(2U, cm.MovesCount());
  ASSERT_EQ(0U, cm.CopiesCount());
}

namespace {
struct CopyableOnly : public CopyAndMovable {
   CopyableOnly() noexcept = default;

   CopyableOnly(const CopyableOnly& m) noexcept = default;
   CopyableOnly& operator=(const CopyableOnly &m) noexcept = default;
};
} // namespace


TEST(TestOfSharedQueue, PushPopCopiesValueIfMoveNotPossible) {
   shared_queue<CopyableOnly> queue;

   CopyableOnly src;
   queue.push(src);

   CopyableOnly c;
   EXPECT_TRUE(queue.try_and_pop(c));
   
   EXPECT_EQ(0U, c.MovesCount());
   ASSERT_EQ(3U, c.CopiesCount());
}

TEST(TestOfSharedQueue, PushWaitAndPopCopiesValueIfMoveNotIfPossible) {
   shared_queue<CopyableOnly> queue;
  
   CopyableOnly src;
   queue.push(src);
  
   CopyableOnly dst;
   queue.wait_and_pop(dst);
  
   EXPECT_EQ(0U, dst.MovesCount());
   ASSERT_EQ(3U, dst.CopiesCount());
}
