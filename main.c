#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

typedef enum cor { VERMELHO, AZUL, VERDE } cor;

const uint pino_botao = 5;
const uint pino_led_vermelho = 11;
const uint pino_led_azul = 12;
const uint pino_led_verde = 13;

absolute_time_t turn_off_time;

static volatile uint32_t ultimo_tempo = 0; // variavel para debouce

bool ligar_todos_leds = false;  
bool desativar_leds = false;
static cor cor_atual_para_apagar = VERMELHO;

bool repeating_timer_callback(struct repeating_timer *t);
void gpio_irq_handler(uint gpio, uint32_t events);

int main()
{
    stdio_init_all();

    gpio_init(pino_botao);
    gpio_set_dir(pino_botao, GPIO_IN);
    gpio_pull_up(pino_botao);

    gpio_init(pino_led_vermelho);
    gpio_set_dir(pino_led_vermelho, GPIO_OUT);
    
    gpio_init(pino_led_azul);
    gpio_set_dir(pino_led_azul, GPIO_OUT);

    gpio_init(pino_led_verde);
    gpio_set_dir(pino_led_verde, GPIO_OUT);
    
    gpio_put_all(false);


    struct repeating_timer timer;
    add_repeating_timer_ms(3000, repeating_timer_callback, NULL, &timer);



    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {


    // fazendo o debouce
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    bool debouce = tempo_atual - ultimo_tempo > 200000;

    if (!debouce) { // caso nao tenha passado o intervalo ideal entre pressionar os botoes
        return;
    }

    ultimo_tempo = tempo_atual; // atualizando o tempo


    if (desativar_leds == false) {

        gpio_put(pino_led_vermelho, true);
        gpio_put(pino_led_azul, true);
        gpio_put(pino_led_verde, true);
        desativar_leds = true;
        turn_off_time = make_timeout_time_ms(3000);
    }

}


bool repeating_timer_callback(struct repeating_timer *t) {
    if ((!desativar_leds) && absolute_time_diff_us(get_absolute_time(), turn_off_time)) {return true;}

    switch (cor_atual_para_apagar) {
        case VERMELHO:
            gpio_put(pino_led_vermelho, false);
            cor_atual_para_apagar = AZUL;
            turn_off_time = make_timeout_time_ms(3000);
            break;
        case AZUL:
            gpio_put(pino_led_azul, false);
            cor_atual_para_apagar = VERDE;
            turn_off_time = make_timeout_time_ms(3000);
            break;
        case VERDE:
            gpio_put(pino_led_verde, false);
            cor_atual_para_apagar = VERMELHO;
            desativar_leds = false;
            break;
    }

    return true;
}