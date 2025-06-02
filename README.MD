# SeRizzler

Serialization library, doesn't currently support disk etching (writing memory structure to disk), though it should by fairly trivial (Its just a matter of shipping the files with memory length and other metadata).
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
Alternatively, you can forego such things by doing:
```
INFER_PREFIX(my_mem_var)
my_mem_var_push()
my_mem_var_get()
```
OR, in case you want to select the prefix:
```
INFER_ARG(main, my_mem_var)
main_push()
main_get()
```

# Have fyn!
