#pragma once

#include "WebUi.h"

class WifiWebUi : public WebUi {
 public:
  using WebUi::WebUi;
  void begin() override;

 protected:
  void handleConfig() override;
};
