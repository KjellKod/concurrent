#include <gtest/gtest.h>
#include <string>
#include <atomic>
#include <memory>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <future>
#include <cassert>
#include <iostream>
#include "concurrent.hpp"
#include "moveoncopy.hpp"

#include "test_helper.hpp"
using namespace test_helper;


TEST(TestOfConcurrent, CompilerCheckForEmptyStruct) {
   concurrent<DummyObject> doNothing1{};
   concurrent<DummyObject> doNothing2;
   concurrent<DummyObject> doNothing3 ;
   EXPECT_FALSE(doNothing2.empty());
   EXPECT_FALSE(doNothing3.empty());
}

TEST(TestOfConcurrent, CompilerCheckForVoidCall) {
   concurrent<DummyObject> doNothing1{};
   doNothing1.call(&DummyObject::doNothing);
   EXPECT_FALSE(doNothing1.empty());
}

TEST(TestOfConcurrent, CompilerCheckForStringCall) {
   concurrent<Greeting> hello;
   EXPECT_EQ("Hello World", hello.call(&Greeting::sayHello).get());
}

// Example on how to receive unique_ptr' or other un-copyable objects
TEST(TestOfConcurrent, CompilerCheckForStringCallWithObjectArg) {
   concurrent<GreetingWithUnique> hello;
   UniqueGreeting  arg{new Greeting};
   auto futureHello = hello.call(&GreetingWithUnique::talkBack, MoveOnCopy<UniqueGreeting>(std::move(arg)));
   EXPECT_EQ("Hello World", futureHello.get());
}

// using the object without concurrent wrapper., this can receive
// unique ptr args without any problems
TEST(TestOfConcurrent, CompilerCheckForUniqueArg) {
   DummyObjectWithUniqueString hello;
   std::unique_ptr<std::string> msg(new std::string{"Hello World"});
   EXPECT_EQ("Hello World", hello.talkBack(std::move(msg)));
}

// shared_ptr as args are of course
TEST(TestOfConcurrent, CompilerCheckForConcurrentUniqueArg) {
   concurrent<DummyObjectWithUniqueString> hello;
   std::shared_ptr<std::string> msg1(new std::string{"Hello World"});
   auto response1 = hello.call(&DummyObjectWithUniqueString::talkBack2, std::move(msg1));
   EXPECT_EQ("Hello World", response1.get());

   std::unique_ptr<std::string> msg2(new std::string{"Hello World"});
   auto response2 = hello.call(&DummyObjectWithUniqueString::talkBack3, MoveOnCopy<std::unique_ptr<std::string>>(std::move(msg2)));
   EXPECT_EQ("Hello World", response2.get());

}


TEST(TestOfConcurrent, Empty) {
   concurrent<std::string> cs{std::unique_ptr<std::string>{nullptr}};
   EXPECT_TRUE(cs.empty());

   // Calling an empty concurrent object will throw
   EXPECT_ANY_THROW(cs.call(&std::string::substr, 0, std::string::npos).get());
}


TEST(TestOfConcurrent, Is_Not_Empty) {
   concurrent<std::string> cs{"Hello World"};
   EXPECT_EQ("Hello World", cs.call(&std::string::substr, 0, std::string::npos).get());
   EXPECT_FALSE(cs.empty());
}

TEST(TestOfConcurrent, Hello_World) {
   concurrent<std::string> cs{std2::make_unique<std::string>("Hello World")};
   EXPECT_FALSE(cs.empty());
   EXPECT_EQ("Hello World", cs.call(&std::string::substr, 0, std::string::npos).get());
}


/** Oops. The straight forward approach can also be backwards */
TEST(TestOfConcurrent, KlunkyUsage__Disambiguity__overloads) {
   concurrent<std::string> hello;
   // Unfortunately this does not compile. It cannot deduce the function pointer since
   // the std::string::append has overloads
   //auto response = hello.call(&std::string::append, msg);

   // A very cumbersome work-around exist. Typedef the function pointer.
   // Set it and use it. ... So in this instance the Sutter approach would be
   // way easier.
   typedef std::string&(std::string::*append_type)(const std::string&);
   append_type appender = &std::string::append;
   auto response = hello.call(appender, "Hello World");
   EXPECT_EQ("Hello World", response.get());
}


/** Oops. The straight forward approach can also be backwards */
TEST(TestOfConcurrent, KlunkyUsage__Disambiguity__overloads_repeat) {
   concurrent<std::string> hello;
   typedef std::string&(std::string::*append_func)(const std::string&);
   append_func appender = &std::string::append;
   auto response = hello.call(appender, "Hello World");
   EXPECT_EQ("Hello World", response.get());
}



// This just don't work... At least not easily
TEST(TestOfConcurrent, AbstractInterface__Works__Fine) {
   concurrent<Animal> animal1{std::unique_ptr<Animal>(new Dog)};
   concurrent<Animal> animal2{std::unique_ptr<Animal>(new Cat)};
   EXPECT_EQ("Wof Wof", animal1.call(&Animal::sound).get());
   EXPECT_EQ("Miauu Miauu", animal2.call(&Animal::sound).get());
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


TEST(TestOfConcurrent, VerifyImmediateReturnForSlowFunctionCalls) {
   auto start = clock::now();
   {
      concurrent<DelayedCaller> snail;
      for (size_t call = 0; call < 10; ++call) {
         snail.call(&DelayedCaller::DoDelayedCall);
      }
      EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - start).count(), 1);
   } // at destruction all 1 second calls will be executed before we quit

   EXPECT_TRUE(std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count() >= (10 * 200)); //
}




TEST(TestOfConcurrent, IsConcurrentReallyAsyncWithFifoGuarantee__AtomicInside_Wait1Minute) {
   std::cout << "100 thread runs. Please wait a minute" << std::endl;
   std::vector<std::future<void>> result;

   std::atomic<size_t> count_of_flip{0};
   std::atomic<size_t> total_thread_access{0};
   concurrent<FlipOnce> flipOnceObject{&count_of_flip, &total_thread_access};
   ASSERT_EQ(0, count_of_flip);

   for (size_t howmanyflips = 0; howmanyflips < 100; ++howmanyflips) {
      std::cout << "." << std::flush;
      result.push_back(std::async(std::launch::async, DoAFlipAtomic, std::ref(flipOnceObject)));
   }

   // wait for all the async to finish.
   for (auto& res : result) {
      res.get(); // future of future
   }

   EXPECT_EQ(1, count_of_flip);
   EXPECT_EQ(100, total_thread_access);
   std::cout << std::endl;

}

struct AddInt {
   std::vector<int>& collectedValues;
   AddInt(std::vector<int>& values) : collectedValues(values) {}
   void Add(int value) {
      collectedValues.push_back(value);
   }
};


struct ConcurrentAddInt : public concurrent<AddInt> {
   ConcurrentAddInt(std::vector<int>& values) : concurrent<AddInt>(values) {}
   virtual ~ConcurrentAddInt() = default;

   size_t size() override {
      return concurrent<AddInt>::size();
   }
};

TEST(TestOfConcurrent, Verify100FireCallsAreAsynchronous) {
   std::vector<int> values;
   std::vector<int> expected;
   int stoppedAt = 0;
   int sizeWhenStopped = 0;
   {
      // RAII enforced
      ConcurrentAddInt addInt(values);
      // Add a async call and wait for it to complete. That way
      // we know that the background thread is asynchronous
      addInt.call(&AddInt::Add, 999).wait();
      expected.push_back(999);

      // Now execute many rapid calls. If it is asynchronous
      // then the queue should become larger than 1 since
      // it's "call" overhead to execute the call in the background thread
      for (int i = 0; i < 100; ++i) {
         auto size = addInt.size();
         if (size > 2) {
            stoppedAt = i - 1;
            sizeWhenStopped = size;

            break;
         }
         addInt.fire(&AddInt::Add, i);
         expected.push_back(i);
      }
   } // RAII enforces flush

   EXPECT_EQ(expected.size(), values.size());
   EXPECT_TRUE(std::equal(values.begin(), values.end(), expected.begin()));
   EXPECT_TRUE(stoppedAt > 1) << "Stopped at: " << stoppedAt << ", size: " << sizeWhenStopped;
}



