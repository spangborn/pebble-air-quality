#include <pebble.h>

static Window *window;

static TextLayer *aqi_layer;
static TextLayer *aqi_level_layer;
static TextLayer *city_layer;
static BitmapLayer *icon_layer;
static GBitmap *icon_bitmap = NULL;

static AppSync sync;
static uint8_t sync_buffer[128];

enum AirQualityKey {
  AIR_QUALITY_ICON_KEY = 0,       // TUPLE_INT
  AIR_QUALITY_AQI_KEY = 1,        // TUPLE_CSTRING
  AIR_QUALITY_AQI_LEVEL_KEY = 2,  // TUPLE_CSTRING
  AIR_QUALITY_CITY_KEY = 3,       // TUPLE_CSTRING
};

static const uint32_t AIR_QUALITY_ICONS[] = {
  RESOURCE_ID_IMAGE_GOOD, //0
  RESOURCE_ID_IMAGE_MODERATE, //1
  RESOURCE_ID_IMAGE_UNHEALTHY_FOR_SENSITIVE_GROUPS, //2
  RESOURCE_ID_IMAGE_UNHEALTHY, //3
  RESOURCE_ID_IMAGE_VERY_UNHEALTHY, //4
  RESOURCE_ID_IMAGE_HAZARDOUS //5
};


static char reasonStr[20];

static void getAppMessageResult(AppMessageResult reason){

  switch(reason){
    case APP_MSG_OK:
      snprintf(reasonStr,20,"%s","APP_MSG_OK");
    break;
    case APP_MSG_SEND_TIMEOUT:
      snprintf(reasonStr,20,"%s","SEND TIMEOUT");
    break;
    case APP_MSG_SEND_REJECTED:
      snprintf(reasonStr,20,"%s","SEND REJECTED");
    break;
    case APP_MSG_NOT_CONNECTED:
      snprintf(reasonStr,20,"%s","NOT CONNECTED");
    break;
    case APP_MSG_APP_NOT_RUNNING:
      snprintf(reasonStr,20,"%s","NOT RUNNING");
    break;
    case APP_MSG_INVALID_ARGS:
      snprintf(reasonStr,20,"%s","INVALID ARGS");
    break;
    case APP_MSG_BUSY:
      snprintf(reasonStr,20,"%s","BUSY");
    break;
    case APP_MSG_BUFFER_OVERFLOW:
      snprintf(reasonStr,20,"%s","BUFFER OVERFLOW");
    break;
    case APP_MSG_ALREADY_RELEASED:
      snprintf(reasonStr,20,"%s","ALRDY RELEASED");
    break;
    case APP_MSG_CALLBACK_ALREADY_REGISTERED:
      snprintf(reasonStr,20,"%s","CLB ALR REG");
    break;
    case APP_MSG_CALLBACK_NOT_REGISTERED:
      snprintf(reasonStr,20,"%s","CLB NOT REG");
    break;
    case APP_MSG_OUT_OF_MEMORY:
      snprintf(reasonStr,20,"%s","OUT OF MEM");
    break;

  }
}
static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  getAppMessageResult(app_message_error);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %s", reasonStr);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case AIR_QUALITY_ICON_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "ICON: %lu", ( unsigned long )new_tuple->value->uint8 );
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }
      icon_bitmap = gbitmap_create_with_resource(AIR_QUALITY_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case AIR_QUALITY_AQI_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "AQI: %s", new_tuple->value->cstring );
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(aqi_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_CITY_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "CITY: %s", new_tuple->value->cstring );
      text_layer_set_text(city_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_AQI_LEVEL_KEY:
      text_layer_set_text(aqi_level_layer, new_tuple->value->cstring);
      break;
  }
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(0, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  icon_layer = bitmap_layer_create(GRect(32, 0, 80, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(icon_layer));

  aqi_layer = text_layer_create(GRect(0, 60, 144, 68));
  text_layer_set_text_color(aqi_layer, GColorWhite);
  text_layer_set_background_color(aqi_layer, GColorClear);
  text_layer_set_font(aqi_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(aqi_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(aqi_layer));

  aqi_level_layer = text_layer_create(GRect(0, 88, 144, 68));
  text_layer_set_text_color(aqi_level_layer, GColorWhite);
  text_layer_set_background_color(aqi_level_layer, GColorClear);
  text_layer_set_font(aqi_level_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(aqi_level_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(aqi_level_layer));

  city_layer = text_layer_create(GRect(0, 115, 144, 68));
  text_layer_set_text_color(city_layer, GColorWhite);
  text_layer_set_background_color(city_layer, GColorClear);
  text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(city_layer));

  Tuplet initial_values[] = {
    TupletInteger(AIR_QUALITY_ICON_KEY, (uint8_t) 0),
    TupletCString(AIR_QUALITY_AQI_KEY, "0"),
    TupletCString(AIR_QUALITY_AQI_LEVEL_KEY, ""),
    TupletCString(AIR_QUALITY_CITY_KEY, "Loading...")
    
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();
}

static void window_unload(Window *window) {
  app_sync_deinit(&sync);

  if (icon_bitmap) {
    gbitmap_destroy(icon_bitmap);
  }

  text_layer_destroy(city_layer);
  text_layer_destroy(aqi_layer);
  text_layer_destroy(aqi_level_layer);  
  bitmap_layer_destroy(icon_layer);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}