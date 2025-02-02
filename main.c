#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

typedef enum cor { VERMELHO, AZUL, VERDE } cor; // enum para as cores

const uint pino_botao = 5;  // declarando o pino do botao a
const uint pino_led_vermelho = 13; // declarando o pino do led vermelho
const uint pino_led_azul = 12; // declarando o pino do led azul
const uint pino_led_verde = 11; // declarando o pino do led verde
 
absolute_time_t turn_off_time; // variavel para o temporizador
static volatile uint32_t ultimo_tempo = 0; // variável para debounce

// variaveis de controle
bool desativar_leds = false;
static cor cor_atual_para_apagar = VERMELHO;

// declaração do prótotipo das funções
bool repeating_timer_callback(struct repeating_timer *t);
void gpio_irq_handler(uint gpio, uint32_t events);


int main() {
    stdio_init_all(); // inicializando a comunicação serial

    gpio_init(pino_botao); // inicializando o pino do botao
    gpio_set_dir(pino_botao, GPIO_IN); // declarando como entrada
    gpio_pull_up(pino_botao); // colocando resistor pull up

    gpio_init(pino_led_vermelho); // inicializando o pino do led vermelho
    gpio_set_dir(pino_led_vermelho, GPIO_OUT); // declarando como saida
    gpio_init(pino_led_azul); // inicializando o pino do led azul
    gpio_set_dir(pino_led_azul, GPIO_OUT); // declarando como saida
    gpio_init(pino_led_verde); // inicializando o pino do led verde
    gpio_set_dir(pino_led_verde, GPIO_OUT); // declarando como saida
    
    // Garantindo que os LEDs comecem apagados
    gpio_put(pino_led_vermelho, false);
    gpio_put(pino_led_azul, false);
    gpio_put(pino_led_verde, false);

    gpio_set_irq_enabled_with_callback(pino_botao, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // declarando uma interrupção para o botao 
    // a interrupção é acionada com a borda de descida

    struct repeating_timer timer; // declarando o timer
    add_repeating_timer_ms(3000, repeating_timer_callback, NULL, &timer); // temporizador para desativar os loops

    turn_off_time = make_timeout_time_ms(0); // Inicializando tempo de desligamento

    while (true) { // o loop principal nao eh utilizado evitando o polling
        tight_loop_contents();
    }
}


/*
* Função para tratar a interrupção
*/
void gpio_irq_handler(uint gpio, uint32_t events) {
    // Implementação do debounce
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    bool debounce = (tempo_atual - ultimo_tempo) > (200 * 1000); // 200ms debounce

    if (!debounce) { // Ignora se debounce não passou
        return;
    }

    ultimo_tempo = tempo_atual; // Atualizando o tempo de última ativação do botão

    if (!desativar_leds) {
        gpio_put(pino_led_vermelho, true); // ativando os leds
        gpio_put(pino_led_azul, true);
        gpio_put(pino_led_verde, true);
        desativar_leds = true; // permite que os leds sejam desativados agora
        turn_off_time = make_timeout_time_ms(3000); // Inicia contagem para desligar LEDs
    }
}

/*
* Função para o temporizador
*/
bool repeating_timer_callback(struct repeating_timer *t) {
    // Se os LEDs já estiverem desligados, não faz nada
    if (!desativar_leds || (absolute_time_diff_us(get_absolute_time(), turn_off_time) > 0)) {
        return true;
    }

    switch (cor_atual_para_apagar) { // estrutura para apagar a cor e selecionar a proxima
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
            desativar_leds = false; // garante que todos os leds estao apagados
            break;
    }

    turn_off_time = make_timeout_time_ms(3000); // Atualiza tempo para a próxima cor

    return true;
}
