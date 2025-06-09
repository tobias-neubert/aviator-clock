#pragma once

#include <iostream> // For printing to console in the mock

class MockSerial
{
public:
    void begin(unsigned long baud)
    {
        std::cout << "MockSerial::begin(" << baud << ") called" << std::endl;
    }

    size_t print(const char *s)
    {
        std::cout << s;
        return strlen(s);
    }

    size_t println(const char *s)
    {
        std::cout << s << std::endl;
        return strlen(s);
    }

    // Add other Serial methods your code uses (write, read, available, etc.)

    // Mock operator! to allow 'if (!Serial)'
    operator bool() const
    {
        return true; // Or false, depending on desired mock behavior for 'if (!Serial)'
                     // 'if (!Serial)' is true if Serial is NOT ready.
                     // So, if you want to skip blocks guarded by !Serial during test, return true.
                     // If you want to enter them, return false.
                     // Returning true is typical for a simple mock.
    }
};

// Declare a global instance that your code can see
extern MockSerial Serial;

