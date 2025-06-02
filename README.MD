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
#### You can forego passing the memory as an argument with these alternatives:
##### Defauly memory macro
```c
MEMORY(m) // automatically creates the memory variable, and infers prefixes
m_push()
m_get()
```
##### Prefix inference:
```c
INFER_PREFIX(my_mem_var)
my_mem_var_push()
my_mem_var_get()
```
##### Argument inference
```c
INFER_ARG(main, my_mem_var)
main_push()
main_get()
```


# Have fyn!
