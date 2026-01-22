#pragma once

#include <stdint.h>

#define EOK             0       // success
#define EIO             1       // I/O error
#define EAGAIN          2       // Try again
#define ENODEV          3       // No such device
#define EINVAL          4       // Invalid argument
#define ENOSUP          5       // Not supported
#define ENOSYS          6       // Function not implemented
#define ETIMEOUT        7       // timeout reached
#define EBADRESP        8       // bad response
