# README - Controle de LEDs com Raspberry Pi Pico

## Descrição

Este projeto implementa um controle de LEDs utilizando um botão para ativar a sequência de iluminação e um temporizador para desligar os LEDs de forma escalonada. Ele foi desenvolvido para a Raspberry Pi Pico, utilizando a biblioteca `pico/stdlib.h` e recursos de temporização da `hardware/timer.h`.

## Explicação do Código

### Inclusão de Bibliotecas
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
```
- `stdio.h`: Usado para inicializar a comunicação serial.
- `pico/stdlib.h`: Biblioteca padrão para controle de GPIOs e outros periféricos do Raspberry Pi Pico.
- `hardware/timer.h`: Necessária para trabalhar com temporizadores.

### Definição de Constantes e Variáveis
```c
typedef enum cor { VERMELHO, AZUL, VERDE } cor;
```
- Define um tipo enumerado `cor` para representar as cores dos LEDs.

```c
const uint pino_botao = 5;
const uint pino_led_vermelho = 13;
const uint pino_led_azul = 12;
const uint pino_led_verde = 11;
```
- Define os pinos do botão e dos LEDs na Raspberry Pi Pico.

```c
absolute_time_t turn_off_time;
static volatile uint32_t ultimo_tempo = 0;
bool desativar_leds = false;
static cor cor_atual_para_apagar = VERMELHO;
```
- `turn_off_time`: Armazena o tempo de desligamento dos LEDs.
- `ultimo_tempo`: Usado para implementar debounce no botão.
- `desativar_leds`: Indica se os LEDs devem ser desligados.
- `cor_atual_para_apagar`: Controla qual LED será apagado no próximo ciclo.

### Configuração e Inicialização no `main()`
```c
void main() {
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
    
    gpio_put(pino_led_vermelho, false);
    gpio_put(pino_led_azul, false);
    gpio_put(pino_led_verde, false);

    gpio_set_irq_enabled_with_callback(pino_botao, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    struct repeating_timer timer;
    add_repeating_timer_ms(3000, repeating_timer_callback, NULL, &timer);
    
    turn_off_time = make_timeout_time_ms(0);
    
    while (true) {
        tight_loop_contents();
    }
}
```
- Configura os GPIOs e os LEDs.
- Ativa uma interrupção no botão.
- Inicia um temporizador periódico para controlar o desligamento dos LEDs.
- O loop principal mantém o programa em execução sem polling excessivo.

### Tratamento de Interrupção (`gpio_irq_handler`)
```c
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    bool debounce = (tempo_atual - ultimo_tempo) > (200 * 1000);

    if (!debounce) {
        return;
    }

    ultimo_tempo = tempo_atual;

    if (!desativar_leds) {
        gpio_put(pino_led_vermelho, true);
        gpio_put(pino_led_azul, true);
        gpio_put(pino_led_verde, true);
        desativar_leds = true;
        turn_off_time = make_timeout_time_ms(3000);
    }
}
```
- Garante que o botão só é reconhecido se um tempo mínimo (200ms) tiver passado desde o último acionamento (debounce).
- Liga todos os LEDs quando o botão é pressionado.

### Callback do Temporizador (`repeating_timer_callback`)
```c
bool repeating_timer_callback(struct repeating_timer *t) {
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
    
    turn_off_time = make_timeout_time_ms(3000);
    return true;
}
```
- Apaga os LEDs um a um a cada 3 segundos.
- Quando todos estão apagados, permite que o botão seja pressionado novamente.

## Como Executar

### Requisitos
- Raspberry Pi Pico
- SDK do Raspberry Pi Pico instalado
- Ferramentas de compilação (`cmake`, `make`, `gcc-arm-none-eabi`)

### Compilação e Upload
1. Clone o repositório do projeto:
   ```sh
   git clone <seu-repositorio>
   cd <seu-repositorio>
   ```
2. Configure o ambiente de compilação:
   ```sh
   mkdir build
   cd build
   cmake ..
   make
   ```
3. Envie o firmware para a Raspberry Pi Pico:
   - Conecte a Pico ao PC segurando o botão BOOTSEL.
   - Monte-a como um dispositivo de armazenamento USB.
   - Copie o arquivo `.uf2` gerado na pasta `build` para a Pico.

Após reiniciar, pressione o botão para ativar os LEDs. Eles desligarão um a um a cada 3 segundos.

## Conclusão
Este projeto demonstra como utilizar interrupções para capturar eventos de botões e como usar temporizadores para tarefas periódicas na Raspberry Pi Pico. O código evita polling excessivo e implementa um debounce eficiente para garantir um funcionamento confiável.

