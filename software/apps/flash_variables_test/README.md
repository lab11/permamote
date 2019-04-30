`flash_storage` is a library that acts as a wrapper for Nordic's `fds` library. It enables the assignment of variables in the main flow of a program on Permamote while simultaneously storing those variables' values in non-volatile memory. When Permamote is reset and the assignment statements are executed again, their results can change if the variables in non-volatile memory have been updated in the interim.

An example of `flash_storage` usage is below. Here, `flash_storage` functions are used to initialize an integer variable, update the value of that variable in non-volatile memory, and then re-initialize that variable with its new value.

First, this block of code is executed on Permamote:

```c
flash_storage_init();
uint16_t x_id = 0x1111; // arbitrary ID
int x = define_flash_variable_int(-1017, x_id);
printf("%d\n", x); // Prints -1017
```

Then, this line of code is executed on Permamote:

```c
flash_update_int(x_id, 1206);
```

Finally, Permamote is reset, and the first block of code runs again.

```c
int x = define_flash_variable_int(-1017, x_id);
printf("%d\n", x); // Prints 1206 instead of -1017
```
