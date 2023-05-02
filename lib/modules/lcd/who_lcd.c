#include "who_lcd.h"
#include "esp_camera.h"
#include <string.h>
#include "logo_en_240x240_lcd.h"

static const char *TAG = "who_lcd";

static scr_driver_t g_lcd;
static scr_info_t g_lcd_info;

static QueueHandle_t xQueueFrameI = NULL;
static QueueHandle_t xQueueFrameO = NULL;
static bool gReturnFB = true;

static void task_process_handler(void *arg)
{
    camera_fb_t *frame = NULL;

    while (true)
    {
        if (xQueueReceive(xQueueFrameI, &frame, portMAX_DELAY))
        {
            g_lcd.draw_bitmap(0, 0, frame->width, frame->height, (uint16_t *)frame->buf);

            if (xQueueFrameO)
            {
                xQueueSend(xQueueFrameO, &frame, portMAX_DELAY);
            }
            else if (gReturnFB)
            {
                esp_camera_fb_return(frame);
            }
            else
            {
                free(frame);
            }
        }
    }
}

esp_err_t register_lcd(const QueueHandle_t frame_i, const QueueHandle_t frame_o, const bool return_fb)
{
    spi_config_t bus_conf = {
        .miso_io_num = BOARD_LCD_MISO,
        .mosi_io_num = BOARD_LCD_MOSI,
        .sclk_io_num = BOARD_LCD_SCK,
        .max_transfer_sz = 2 * 240 * 240 + 10,
    };
    spi_bus_handle_t spi_bus = spi_bus_create(SPI2_HOST, &bus_conf);

    scr_interface_spi_config_t spi_lcd_cfg = {
        .spi_bus = spi_bus,
        .pin_num_cs = BOARD_LCD_CS,
        .pin_num_dc = BOARD_LCD_DC,
        .clk_freq = 40 * 1000000,
        .swap_data = 0,
    };

    scr_interface_driver_t *iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_lcd_cfg, &iface_drv);
    esp_err_t ret = scr_find_driver(SCREEN_CONTROLLER_ST7789, &g_lcd);
    if (ESP_OK != ret)
    {
        return ret;
        ESP_LOGE(TAG, "screen find failed");
    }

    scr_controller_config_t lcd_cfg = {
        .interface_drv = iface_drv,
        .pin_num_rst = BOARD_LCD_RST,
        .pin_num_bckl = BOARD_LCD_BL,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .offset_hor = 0,
        .offset_ver = 0,
        .width = 240,
        .height = 240,
        .rotate = 0,
    };
    ret = g_lcd.init(&lcd_cfg);
    if (ESP_OK != ret)
    {
        return ESP_FAIL;
        ESP_LOGE(TAG, "screen initialize failed");
    }

    g_lcd.get_info(&g_lcd_info);
    ESP_LOGI(TAG, "Screen name:%s | width:%d | height:%d", g_lcd_info.name, g_lcd_info.width, g_lcd_info.height);

    app_lcd_set_color(0x000000);
    vTaskDelay(pdMS_TO_TICKS(200));
    app_lcd_draw_wallpaper();
    vTaskDelay(pdMS_TO_TICKS(200));

    xQueueFrameI = frame_i;
    xQueueFrameO = frame_o;
    gReturnFB = return_fb;
    xTaskCreatePinnedToCore(task_process_handler, TAG, 4 * 1024, NULL, 5, NULL, 0);

    return ESP_OK;
}

void app_lcd_draw_wallpaper()
{
    scr_info_t lcd_info;
    g_lcd.get_info(&lcd_info);

    uint16_t *pixels = (uint16_t *)heap_caps_malloc((logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, logo_en_240x240_lcd, (logo_en_240x240_lcd_width * logo_en_240x240_lcd_height) * sizeof(uint16_t));
    g_lcd.draw_bitmap(0, 0, logo_en_240x240_lcd_width, logo_en_240x240_lcd_height, (uint16_t *)pixels);
    heap_caps_free(pixels);
}

void app_lcd_set_color(int color)
{
    scr_info_t lcd_info;
    g_lcd.get_info(&lcd_info);
    uint16_t *buffer = (uint16_t *)malloc(lcd_info.width * sizeof(uint16_t));
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (size_t i = 0; i < lcd_info.width; i++)
        {
            buffer[i] = color;
        }

        for (int y = 0; y < lcd_info.height; y++)
        {
            g_lcd.draw_bitmap(0, y, lcd_info.width, 1, buffer);
        }

        free(buffer);
    }
}

void draw_bitmap(uint16_t *bitmap){
    g_lcd.draw_bitmap(0, 0, 240, 240, bitmap);
}

void set_color_bmp(int color, uint16_t *bitmap){
    scr_info_t lcd_info;
    g_lcd.get_info(&lcd_info);
    for (int i = 0; i < 240; i++){
        for(int j = 0; j < 240; j++){
            bitmap[i*240 + j] = color;
        }
    }
}


void draw_rectangle(int x, int y, int width, int height, uint16_t color){
  uint16_t *bitmap = (uint16_t *) malloc((width * height) * sizeof(uint16_t));

 
  for(int i = 0; i < (width * height); i++){
    bitmap[i] = color;
  }

  g_lcd.draw_bitmap(x, y, width, height, bitmap);

  free(bitmap);
}

void draw_rectangle_bmp(int x, int y, int width, int height, uint16_t color, uint16_t *bitmap){
    for(int i = y; i < (y + height); i++){
        for (int j = x; j < (x + width); j++){
            bitmap[i*240 + j] = color;
        }
    }
}

void draw_circle(int x0, int y0, int radius, uint16_t color, uint16_t bg_color){

  uint16_t *bitmap = (uint16_t *) malloc((4*radius*radius) * sizeof(uint16_t));

  for(int y = 0; y < 2*radius; y++){
    for(int x = 0; x < 2*radius; x++){
      if(((x - radius)*(x - radius) + (y - radius)*(y - radius) - radius*radius ) <= 0){
        bitmap[y*2*radius + x] = color;
      } else {
        bitmap[y*2*radius + x] = bg_color;
      }
    }
  }

  g_lcd.draw_bitmap(x0 - radius, y0 - radius, 2*radius, 2*radius, bitmap);
  
  free(bitmap);
}

void draw_circle_bmp(int x0, int y0, int radius, uint16_t color, uint16_t *bitmap){
    for(int i = (y0 - radius); i < (y0 + radius); i++){
        for(int j = (x0 - radius); j < (x0 + radius); j++){
            if(((j - x0)*(j - x0) + (i - y0)*(i - y0) - radius*radius ) <= 0){
                bitmap[i*240 + j] = color;
            }
        }
    }
}

uint16_t color_to_int(Lcd_color_t rgb){
    uint16_t color = (rgb.blue % 32) << 8 | (rgb.red % 32) << 3 | (rgb.green % 8);
    return color;
}