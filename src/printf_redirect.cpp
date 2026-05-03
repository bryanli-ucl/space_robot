#include <Arduino.h>
#include <platform/mbed_retarget.h>

namespace mbed {

FileHandle* mbed_override_console(int fd) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        return static_cast<FileHandle*>(Serial3);
    }

    return nullptr;
}

} // namespace mbed
