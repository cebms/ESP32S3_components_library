# ESP32S3_components_library
> Library made based on the original ESP_WHO library to add new functionalities to the board's components 

The "lib" folder shoud replace the original "components" folder of your ESP-WHO project in order to obtain access to the functions being developed here.
Also, make sure to replace your original CMakeLists.txt file for the one in this repository. The only reason for that is the line:
```
  set(EXTRA_COMPONENT_DIRS ./lib)
```
The line above changes the directory which the compiler should use when looking for the components source codes and headers
