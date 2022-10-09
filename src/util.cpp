#include "log.h"

#include "util.h"

#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <iterator>
#include <string>

#include "absl/random/random.h"

namespace cdf {

    long currentTime() {
        return absl::ToUnixMillis(absl::Now());
    }

    std::string randomString(int length) {
        std::string s;
        static const char alphaNum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        absl::BitGen bitGen;

        for (int i = 0; i < length; i++) {
            size_t index = absl::Uniform(bitGen, 0u, std::size(alphaNum));
            s += alphaNum[index];
        }
        
        return s;
    }
    
}

