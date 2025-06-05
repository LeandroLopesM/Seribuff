# SeRizzler

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

## Macros
### Defauly memory macro
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
(this'll probably be gone in some time, just can't be bothered to remove it yet (I'd rather update it endlessly whenever i add something new to the API))
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
