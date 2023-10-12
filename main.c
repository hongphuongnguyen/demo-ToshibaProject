#include "main.h"
#include "tx_api.h"

#define STACK_SIZE 1024

/* Dinh nghia 2 luong Thread */
TX_THREAD Uart;
TX_THREAD Handle;

/* Mang kieu unsigned char co kich thuoc STACK_SIZE sd de luu tru ngan xep cho 2 luong */
UCHAR uart_stack[STACK_SIZE];
UCHAR handle_stack[STACK_SIZE];

/* handle_ready de xem trang thai cua luong handle, khi handle_ready = 1 co nghia la luong Handle 
 * da san sang de nhan 1 lenh moi tu UART */
int handle_ready = 0;
char usart_buf[COMMAND_MAX_LENGTH];

/* Khoi tao cac cong GPIO de dieu khien Led */
void Init_Output(void) {
    int i;
    GPIO_InitTypeDef GPIO_Settings;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    for (i = 0; i < 7; i++) {
        GPIO_Settings.GPIO_Pin = 1 << i;
        GPIO_Settings.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Settings.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &GPIO_Settings);
    }

    GPIO_Settings.GPIO_Pin = GPIO_Pin_13;
    GPIO_Settings.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Settings.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_Settings);
}

void Turn_On_Led(int pos) {
    GPIOA->ODR |= 1 << pos;
}

void Turn_Off_Led(int pos) {
    GPIOA->ODR &= ~(1 << pos);
}

void Usart_Init() {
    GPIO_InitTypeDef GPIO_Settings;
    USART_InitTypeDef USART_Settings;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_StructInit(&GPIO_Settings);
    GPIO_Settings.GPIO_Pin = GPIO_Pin_9;
    GPIO_Settings.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Settings.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_Settings);

    GPIO_Settings.GPIO_Pin = GPIO_Pin_10;
    GPIO_Settings.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Settings.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_Settings);

    USART_StructInit(&USART_Settings);
    USART_Settings.USART_BaudRate = 9600;
    USART_Settings.USART_WordLength = USART_WordLength_8b;
    USART_Settings.USART_StopBits = USART_StopBits_1;
    USART_Settings.USART_Parity = USART_Parity_No;
    USART_Settings.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Settings.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_Settings);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void Usart_Send(const char chr) {
    while (!(USART1->SR & USART_SR_TC)); /* Wait status register and Transmit Complete is set */
    USART1->DR = chr;
}

void Usart_Send_String(const char *s) {
    int i = 0;
    while (s[i]) {
        Usart_Send(s[i++]);
    }
}

void Usart_Send_Newline(void) {
    Usart_Send_String(CRLF);
}

void Usart_Send_Line(const char *s) {
    Usart_Send_String(s);
    Usart_Send_String(CRLF);
}

int Get_String_Len(char str1[15]) {
    int j = 0;
    while (str1[j] != '\0') {
        j++;
    }
    return j;
}

int Compare_String(char str1[15], char str2[15], int num) {
    int k = 0;
    while ((str1[k] == str2[k]) && (k < num)) {
        k++;
    }
    if (k == num) return 0;
    else return 1;
}

int Compare_String_Reverse(char str1[15], char str2[15], int num) {
    int len = Get_String_Len(str1);
    int k = len - num;
    int l = 0;
    while ((str1[k] == str2[l]) && (k < len)) {
        k++;
        l++;
    }
    if (l == num) return 0;
    else return 1;
}

void Handle_Command(ULONG thread_input) {
    int pos, leng;
    while (1) {
        if (handle_ready == 1) {
            leng = Get_String_Len(usart_buf);
            if (leng == 9) {
                if ((Compare_String(usart_buf, "LED ", 4) == 0) && (Compare_String_Reverse(usart_buf, " ON", 3) == 0)) {
                    pos = (usart_buf[4] - '0') * 10 + (usart_buf[5] - '0');
                    if ((pos >= 0) && (pos <= 7)) {
                        Turn_On_Led(pos);
                        Usart_Send_String("Turn on LED ");
                        Usart_Send(usart_buf[4]);
                        Usart_Send(usart_buf[5]);
                        Usart_Send_Newline();
                    } else Usart_Send_Line(UNKNOWN_COMMAND);
                } else Usart_Send_Line(UNKNOWN_COMMAND);
            } else if (leng == 10) {
                if ((Compare_String(usart_buf, "LED ", 4) == 0) && (Compare_String_Reverse(usart_buf, " OFF", 4) == 0)) {
                    pos = (usart_buf[4] - '0') * 10 + (usart_buf[5] - '0');
                    if ((pos >= 0) && (pos <= 7)) {
                        Turn_Off_Led(pos);
                        Usart_Send_String("Turn off LED ");
                        Usart_Send(usart_buf[4]);
                        Usart_Send(usart_buf[5]);
                        Usart_Send_Newline();
                    } else Usart_Send_Line(UNKNOWN_COMMAND);
                } else Usart_Send_Line(UNKNOWN_COMMAND);
            } else Usart_Send_Line(UNKNOWN_COMMAND);
            handle_ready = 0;
            tx_thread_sleep(1);
        }
    }
}

void Receive_Commmand(ULONG thread_input) {
    unsigned short usart_buf_length = 0;
    unsigned char received;
    while (1) {
        if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
            received = USART_ReceiveData(USART1);
            if (usart_buf_length < COMMAND_MAX_LENGTH) {
                usart_buf[usart_buf_length++] = received;
                Usart_Send(received);
            }
            if (received == EOL) {
                usart_buf[usart_buf_length - 1] = '\0';
                Usart_Send_Newline();
                usart_buf_length = 0;
                handle_ready = 1;
            }
            if (usart_buf_length == COMMAND_MAX_LENGTH) {
                Usart_Send_Newline();
                Usart_Send_Line(COMMAND_TOO_LONG);
                usart_buf_length = 0;
            }
        }
        tx_thread_sleep(1);
    }
}

void tx_application_define(void *first_unused_memory) {
    tx_thread_create(&Uart, "ReceiveCMD", Receive_Commmand, 0, uart_stack, STACK_SIZE, 8, 8, 1, TX_AUTO_START);
    tx_thread_create(&Handle, "HandleCMD", Handle_Command, 0, handle_stack, STACK_SIZE, 16, 16, 1, TX_AUTO_START);
}

int main(void) {
    Init_Output();
    Usart_Init();
    tx_kernel_enter();
}
