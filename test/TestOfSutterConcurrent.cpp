#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <memory>
#include <iostream>
#include <thread>
#include <future>


#include "sutter_concurrent.hpp"
#include "test_helper.hpp"


using namespace test_helper;
using namespace sutter;

TEST(TestOfConcurrent, CompilerCheckForEmptyStruct) {
   concurrent<DummyObject> doNothing1{};
   concurrent<DummyObject> doNothing2;
   concurrent<DummyObject> doNothing3 = {};
}

TEST(TestOfConcurrent, CompilerCheckUniquePtrTest) {
   typedef std::unique_ptr<Animal> RaiiAnimal;
   concurrent<RaiiAnimal> animal1{new Dog};
   concurrent<RaiiAnimal> animal2{new Cat};

   auto make_sound = [](RaiiAnimal & animal) {
      return animal->sound();
   };
   EXPECT_EQ("Wof Wof", animal1(make_sound).get());
   EXPECT_EQ("Miauu Miauu", animal2(make_sound).get());
}



TEST(TestOfConcurrent, VerifyDestruction) {
   std::atomic<bool> flag{true};
   {
      concurrent<TrueAtExit> notifyAtExit1{&flag};
      EXPECT_FALSE(flag); // i.e. constructor has run
   }
   {
      EXPECT_TRUE(flag); // notifyAtExit destructor
      concurrent<TrueAtExit> notifyAtExit2 = {&flag};
      EXPECT_FALSE(flag);
   }
   EXPECT_TRUE(flag); // notifyAtExit destructor
}



TEST(TestOfConcurrent, VerifyFifoCalls) {

   concurrent<std::string> asyncString = {"start"};
   auto received = asyncString([](std::string & s) {
      s.append(" received message"); return std::string{s}; });

   auto clear = asyncString([](std::string & s) {
      s.clear(); return s; });

   EXPECT_EQ("start received message", received.get());
   EXPECT_EQ("", clear.get());

   std::string toCompare;
   for (size_t index = 0; index < 100000; ++index) {
      toCompare.append(std::to_string(index)).append(" ");
      asyncString([ = ](std::string & s){s.append(std::to_string(index)).append(" ");});
   }

   auto appended = asyncString([](const std::string & s) {
      return s; });
   EXPECT_EQ(appended.get(), toCompare);
}



TEST(TestOfConcurrent, VerifyImmediateReturnForSlowFunctionCalls) {
   auto start = clock::now();
   {
      concurrent<DelayedCaller> snail;
      for (size_t call = 0; call < 10; ++call) {
         snail([](DelayedCaller & slowRunner) {
            slowRunner.DoDelayedCall(); });
      }
      EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count(), 1);
   } // at destruction all 1 second calls will be executed before we quit

   EXPECT_TRUE(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() >= 10); // 
}



std::future<void> DoAFlip(concurrent<FlipOnce>& flipper) {
   return flipper([] (FlipOnce & obj) {
      obj.doFlip(); });
}
TEST(TestOfConcurrent, IsConcurrentReallyAsyncWithFifoGuarantee__Wait1Minute) {
   std::cout << "60*10 thread runs. Please wait a minute" << std::endl;
   for (size_t howmanyflips = 0; howmanyflips < 60; ++howmanyflips) {
      std::cout << "." << std::flush;
      std::atomic<size_t> count_of_flip{0};
      std::atomic<size_t> total_thread_access{0};
      {
         concurrent<FlipOnce> flipOnceObject{&count_of_flip, &total_thread_access};
         ASSERT_EQ(0, count_of_flip);
         auto try0 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try1 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try2 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try3 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try4 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try5 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try6 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try7 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try8 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));
         auto try9 = std::async(std::launch::async, DoAFlip, std::ref(flipOnceObject));

         // scope exit. ALL jobs will be executed before this finished. 
         //This means that all 10 jobs in the loop must be done
         // all 10 will wait here till they are finished
      }
      ASSERT_EQ(1, count_of_flip);
      ASSERT_EQ(10, total_thread_access);
   }
   std::cout << std::endl;
}

std::future<void> DoAFlipAtomic(concurrent<FlipOnce>& flipper) {
   return flipper([] (FlipOnce & obj) {
      obj.doFlipAtomic(); });
}
TEST(TestOfConcurrent, IsConcurrentReallyAsyncWithFifoGuarantee__AtomicInside_Wait1Minute) {
   std::cout << "60*10 thread runs. Please wait a minute" << std::endl;
   for (size_t howmanyflips = 0; howmanyflips < 60; ++howmanyflips) {
      std::cout << "." << std::flush;

      std::atomic<size_t> count_of_flip{0};
      std::atomic<size_t> total_thread_access{0};
      {
         concurrent<FlipOnce> flipOnceObject{&count_of_flip, &total_thread_access};
         ASSERT_EQ(0, count_of_flip);
         auto try0 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try1 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try2 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try3 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try4 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try5 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try6 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try7 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try8 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));
         auto try9 = std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject));

         // scope exit. ALL jobs will be executed before this finished. 
         //This means that all 10 jobs in the loop must be done
         // all 10 will wait here till they are finished
      }
      ASSERT_EQ(1, count_of_flip);
      ASSERT_EQ(10, total_thread_access);
   }
   std::cout << std::endl;

}



