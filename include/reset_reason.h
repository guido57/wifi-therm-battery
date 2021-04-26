#include <Arduino.h>
#include <rom/rtc.h>

/*
USAGE:

Serial.println("======== CPU0 reset reason: ==============");
  print_reset_reason(rtc_get_reset_reason(0));
  verbose_print_reset_reason(rtc_get_reset_reason(0));
  Serial.println("======== CPU1 reset reason: ==============");
  print_reset_reason(rtc_get_reset_reason(1));
  verbose_print_reset_reason(rtc_get_reset_reason(1));
  Serial.println("=========================================");

*/
class ResetReasons {

  public:
    static String get_reset_reason_str(RESET_REASON reason);
    static void print_reset_reason(RESET_REASON reason);
    static String get_verbose_reset_reason_str(RESET_REASON reason);
    static void verbose_print_reset_reason(RESET_REASON reason);
};
