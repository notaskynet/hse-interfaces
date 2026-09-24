// Лабораторная 1, задания 2-3
// PIC18F4520, плата UNI DS-6, mikroC PRO for PIC.

// Частота тактового генератора платы, Гц
#define FOSC 10000000UL
// Скорость обмена, бод
#define BAUD 19200UL // TODO: 19200 или 57600, уточнить

// Режим BRG16 = 1, BRGH = 1: скорость = FOSC / [4 (n + 1)].
// Деление с округлением до ближайшего целого.
//   10 МГц, 19200 бод: n = 129, реальная скорость 19230,8 (+0,16 %)
//   10 МГц, 57600 бод: n = 42,  реальная скорость 58139,5 (+0,94 %)
#define BRG_VALUE ((FOSC + 2 * BAUD) / (4 * BAUD) - 1)

// Уровень на выводе при нажатой кнопке: 
// 1 - кнопки подтянуты к VCC при нажатии,
// 0 - к GND.
// Зависит от положения перемычки выбора уровня кнопок на плате.
#define BTN_ACTIVE_HIGH 1
#define BTN_MASK 0x3F

// Приём по прерыванию: байт из RCREG сразу выводится на светодиоды.
void interrupt() {
    if (RCIE_bit && RCIF_bit) {
        LATB = RCREG;           // чтение RCREG сбрасывает RCIF и FERR
        if (OERR_bit) {         // переполнение: приёмник остановлен до сброса CREN
            CREN_bit = 0;
            CREN_bit = 1;
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

void uart_write(unsigned char byte) {
    while (!TXIF_bit)           // ждём, пока освободится TXREG
        ;
    TXREG = byte;
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
            for (i = 0; i < 6; i++) {
                if (pressed & (1 << i))
                    uart_write('0' + i);
            }
            prev = cur;
        }
    }
}
