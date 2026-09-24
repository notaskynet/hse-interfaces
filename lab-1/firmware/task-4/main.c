// Лабораторная 1, задание 4.
// PIC18F4520, плата UNI DS-6, mikroC PRO for PIC.
//
// Кнопки RA0-RA5 -> нуль-терминированные строки в UART (передача по прерываниям).
// Принятый из UART байт -> светодиоды RB0-RB7 (приём по прерываниям).
//
// Библиотечные функции не используются, работа идёт напрямую с регистрами EUSART.

// Частота тактового генератора платы, Гц. Проверить по кварцу на плате.
#define FOSC 10000000UL
// Скорость обмена, бод
#define BAUD 19200UL // TODO: 19200 или 57600, уточнить

// Режим BRG16 = 1, BRGH = 1: скорость = FOSC / [4 (n + 1)].
// Деление с округлением до ближайшего целого.
//   10 МГц, 19200 бод: n = 129, реальная скорость 19230,8 (+0,16 %)
//   10 МГц, 57600 бод: n = 42,  реальная скорость 58139,5 (+0,94 %)
#define BRG_VALUE ((FOSC + 2 * BAUD) / (4 * BAUD) - 1)

// Уровень на выводе при нажатой кнопке: 1 - кнопки подтянуты к VCC при нажатии,
// 0 - к GND. Зависит от положения перемычки выбора уровня кнопок на плате.
#define BTN_ACTIVE_HIGH 1
#define BTN_MASK 0x3F

#define MSG_COUNT 6
#define MSG_SIZE 20

// Строки для кнопок RA0-RA5, каждая не короче 7 символов.
const char messages[MSG_COUNT][MSG_SIZE] = {
    "Button RA0\r\n",
    "Button RA1\r\n",
    "Button RA2\r\n",
    "Button RA3\r\n",
    "Button RA4\r\n",
    "Button RA5\r\n"
};

const char *tx_ptr;                 // следующий байт передаваемой строки
volatile unsigned char tx_busy = 0; // 1 - идёт передача строки

void interrupt() {
    // Приём: байт из RCREG сразу выводится на светодиоды.
    if (RCIE_bit && RCIF_bit) {
        LATB = RCREG;           // чтение RCREG сбрасывает RCIF и FERR
        if (OERR_bit) {         // переполнение: приёмник остановлен до сброса CREN
            CREN_bit = 0;
            CREN_bit = 1;
        }
    }

    // Передача: TXIF = 1, когда TXREG пуст. Флаг нельзя сбросить программно,
    // поэтому в конце строки прерывание запрещается через TXIE.
    if (TXIE_bit && TXIF_bit) {
        if (*tx_ptr) {
            TXREG = *tx_ptr;
            tx_ptr++;
        } else {
            TXIE_bit = 0;
            tx_busy = 0;
        }
    }
}

void uart_init(void) {
    TRISC6_bit = 0;             // TX - выход
    TRISC7_bit = 1;             // RX - вход

    BAUDCON = 0x08;             // BRG16 = 1
    SPBRGH = (BRG_VALUE >> 8) & 0xFF;
    SPBRG = BRG_VALUE & 0xFF;

    TXSTA = 0x24;               // TX9 = 0, TXEN = 1, SYNC = 0, BRGH = 1
    RCSTA = 0x90;               // SPEN = 1, RX9 = 0, CREN = 1
    // Кадр: 8 бит данных, 1 стоп-бит (в EUSART стоп-бит всегда один).
}

// Запуск передачи строки. Возвращает 0, если предыдущая строка ещё передаётся.
unsigned char uart_send_string(const char *str) {
    if (tx_busy)
        return 0;
    tx_ptr = str;
    tx_busy = 1;
    TXIE_bit = 1;               // TXIF уже равен 1, первое прерывание сработает сразу
    return 1;
}

// Программная задержка для антидребезга, порядка 20 мс при 10 МГц.
void debounce_delay(void) {
    unsigned int i;
    for (i = 0; i < 5000; i++)
        ;
}

// Состояние кнопок RA0-RA5: 1 в разряде - кнопка нажата.
unsigned char read_buttons(void) {
    unsigned char state = PORTA & BTN_MASK;
#if !BTN_ACTIVE_HIGH
    state ^= BTN_MASK;
#endif
    return state;
}

void main() {
    unsigned char prev = 0;
    unsigned char cur, pressed, i;

    ADCON1 = 0x0F;              // все выводы AN - цифровые
    CMCON = 0x07;               // компараторы выключены
    TRISA |= BTN_MASK;          // RA0-RA5 - входы
    LATB = 0;
    TRISB = 0;                  // RB0-RB7 - выходы

    uart_init();

    RCIE_bit = 1;               // прерывание по приёму
    PEIE_bit = 1;               // периферийные прерывания
    GIE_bit = 1;                // глобальное разрешение

    while (1) {
        cur = read_buttons();
        if (cur != prev) {
            debounce_delay();
            if (read_buttons() != cur)
                continue;       // дребезг, состояние ещё не установилось

            pressed = cur & ~prev;          // только что нажатые кнопки
            for (i = 0; i < MSG_COUNT; i++) {
                if (pressed & (1 << i)) {
                    while (tx_busy)         // дождаться конца предыдущей строки
                        ;
                    uart_send_string(messages[i]);
                }
            }
            prev = cur;
        }
    }
}
