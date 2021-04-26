#include <reset_reason.h>

String ResetReasons::get_reset_reason_str(RESET_REASON reason)
{
  String str_reason = "";
  switch (reason)
  {
  case 1:
    str_reason = ("POWERON_RESET");
    break; /**<1,  Vbat power on reset*/
  case 3:
    str_reason = ("SW_RESET");
    break; /**<3,  Software reset digital core*/
  case 4:
    str_reason = ("OWDT_RESET");
    break; /**<4,  Legacy watch dog reset digital core*/
  case 5:
    str_reason = ("DEEPSLEEP_RESET");
    break; /**<5,  Deep Sleep reset digital core*/
  case 6:
    str_reason = ("SDIO_RESET");
    break; /**<6,  Reset by SLC module, reset digital core*/
  case 7:
    str_reason = ("TG0WDT_SYS_RESET");
    break; /**<7,  Timer Group0 Watch dog reset digital core*/
  case 8:
    str_reason = ("TG1WDT_SYS_RESET");
    break; /**<8,  Timer Group1 Watch dog reset digital core*/
  case 9:
    str_reason = ("RTCWDT_SYS_RESET");
    break; /**<9,  RTC Watch dog Reset digital core*/
  case 10:
    str_reason = ("INTRUSION_RESET");
    break; /**<10, Instrusion tested to reset CPU*/
  case 11:
    str_reason = ("TGWDT_CPU_RESET");
    break; /**<11, Time Group reset CPU*/
  case 12:
    str_reason = ("SW_CPU_RESET");
    break; /**<12, Software reset CPU*/
  case 13:
    str_reason = ("RTCWDT_CPU_RESET");
    break; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    str_reason = ("EXT_CPU_RESET");
    break; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    str_reason = ("RTCWDT_BROWN_OUT_RESET");
    break; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    str_reason = ("RTCWDT_RTC_RESET");
    break; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    str_reason = ("NO_MEAN");
  }
  return str_reason;
}

void ResetReasons::print_reset_reason(RESET_REASON reason)
{
  Serial.println(get_reset_reason_str(reason));
}

String ResetReasons::get_verbose_reset_reason_str(RESET_REASON reason)
{
  String str_reason = "";
  switch (reason)
  {
  case 1:
    str_reason = ("Vbat power on reset");
    break;
  case 3:
    str_reason = ("Software reset digital core");
    break;
  case 4:
    str_reason = ("Legacy watch dog reset digital core");
    break;
  case 5:
    str_reason = ("Deep Sleep reset digital core");
    break;
  case 6:
    str_reason = ("Reset by SLC module, reset digital core");
    break;
  case 7:
    str_reason = ("Timer Group0 Watch dog reset digital core");
    break;
  case 8:
    str_reason = ("Timer Group1 Watch dog reset digital core");
    break;
  case 9:
    str_reason = ("RTC Watch dog Reset digital core");
    break;
  case 10:
    str_reason = ("Instrusion tested to reset CPU");
    break;
  case 11:
    str_reason = ("Time Group reset CPU");
    break;
  case 12:
    str_reason = ("Software reset CPU");
    break;
  case 13:
    str_reason = ("RTC Watch dog Reset CPU");
    break;
  case 14:
    str_reason = ("for APP CPU, reseted by PRO CPU");
    break;
  case 15:
    str_reason = ("Reset when the vdd voltage is not stable");
    break;
  case 16:
    str_reason = ("RTC Watch dog reset digital core and rtc module");
    break;
  default:
    str_reason = ("NO_MEAN");
  }
  return (str_reason);
}

void ResetReasons::verbose_print_reset_reason(RESET_REASON reason)
{
  Serial.println(get_verbose_reset_reason_str(reason));
}
