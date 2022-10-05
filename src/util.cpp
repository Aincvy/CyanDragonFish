#include "log.h"

#include "util.h"

#include <absl/time/clock.h>
#include <absl/time/time.h>

namespace cdf {

    long currentTime() {
        return absl::ToUnixMillis(absl::Now());
    }
    
}

