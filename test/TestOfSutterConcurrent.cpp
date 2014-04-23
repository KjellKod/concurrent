#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <memory>
#include <iostream>
#include <thread>
#include <future>


#include "concurrent.hpp"
#include "test_helper.hpp"
#include "std2_make_unique.hpp"

using namespace test_helper;

TEST(TestOfSutterConcurrent, CompilerCheckForEmptyStruct) {
   concurrent<DummyObject> doNothing1{};
   concurrent<DummyObject> doNothing2;
   concurrent<DummyObject> doNothing3 = {};
}


TEST(TestOfSutterConcurrent, ThrowingConstructor) {
   EXPECT_THROW(concurrent<ThrowUp> cs{" bad soup "}, std::runtime_error);
}
   

TEST(TestOfSutterConcurrent, Empty) {
   concurrent<std::string> cs{std::unique_ptr<std::string>{nullptr}};
   EXPECT_TRUE(cs.empty());
     
   // Calling an empty concurrent object will throw
   auto result = cs.lambda( [](std::string& s) { 
      return s.substr(0, std::string::npos); });
   EXPECT_ANY_THROW(result.get());  
}


TEST(TestOfSutterConcurrent, Clear_plain) {
   concurrent<std::string> cs{"Hello World"};
   EXPECT_EQ("Hello World", cs.lambda([](std::string & str) {
                                         return str.substr(0, std::string::npos); 
                                      }).get());
   EXPECT_FALSE(cs.empty());
}

TEST(TestOfSutterConcurrent, Hello_World) {
   concurrent<std::string> cs{std2::make_unique<std::string>("Hello World")};
   EXPECT_FALSE(cs.empty());
   EXPECT_EQ("Hello World", cs.lambda([](std::string & str) {
                                         return str.substr(0, std::string::npos); 
                                      }).get());
}

/** Oops. The straight forward approach can also be backwards */
TEST(TestOfConcurrent, No_Issue_with_Overloads) {
   concurrent<std::string> hello;

   auto response = hello.lambda( [](std::string& s) {
      s.append("Hello World");
      return s; });
      
   EXPECT_EQ("Hello World", response.get());   
}




TEST(TestOfSutterConcurrent, AbstractInterface__Works__Fine) {
   concurrent<Animal> animal1{std::unique_ptr<Animal>(new Dog)};  // two example how this can be achieved
   concurrent<Animal> animal2{std::unique_ptr<Animal>(new Cat)};

   auto make_sound = [](Animal& animal) { return animal.sound();  };
   
  EXPECT_EQ("Wof Wof", animal1.lambda(make_sound).get());
  EXPECT_EQ("Miauu Miauu", animal2.lambda(make_sound).get());
  
}

TEST(TestOfSutterConcurrent, VerifyDestruction) {
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







TEST(TestOfSutterConcurrent, VerifyFifoCalls) {

   concurrent<std::string> asyncString = {"start"};
   auto received = asyncString.lambda([](std::string & s) {
      s.append(" received message"); return std::string{s}; });

   auto clear = asyncString.lambda([](std::string & s) {
      s.clear(); return s; });

   EXPECT_EQ("start received message", received.get());
   EXPECT_EQ("", clear.get());

   std::string toCompare;
   for (size_t index = 0; index < 100000; ++index) {
      toCompare.append(std::to_string(index)).append(" ");
      asyncString.lambda([ = ](std::string & s){s.append(std::to_string(index)).append(" ");});
   }

   auto appended = asyncString.lambda([](const std::string & s) {
      return s; });
   EXPECT_EQ(appended.get(), toCompare);
}



TEST(TestOfSutterConcurrent, VerifyImmediateReturnForSlowFunctionCalls) {
   auto start = clock::now();
   {
      concurrent<DelayedCaller> snail;
      for (size_t call = 0; call < 10; ++call) {
         snail.lambda([](DelayedCaller & slowRunner) {
            slowRunner.DoDelayedCall(); });
      }
      EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count(), 1);
   } // at destruction all 1 second calls will be executed before we quit

   EXPECT_TRUE(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count() >= 10); // 
}

TEST(TestOfSutterConcurrent, unique_ptr_wrapps_concurrent) {
   std::unique_ptr<concurrent<Greeting>> gossip;
   gossip.reset(new concurrent<Greeting>());
   auto tjena = gossip->call(&Greeting::sayHello);
   EXPECT_EQ(tjena.get(), "Hello World");
}






TEST(TestOfSutterConcurrent, IsConcurrentReallyAsyncWithFifoGuarantee__AtomicInside_Wait1Minute) {
   std::cout << "100 thread runs. Please wait a a bit" << std::endl;

   std::vector<std::future<void>> result;

   std::atomic<size_t> count_of_flip{0};
   std::atomic<size_t> total_thread_access{0};
   concurrent<FlipOnce> flipOnceObject{&count_of_flip, &total_thread_access};
   ASSERT_EQ(0, count_of_flip);

   for (size_t howmanyflips = 0; howmanyflips < 100; ++howmanyflips) {
      std::cout << "." << std::flush;
      result.push_back(std::async(std::launch::async, DoALambdaFlipAtomic, std::ref(flipOnceObject)));
   }

   // wait for all the async to finish.
   for (auto& res : result) {
      res.get(); // future of future
   }

   EXPECT_EQ(1, count_of_flip);
   EXPECT_EQ(100, total_thread_access);

   std::cout << std::endl;
}



