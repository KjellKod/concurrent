/* 
 * File:   test_helper.hpp
 * Author: kjell
 *
 * Created on April 14, 2014, 12:47 AM
 */

#pragma once


#include <random>
#include <iostream>
#include <string>
#include <cassert>
#include <chrono>
#include <memory>
#include "moveoncopy.hpp"

namespace test_helper {
   typedef std::chrono::steady_clock clock;

   /** Random function from http://www2.research.att.com/~bs/C++0xFAQ.html#std-random */
   int random_int(int low, int high) {
      using namespace std;
      static std::random_device rd; // Seed with a real random value, if available
      static default_random_engine engine{rd()};
      typedef uniform_int_distribution<int> Distribution;
      static Distribution distribution{};

      return distribution(engine, Distribution::param_type{low, high});
   }

   struct DummyObject {
      void doNothing() {}
   };

   struct DummyStringObject {
      std::string sayHello() {
         return {"Hello World"};
      }
   };


   typedef std::unique_ptr<DummyStringObject> UniqueDummyType;

   struct DummyObjectWithUniqueType {
      std::string talkBack(MoveOnCopy<UniqueDummyType> obj) {
         return obj.get()->sayHello();
      }
   };

   struct DummyObjectWithUniqueString {
      std::string talkBack(std::unique_ptr<std::string> str) {
         return *(str.get());
      }
      std::string talkBack2(std::shared_ptr<std::string> str) {
         return *(str.get());
      }
 
      std::string talkBack3(MoveOnCopy<std::unique_ptr<std::string>> str) {
         return *(str.get());
      }
};
   

   /** Verify clean destruction */
   struct TrueAtExit {
      std::atomic<bool>* flag;

      explicit TrueAtExit(std::atomic<bool>* f) : flag(f) {
         flag->store(false);
      }

      bool value() {
         return *flag;
      }

      ~TrueAtExit() {
         flag->store(true);
      }
      // concurrent improvement from the original Herb Sutter example
      // Which would copy/move the object into the concurrent wrapper
      // i.e the original concurrent wrapper could not use an object such as this (or a unique_ptr for that matter)
      TrueAtExit(const TrueAtExit&) = delete;
      TrueAtExit& operator=(const TrueAtExit&) = delete;
   };



   /** Verify concurrent runs,. "no" delay for the caller. */
   struct DelayedCaller {
      void DoDelayedCall() {
         std::this_thread::sleep_for(std::chrono::seconds(1));
      }
   };


   /** To verify that it is FIFO access */
   class FlipOnce {
      std::atomic<size_t>* _stored_counter;
      std::atomic<size_t>* _stored_attempts;
      bool _is_flipped;
      size_t _counter;
      size_t _attempts;

   public:

      explicit FlipOnce(std::atomic<size_t>* c, std::atomic<size_t>* t)
      : _stored_counter(c), _stored_attempts(t)
      , _is_flipped(false), _counter(0), _attempts(0) {
      }

      ~FlipOnce() {
         if (0 == *_stored_counter) {
            //FlipOnce with NO atomics in the doFlip operation
            (*_stored_counter) = _counter;
            (*_stored_attempts) = _attempts;
         } else {
            // FlipOnce WITH atomics in the doFlipAtomic operation
            assert(0 == _counter);
            assert(0 == _attempts);
         }
      }

      /** Void flip will  count up NON ATOMIC internal variables. They are non atomic to avoid 
      * any kind of unforseen atomic synchronization. Only in the destructor will the values 
      * be saved to the atomic storages
      */ 
      void doFlip() {
         if (!_is_flipped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(random_int(0, 1000)));
            _is_flipped = true;
            _counter++;
         }
         _attempts++;
      }

      void doFlipAtomic() {
         if (!_is_flipped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(random_int(0, 1000)));
            _is_flipped = true;
            (*_stored_counter)++;
         }
         (*_stored_attempts)++;
      }
   };

   struct Animal {
      virtual std::string sound() = 0;
   };

   struct Dog : public Animal {
      std::string sound() override {
         return
         {
            "Wof Wof"
         };
      }
   };

   struct Cat : public Animal {

      std::string sound() override {
         return
         {
            "Miauu Miauu"
         };
      }
   };

} // namespace test_helper