# SeriBuff
### sér-í-bûff

(Sorta) single-header serialization library in pure C
    |> utils.h can make your life easier, but that's up to you :3
## Usage
```c
memory mem;

struct {
    ...
} MyStruct;

int main() {
    struct MyStruct my_var = {...};
    int my_var_handle = push(mem, my_var, sizeof(MyStruct));
    // ...
    struct MyStruct another_var = get(mem, my_var_handle);
}
```
## API
### `int push(memory* mem, const void* src, size_t srcsize)`
Pushes `srcsize` bytes of `src` into `mem`; Returns a handle so you can fetch it again in the future.
### `void* get(memory* mem, int h)`
Returns a `void*` to the object with handle `h` in `mem`; Returns NULL if object doesn't exist in `mem`.
### `void read(memory* mem, const char* path)`
Reads file in `path` to mem. Doesn't support anything that wasn't created with `write()`, likely to suffer fatal problems if the file was written by a program that uses a different `MEM_SIZE`.
### `void write(memory* mem, const char* path)`
Dumps `mem` to `path`. In the future i plan to implement hash checking and maybe even some encryption.
### `int size(memory* mem)`
I'm sure you can figure this one out :) (returns the stored item count);
## Macros
### Default memory macro
```c
MEMORY(m) // automatically creates the memory variable and infers prefixes
m_push()
m_get()
```
### Prefix inference:
```c
INFER_PREFIX(my_mem_var)
my_mem_var_push()
my_mem_var_get()
```
### Argument inference
(this'll probably be gone in some time, just can't be bothered to remove it yet (I'd rather update it endlessly whenever i add something new to the API!!!))
```c
INFER_ARG(main, my_mem_var)
main_push()
main_get()
```
## Phony makefile targets
### example
Self explanatory, builds all examples into exalpes/bin
### run
Runs all examples
### cleanex
Cleans the exaple binary folder

# Have fyn!
