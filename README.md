fifo
================

A simple ringbuffer template implementation in C++ that
can utilize local or shared memory.  

# USE
Compile with -std=c++11 flag, there's an example threaded
app in main.cpp.  One limitation of the SHM version is that
the buffer must be statically sized.  The locally allocated
version can be allocated on the fly.  

# Default
By defualt this will build a library. The header file that should be included
when using the FIFO is simply fifo.hpp. It can be instantiated on SHM or heap. 
The size of each FIFO is static. It is also lock free. 



# Example

```cpp


using ringb_t =  RingBuffer< std::int64_t          /* buffer type */,
                             Type::Heap            /* allocation type */
                           >  TheBuffer;
static void func_a( ringb_t &buffer, bool &term )
{
  int i = 0;
  while( ! term )
  {
    buffer.push( i++ );
  }
  return;
}

static void func_b( ringb_t &buffer, bool &term )
{
  while( ! term )
  {
    int value; 
    buffer.push( value );
    std::cout << value << "\n";
  }
  return;
}
ringb_t buffer( 100 /** size **/ );

bool term( false );

std::thread a( func_a, std::ref( buffer ), std::ref( term ) );
std::thread b( func_b, std::ref( buffer ), std::ref( term ) );

term = true;

a.join();
b.join();

```

# TODO
* Add TCP connected ringbuffer implementation.
* Add Java implementation that can use the C/C++ allocated SHM with at least primitive types.
* Add write and read optimizations.
