#include <stdio.h>
#include <stdlib.h>
#include "driver/dac_oneshot.h"
#include "driver/timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"
#include "driver/i2s.h"

#include "driver/gpio.h"

#define LED_AZUL_GPIO GPIO_NUM_2  // Cambia esto al pin correcto para tu placa

#include <stdio.h>
#include <stdlib.h>
#include <string.h>








#define TIMER_INTERVAL_US 22  // 8kHz
#define WAV_FILE_PATH "/spiffs/IntPointVar.wav"


typedef enum {
    ROJO,
    AMARILLO,
    VERDE
}audio_t;

audio_t audio_actual=ROJO;
bool audio_cargado=true;

typedef struct {
    uint8_t *data;
    size_t size;
} audio_data_t;

audio_data_t audio_rojo;
audio_data_t audio_verde;

static const char *TAG = "DAC_LOOP";
static dac_oneshot_handle_t dac_handle;

static volatile size_t play_index = 0;

// ISR del timer
static void IRAM_ATTR timer_isr(void *arg)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    const uint8_t *data = NULL;
    size_t size = 0;

    // Asignar el buffer según el audio actual
    if (audio_actual == ROJO) {
        data = audio_rojo.data;
        size = audio_rojo.size;
    } else if (audio_actual == VERDE) {
        data = audio_verde.data;
        size = audio_verde.size;
    } else {
        return;  // Si es AMARILLO, no hacer nada
    }

    // Verificar que el buffer sea válido
    if (data == NULL || size == 0) return;

    // Protección simple para evitar desborde
    if (play_index >= size) play_index = 0;

    // Reproducir muestra


    gpio_set_level(LED_AZUL_GPIO, 1);  // Nivel alto = encender 
    dac_oneshot_output_voltage(dac_handle, data[play_index]);
    gpio_set_level(LED_AZUL_GPIO, 0);  // Nivel alto = encender


    play_index++;
}


void init_timer()
{
    timer_config_t config = {
        .divider = 80,  // 80 MHz / 80 = 1 MHz (1 tick = 1us)
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
        .intr_type = TIMER_INTR_LEVEL,
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_INTERVAL_US);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

void init_dac()
{
    dac_oneshot_config_t dac_config = {
        .chan_id = DAC_CHAN_0  // GPIO25
    };

    esp_err_t ret = dac_oneshot_new_channel(&dac_config, &dac_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando DAC: %s", esp_err_to_name(ret));
    }
}

void init_spiffs()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error montando SPIFFS: %s", esp_err_to_name(ret));
    }
}
#include <stdio.h>
#include <stdlib.h>


#include <stdio.h>
#include <stdlib.h>

esp_err_t load_wav_to_ram(audio_data_t *audio, const char *file_path)
{
    if (!audio || !file_path) {
        return ESP_ERR_INVALID_ARG;
    }

    audio->data = NULL;
    audio->size = 0;

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s\n", file_path);
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    long total_size = ftell(file);
    rewind(file);

    if (total_size <= 44) {
        printf("Error: El archivo %s es muy pequeño o inválido\n", file_path);
        fclose(file);
        return ESP_FAIL;
    }

    audio->size = total_size - 44;
    audio->data = malloc(audio->size);
    if (!audio->data) {
        printf("Error: No hay suficiente memoria para cargar el archivo\n");
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    fseek(file, 44, SEEK_SET);  // Saltar encabezado WAV
    size_t read_bytes = fread(audio->data, 1, audio->size, file);
    fclose(file);

    if (read_bytes != audio->size) {
        printf("Error: No se pudo leer el archivo completo\n");
        free(audio->data);
        audio->data = NULL;
        audio->size = 0;
        return ESP_FAIL;
    }

    return ESP_OK;
}




int parse_input() {
    char buffer[32];


    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 2;  // Error al leer
    }

    // Eliminar salto de línea si lo hay
    buffer[strcspn(buffer, "\n")] = '\0';

    // Convertir a entero
    char *endptr;
    int valor = strtol(buffer, &endptr, 10);

    if (endptr == buffer || *endptr != '\0') {
        // No se pudo convertir completamente (por ejemplo, entrada no numérica)
        return 2;
    }

    audio_t aux= AMARILLO;//dEFAULT SIN AUDIO
    if (valor == 0) {
        aux =ROJO;
    } else if (valor == 1) {
        aux =VERDE;
    } else {
        aux= AMARILLO;
    }

    if(aux!=audio_actual)
        audio_cargado=false;

    return aux;


}
 void initDacDma(){

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = 8000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // o LEFT GPIO ??
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    i2s_set_clk(I2S_NUM_0, 8000, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);  // DAC interno: GPIO25


 }


void app_main(void){
    init_spiffs();
    //Leo lso archivos
    esp_err_t err1 = load_wav_to_ram(&audio_rojo, "/spiffs/Alarma01_8k.wav");
    esp_err_t err2 = load_wav_to_ram(&audio_verde, "/spiffs/Alarma02_8k.wav");
    gpio_reset_pin(LED_AZUL_GPIO);
    gpio_set_direction(LED_AZUL_GPIO, GPIO_MODE_OUTPUT);
    initDacDma();
    if (err1 == ESP_OK && err2 == ESP_OK) {
        printf("Audio rojo cargado: %d bytes\n", audio_rojo.size);
        printf("Audio verde cargado: %d bytes\n", audio_verde.size);

        // Usar audio_rojo.data y audio_verde.data para reproducir

    } else {
        printf("Error cargando uno o ambos audios.\n");
    }
    //Se tiene que iniciar el timer despues de cargar los archivos, si no
    //la interrupcion crashea el programa 



    //I2S necesita datos de 16bits por lo que se ponen los 8bits de audio en los mas significativos
    uint16_t *i2s_rojo_data = malloc(audio_rojo.size * sizeof(uint16_t));
    uint16_t *i2s_verde_data = malloc(audio_verde.size * sizeof(uint16_t));

    for (int i = 0; i < audio_rojo.size; i++) {
        i2s_rojo_data[i] = ((uint16_t)audio_rojo.data[i]) << 8;
    }

    for (int i = 0; i < audio_verde.size; i++) {
        i2s_verde_data[i] = ((uint16_t)audio_verde.data[i]) << 8;
    }
 
    size_t pos=0;

    
    while(1){

        // Reproducir una vez y se queda trabado hasta que temrine
        size_t bytes_written = 0;
        esp_err_t ret;


        int resultado = parse_input();


        if(audio_cargado==false){
            ESP_LOGE(TAG, "Cambiando audio:");
            switch(resultado){
                case ROJO:
                    play_index=0;//Reinicio el audio o sea arranco desde el 0
                    audio_actual=ROJO;
                    audio_cargado=true;
                    ESP_LOGE(TAG, "ROJO");
                    break;
                case VERDE:
                    play_index=0;//Reinicio el audio o sea arranco desde el 0
                    audio_actual=VERDE;
                    audio_cargado=true;
                    ESP_LOGE(TAG, "VERDE");
                    break;
                default:
                    audio_actual=AMARILLO;
                    audio_cargado=true;
                    ESP_LOGE(TAG, "AMARILLO(DEFAULT)");
                    break;
            }
        }

    

 
        switch(audio_actual){

            case ROJO:
                gpio_set_level(LED_AZUL_GPIO, 1);  // Nivel alto = encender 
                size_t bytes_left = (audio_rojo.size * sizeof(uint16_t)) - pos;
                size_t bytes_written = 0;
                esp_err_t ret = i2s_write(I2S_NUM_0, &i2s_rojo_data[pos / 2], bytes_left, &bytes_written, 0);
                pos += bytes_written;
                if (pos >= audio_rojo.size * sizeof(uint16_t)) pos = 0;

                gpio_set_level(LED_AZUL_GPIO, 0);  // Nivel alto = encender

                break;

            case VERDE:
                ret = i2s_write(I2S_NUM_0, i2s_verde_data, audio_verde.size * sizeof(uint16_t), &bytes_written, portMAX_DELAY);
                break;
            default:
                break;


        }
        vTaskDelay(1);

    }
    





}
/*

//Main para el modo con dac one shot
void app_main2(void)
{
    init_spiffs();
    init_dac();

    esp_err_t err1 = load_wav_to_ram(&audio_rojo, "/spiffs/Alarma01_8k.wav");
    esp_err_t err2 = load_wav_to_ram(&audio_verde, "/spiffs/Alarma02_8k.wav");
    gpio_reset_pin(LED_AZUL_GPIO);
    gpio_set_direction(LED_AZUL_GPIO, GPIO_MODE_OUTPUT);

    if (err1 == ESP_OK && err2 == ESP_OK) {
        printf("Audio rojo cargado: %d bytes\n", audio_rojo.size);
        printf("Audio verde cargado: %d bytes\n", audio_verde.size);

        // Usar audio_rojo.data y audio_verde.data para reproducir

    } else {
        printf("Error cargando uno o ambos audios.\n");
    }
    //Se tiene que iniciar el timer despues de cargar los archivos, si no
    //la interrupcion crashea el programa 

    init_timer() ;  
    vTaskDelay(1000);
    //while(true){vTaskDelay(1000);}

    while (1) {

        int resultado = parse_input();


        if(audio_cargado==false){
            ESP_LOGE(TAG, "Cambiando audio:");

            switch(resultado){
                case ROJO:
                    play_index=0;//Reinicio el audio o sea arranco desde el 0
                    audio_actual=ROJO;
                    audio_cargado=true;
                    ESP_LOGE(TAG, "ROJO");

                    break;
                case VERDE:
                    play_index=0;//Reinicio el audio o sea arranco desde el 0
                    audio_actual=VERDE;
                    audio_cargado=true;
                    ESP_LOGE(TAG, "VERDE");

                    break;

                default:
                    audio_actual=AMARILLO;
                    audio_cargado=true;

                    ESP_LOGE(TAG, "AMARILLO(DEFAULT)");
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(700));  // Nada más que hacer

    }
}


*/