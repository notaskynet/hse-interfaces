// Лабораторная 2
// PIC18F4520, плата UNI DS-6, mikroC PRO for PIC.
//
// Читает канал АЦП MCP3204 по SPI и отправляет код в UART
// десятичным числом в отдельной строке: "2048\r\n".

// Частота тактового генератора платы, Гц
#define FOSC 8000000UL
// Скорость обмена, бод
#define BAUD 19200UL // TODO: 19200 или 57600, уточнить

// Режим BRG16 = 1, BRGH = 1: скорость = FOSC / [4 (n + 1)].
// Деление с округлением до ближайшего целого.
//   8 МГц, 19200 бод: n = 103, реальная скорость 19230,8 (+0,16 %)
//   8 МГц, 57600 бод: n = 34,  реальная скорость 57142,9 (-0,79 %)
#define BRG_VALUE ((FOSC + 2 * BAUD) / (4 * BAUD) - 1)

// Вывод выбора АЦП (линия #CS1# платы, SW13.7). ПРОВЕРИТЬ по схеме MCU-карты,
// на какой вывод контроллера она приходит, и поправить оба определения.
#define ADC_CS      LATC0_bit
#define ADC_CS_TRIS TRISC0_bit

// Канал АЦП: 0-3 (CH0 и CH1 - клеммник CN15, CH2 и CH3 - CN16).
#define ADC_CHANNEL 0

void uart_init(void) {
    TRISC6_bit = 0;             // TX - выход
    TRISC7_bit = 1;             // RX - вход

    BAUDCON = 0x08;             // BRG16 = 1
    SPBRGH = (BRG_VALUE >> 8) & 0xFF;
    SPBRG = BRG_VALUE & 0xFF;

    TXSTA = 0x24;               // TX9 = 0, TXEN = 1, SYNC = 0, BRGH = 1
    RCSTA = 0x80;               // SPEN = 1, приёмник не нужен
}

void uart_write(unsigned char byte) {
    while (!TXIF_bit)           // ждём, пока освободится TXREG
        ;
    TXREG = byte;
}

// Десятичное число без ведущих нулей и "\r\n".
void uart_write_value(unsigned int value) {
    unsigned char digits[5];
    unsigned char count = 0;

    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value);

    while (count)
        uart_write(digits[--count]);
    uart_write('\r');
    uart_write('\n');
}

void spi_init(void) {
    ADC_CS = 1;                 // АЦП не выбран
    ADC_CS_TRIS = 0;

    TRISC3_bit = 0;             // SCK - выход
    TRISC4_bit = 1;             // SDI - вход
    TRISC5_bit = 0;             // SDO - выход

    // Режим SPI 0,0: SCK в покое 0, данные защёлкиваются по нарастающему фронту.
    SSPSTAT = 0x40;             // SMP = 0, CKE = 1
    SSPCON1 = 0x21;             // SSPEN = 1, CKP = 0, Master, SCK = FOSC/16 (500 кГц при 8 МГц)
}

// Обмен одним байтом. Передача и приём идут одновременно.
unsigned char spi_transfer(unsigned char byte) {
    SSPBUF = byte;
    while (!BF_bit)             // ждём конца обмена
        ;
    return SSPBUF;              // чтение SSPBUF сбрасывает BF
}

// Одно измерение, несимметричный вход. Результат 0-4095 (1 единица = 1 мВ при опоре 4,096 В).
// Посылка из трёх байт: 0000 0 1 SGL D2 | D1 D0 xx xxxx | xxxx xxxx.
unsigned int adc_read(unsigned char channel) {
    unsigned char high, low;

    ADC_CS = 0;
    spi_transfer(0x06);                     // старт-бит, SGL/DIFF = 1, D2 = 0
    high = spi_transfer(channel << 6);      // D1, D0; в ответ: ? ? ? 0 B11 B10 B9 B8
    low = spi_transfer(0x00);               // в ответ: B7-B0
    ADC_CS = 1;                             // по фронту CS АЦП сбрасывает счётчик бит

    return ((unsigned int)(high & 0x0F) << 8) | low;
}

// Программная задержка между измерениями, порядка 20 мс при 8 МГц.
void sample_delay(void) {
    unsigned int i;
    for (i = 0; i < 4000; i++)
        ;
}

void main() {
    ADCON1 = 0x0F;              // все выводы AN - цифровые
    CMCON = 0x07;               // компараторы выключены

    uart_init();
    spi_init();

    while (1) {
        uart_write_value(adc_read(ADC_CHANNEL));
        sample_delay();
    }
}
