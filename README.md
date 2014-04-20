concurrent\<T\> wrapper
=================================

Makes access to *any* object asynchronous. All asynchronous calls will be executed in the background and in FIFO order.

The concurrent\<T\> wrapper can be called in two ways. 

**1** As suggested by [Herb Sutter in C++ and Beyond 2012] (http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism)

* Many actions can be bundled together in one asynchronous operation
* All calls are made through a lambda call that takes a reference to the wrapped object as input argument
* See example from the unit tests:
```cpp
class Greetings {
 std::string _msg;
 public:
  explicit Greetings(const std::string& msg) : _msg(msg){}
  std::string Hello(size_t number) { 
     return {_msg + " " + std::to_string(number)};
  }
  
  concurrent<Greetings> greeting{"Hello World"};
  // execute two Hello calls in one asynchronous operation. 
  std::future<std::string> response = greeting.lambda( 
      [](Greetings& g) { 
         std::string reply{g.Hello(123) + " " + g.Hello(456)}; 
         return reply;
       }
     ); // Hello World 123 Hello World 456
  EXPECT_EQ(response.get(), "Hello World 123 Hello World 456");
  
  
```

**2** As used in the *asynchronous, "crash-safe" logger, [G3Log](https://bitbucket.org/KjellKod/g3log)* , using a function-pointer syntax. 
* This is a safer approch, it's protected from unintentianal abuse. Compare this to the lambda approach where the lambda capture is powerful but can cause unintended problems if the lambda capturing ia used irresponsibly. 
* Only  **one** action can be done per each asynchronous request.
* See example from unit tests:
```cpp
   concurrent<Greetings> greeting2{"Hello World"};
   // execute ONE Hello in one asynchronous operation. 
   std::future<std::string> response2 = greeting.call(&Greetings::Hello, 789); 
   EXPECT_EQ(response2.get(), "Hello World 789");
```


**3** Both can be used for derived objects of abstract types
* See example from unit tests:
```cpp
   struct Animal {
      virtual std::string sound() = 0;
   };
  
   struct Dog : public Animal {
      std::string sound() override {
         return  {"Wof Wof"};
      }
   };

   struct Cat : public Animal {
      std::string sound() override {
         return {"Miauu Miauu"};
      }
   };
   
   
 
  // Internally the concurrent<T> wrapper keeps the object in a std::unique_ptr<T>  
  concurrent<Animal> animal1{std::unique_ptr<Animal>(new Dog)};  
  concurrent<Animal> animal2{std::unique_ptr<Animal>(new Cat)};

  // lambda syntax
  auto make_sound = [](Animal& animal) { return animal.sound();  };
  EXPECT_EQ("Wof Wof", animal1.lambda(make_sound).get());

  // function-call syntax
  EXPECT_EQ("Miauu Miauu", animal2.call(&Animal::sound).get());

  ```
