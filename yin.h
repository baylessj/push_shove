#pragma once

namespace yin {
struct result {
  size_t period;
  float conf;
};

void write(float in);

result read();

result estimate();
} // namespace yin