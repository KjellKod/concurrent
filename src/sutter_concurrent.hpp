// PUBLIC DOMAIN LICENSE: https://github.com/KjellKod/Concurrent/blob/master/LICENSE
// 
// From a Herb Sutter Channel 9 presentation 
//        http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism
// Published at:
//      https://github.com/KjellKod/Concurrent and 
//      http://kjellkod.wordpress.com/2014/04/07/concurrency-concurrent-wrapper/
// 
// What "concurrent" is
// 1) The "concurrent" can wrap ANY object
// 2) All access to the concurrent is done though a lambda. 
// 3) All access call to the concurrent is done asynchronously but they are executed in FIFO order
// 4) At scope exit all queued jobs has to finish before the concurrent goes out of scope

#pragma once
#include <shared_queue.hpp>   
#include <thread>
#include <future>
#include <functional>

namespace sutter {
namespace concurrent_helper {
   typedef std::function<void() > Callback;
   template<typename Fut, typename F, typename T>
   void set_value(std::promise<Fut>& p, F& f, T& t) {
      p.set_value(f(t));
   }

   // helper for setting promise/exception for promise of void

   template<typename F, typename T>
   void set_value(std::promise<void>& p, F& f, T& t) {
      f(t);
      p.set_value();
   }
} // namespace concurrent


/// Basically a light weight active object. www.kjellkod.cc/active-object-with-cpp0x#TOC-Active-Object-the-C-11-way
/// all input happens in the background. At shutdown it exits only after all 
/// queued requests are handled.
template <class T> class concurrent {
   mutable T _worker;
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
   template<typename ... Args>
   concurrent(Args&&... args)
   : _worker(std::forward<Args>(args)...)
   , _done(false)
   , _thd([ = ]{concurrent_helper::Callback call;
      while (!_done) {

         _q.wait_and_pop(call); call();
      }}) {
   }

   virtual ~concurrent() {
      _q.push([ = ]{_done = true;});
      _thd.join();
   }


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
   template<typename F>
   auto operator()(F f) const -> std::future<decltype(f(_worker))> {
      auto p = std::make_shared < std::promise < decltype(f(_worker)) >> ();
      auto future_result = p->get_future();
      _q.push([ = ]{
         try {
            concurrent_helper::set_value(*p, f, _worker);
         } catch (...) {
            p->set_exception(std::current_exception()); }
      });
      return future_result;
   }
};

} // sutter
