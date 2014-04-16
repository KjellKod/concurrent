// PUBLIC DOMAIN LICENSE: https://github.com/KjellKod/Concurrent/blob/master/LICENSE
// 
// Part II --- Concurrent Wrapper
// ===============================
// Wrap "any" object to get concurrent access with asynchronous execution in FIFO  orer
// Improvement from:      http://kjellkod.wordpress.com/2014/04/07/concurrency-concurrent-wrapper/
//
// Two possible approaches that can be used. 
// =========================================
// 1) KjellKod's g3log approach to wrap any object
// struct Hello { void world() { cout << "Hello World" << endl; } };
//
//  concurrent<Hello> hello;
// concurrent.call(&world);
//
//
// 2) Herb Sutter's approach http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
//    wrap any object and call it though a lambda. Multiple things can be executed in the same asynchronous operation.
// concurrent<Hello> hello;
// concurrent( [](Hello& msg){ msg.world(); });
//
//
//
// Part I 
// ======================
// Published at:https://github.com/KjellKod/Concurrent and 
//      http://kjellkod.wordpress.com/2014/04/07/concurrency-concurrent-wrapper/
//
// What this "concurrent" is
// 1) The "concurrent" can wrap ANY object
// 2) All access to the concurrent is done though a lambda or using an easy pointer-to-member function call. 
// 3) All access call to the concurrent is done asynchronously but they are executed in FIFO order
// 4) At scope exit all queued jobs has to finish before the concurrent goes out of scope
#pragma once

#include <thread>
#include <future>
#include <functional>
#include <type_traits>
#include <memory>
#include <stdexcept>
#include "moveoncopy.hpp"
#include "shared_queue.hpp"   
#include "std2_make_unique.hpp" // until available in C++14

namespace concurrent_helper {
   typedef std::function<void() > Callback;

   /** helper for non-void promises */
   template<typename Fut, typename F, typename T>
   void set_value(std::promise<Fut>& p, F& f, T& t) {
      p.set_value(f(t));
   }
   
   /** helper for setting promise/exception for promise of void */
   template<typename F, typename T>
   void set_value(std::promise<void>& p, F& f, T& t) {
      f(t);
      p.set_value();
   }
} // namespace concurrent_helper


/**
 * Basically a light weight active object. www.kjellkod.cc/active-object-with-cpp0x#TOC-Active-Object-the-C-11-way
 * all input happens in the background. At shutdown it exits only after all 
 * queued requests are handled.
 */ 
template <class T> class concurrent {
   mutable std::unique_ptr<T> _worker;
   mutable shared_queue<concurrent_helper::Callback> _q;
   bool _done; // not atomic since only the bg thread is touching it
   std::thread _thd;

   void run() const {
      concurrent_helper::Callback call;
      while (!_done) {
         _q.wait_and_pop(call);
         call();
      }
   }

public:
   /**  Constructs an unique_ptr<T>  that is the background object 
    * @param args to contruct the unique_ptr<T> in-place
    */
   template<typename ... Args>
   concurrent(Args&&... args)
   : concurrent(std2::make_unique<T>(std::forward<Args>(args)...)) {}
   
   /**
    * Moves in a unique_ptr<T> to be the background object. Starts up the worker thread
    * @param workerto act as the background object
    */
   concurrent(std::unique_ptr<T> worker) 
   : _worker(std::move(worker))
   , _done(false)
   , _thd([ = ]{concurrent_helper::Callback call;
      while (_worker && !_done) { 
         _q.wait_and_pop(call);  call(); }}) 
    {     
    }

   virtual ~concurrent() {
      clear();
   }

   
   /**
    * Convenience function to stop the object before it is stopped in the destructor.
    */
   void clear() {
      _q.push([ = ]{_done = true;});
      if (_thd.joinable()) {
         _thd.join();
      }
      _worker.reset(nullptr);
   }
   
   /**
    * @return whether the background object is still active. If the thread is stopped then 
    * the background object will also be removed. 
    */
   bool empty() const { return !_worker; }
   
   /**
    *  Following Herb Sutter's approach for a concurrent wrapper
    * using std::promise and setting the value using a lambda approach
    * Example:   struct Hello { void foo(){...}
    *            concurrent<Hello>  h;
    *            h.( [](Hello& object){ object.foo(); };
    * @param func lambda that has to take the wrapped object by reference as argument
    *        the lambda will be called by the wrapper for the given lambda
    * @return std::future return of the lambda
    */
   template<typename F> // typename std::result_of< decltype(func)(T*, Args...)>::type
   auto operator()(F func) const -> std::future<typename std::result_of<decltype(func)(T&)>::type> {
      typedef typename std::result_of < decltype(func)(T&)>::type result_type;
      auto p = std::make_shared < std::promise<result_type>>();
      auto future_result = p->get_future();

      if (empty()) {
         p->set_exception(std::make_exception_ptr(std::runtime_error("concurrent was cleared of background thread object")));
      }
      else {
         _q.push([ = ]{
            try {
               concurrent_helper::set_value(*p, func, *(_worker.get()));
            } catch (...) {
               p->set_exception(std::current_exception()); }
         });
      }
      return future_result;
   }
   
    
   /**
    * Following Kjell Hedström (KjellKod)'s approach for a concurrent wrapper in g3log
    * using std::packaged_task and and std::bind (since lambda currently cannot  
    * deal with expanding parameter packs in a lambda).
    * 
    * Example:   struct Hello { void foo(){...}
    *            concurrent<Hello>  h;
    *            h.call(&Hello::foo);
    * 
    * @param func function pointer to the wrapped object
    * @param args parameter pack to executed by the function pointer. 
    * @return std::future return of the background executed function
    */                                                          
   //typename decltype( std::declval<T>().func(Args...) )  
   // std::result_of<decltype(&func)(Args..)>::type  
   // typedef typename std::result_of< decltype( & T::toCPD ) ( T * ) >::type
   // typedef typename std::result_of< decltype(func)(T*)>::type
   template<typename AsyncCall, typename... Args>
   auto call(AsyncCall func, Args&&... args) const ->   std::future<typename std::result_of< decltype(func)(T*, Args...)>::type>  {
      typedef typename std::result_of <decltype(func)(T*, Args...)>::type result_type;      
      typedef std::packaged_task<result_type()> task_type;

      if (empty()) {
         auto p = std::make_shared<std::promise<result_type>>();
         std::future<result_type> future_result = p->get_future();
         p->set_exception(std::make_exception_ptr(std::runtime_error("concurrent was cleared of background thread object")));
         return future_result;
      }
    

      // weak compiler support for expanding parameter pack in a lambda. std::function is the work-around
      //auto bgCall = [&, args...]{ return (_worker.*func)(args...); }; 
      auto bgCall = std::bind(func, _worker.get(), std::forward<Args>(args)...);
      task_type task(std::move(bgCall));
      std::future<result_type> result = task.get_future();
      _q.push(MoveOnCopy<task_type>(std::move(task)));
      return std::move(result);
   }
};

