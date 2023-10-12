#ifndef __MAIN_H
#define __MAIN_H

#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>

#define EOL '\r'
#define CRLF "\r\n"

#define COMMAND_MAX_LENGTH 15


#define UNKNOWN_COMMAND "unknown command"
#define COMMAND_TOO_LONG "command too long"

#endif
