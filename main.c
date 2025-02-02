#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

typedef enum cor { VERMELHO, AZUL, VERDE } cor;

const uint pino_botao = 5;
const uint pino_led_vermelho = 13;
const uint pino_led_azul = 12;
const uint pino_led_verde = 11;

absolute_time_t turn_off_time;

static volatile uint32_t ultimo_tempo = 0; // variável para debounce

bool desativar_leds = false;
static cor cor_atual_para_apagar = VERMELHO;

bool repeating_timer_callback(struct repeating_timer *t);
void gpio_irq_handler(uint gpio, uint32_t events);

int main() {
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
    
    // Garantindo que os LEDs comecem apagados
    gpio_put(pino_led_vermelho, false);
    gpio_put(pino_led_azul, false);
    gpio_put(pino_led_verde, false);

    gpio_set_irq_enabled_with_callback(pino_botao, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    struct repeating_timer timer;
    add_repeating_timer_ms(3000, repeating_timer_callback, NULL, &timer);

    turn_off_time = make_timeout_time_ms(0); // Inicializando tempo de desligamento

    while (true) {
        tight_loop_contents();
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    // Implementação do debounce
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    bool debounce = (tempo_atual - ultimo_tempo) > (200 * 1000); // 200ms debounce

    if (!debounce) { // Ignora se debounce não passou
        return;
    }

    ultimo_tempo = tempo_atual; // Atualizando o tempo de última ativação do botão

    if (!desativar_leds) {
        gpio_put(pino_led_vermelho, true);
        gpio_put(pino_led_azul, true);
        gpio_put(pino_led_verde, true);
        desativar_leds = true;
        turn_off_time = make_timeout_time_ms(3000); // Inicia contagem para desligar LEDs
    }
}

bool repeating_timer_callback(struct repeating_timer *t) {
    // Se os LEDs já estiverem desligados, não faz nada
    if (!desativar_leds || (absolute_time_diff_us(get_absolute_time(), turn_off_time) > 0)) {
        return true;
    }

    switch (cor_atual_para_apagar) {
        case VERMELHO:
            gpio_put(pino_led_vermelho, false);
            cor_atual_para_apagar = AZUL;
            break;
        case AZUL:
            gpio_put(pino_led_azul, false);
            cor_atual_para_apagar = VERDE;
            break;
        case VERDE:
            gpio_put(pino_led_verde, false);
            cor_atual_para_apagar = VERMELHO;
            desativar_leds = false;
            break;
    }

    turn_off_time = make_timeout_time_ms(3000); // Atualiza tempo para a próxima cor

    return true;
}
