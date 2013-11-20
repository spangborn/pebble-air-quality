#include <pebble.h>

static Window *window;

static TextLayer *pm25_aqi_layer;
static TextLayer *pm25_aqi_level_layer;
static TextLayer *o3_aqi_layer;
static TextLayer *o3_aqi_level_layer;
static TextLayer *city_layer;

static BitmapLayer *pm25_icon_layer;
static BitmapLayer *o3_icon_layer;

static GBitmap *pm25_icon_bitmap = NULL;
static GBitmap *o3_icon_bitmap = NULL;

static AppSync sync;
static uint8_t sync_buffer[64];

enum AirQualityKey {
  AIR_QUALITY_PM25_ICON_KEY = 0,       // TUPLE_INT
  AIR_QUALITY_PM25_AQI_KEY = 1,        // TUPLE_CSTRING
  AIR_QUALITY_PM25_AQI_LEVEL_KEY = 2,  // TUPLE_CSTRING
  AIR_QUALITY_O3_ICON_KEY = 3,         // TUPLE_INT
  AIR_QUALITY_O3_AQI_KEY = 4,          // TUPLE_CSTRING
  AIR_QUALITY_O3_AQI_LEVEL_KEY = 5,    // TUPLE_CSTRING
  AIR_QUALITY_CITY_KEY = 6,            // TUPLE_CSTRING
};

static const uint32_t AIR_QUALITY_ICONS[] = {
  RESOURCE_ID_IMAGE_GOOD, //0
  RESOURCE_ID_IMAGE_MODERATE, //1
  RESOURCE_ID_IMAGE_UNHEALTHY_FOR_SENSITIVE_GROUPS, //2
  RESOURCE_ID_IMAGE_UNHEALTHY, //3
  RESOURCE_ID_IMAGE_VERY_UNHEALTHY, //4
  RESOURCE_ID_IMAGE_HAZARDOUS, //5
  RESOURCE_ID_IMAGE_HAZARDOUS //6
};



static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case AIR_QUALITY_PM25_ICON_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "ICON: %lu", ( unsigned long )new_tuple->value->uint8 );
      if (pm25_icon_bitmap) {
        gbitmap_destroy(pm25_icon_bitmap);
      }
      pm25_icon_bitmap = gbitmap_create_with_resource(AIR_QUALITY_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(pm25_icon_layer, pm25_icon_bitmap);
      break;

    case AIR_QUALITY_O3_ICON_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "ICON: %lu", ( unsigned long )new_tuple->value->uint8 );
      if (o3_icon_bitmap) {
        gbitmap_destroy(o3_icon_bitmap);
      }
      o3_icon_bitmap = gbitmap_create_with_resource(AIR_QUALITY_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(o3_icon_layer, o3_icon_bitmap);
      break;

    case AIR_QUALITY_PM25_AQI_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "AQI: %s", new_tuple->value->cstring );
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(pm25_aqi_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_PM25_AQI_LEVEL_KEY:
      text_layer_set_text(pm25_aqi_level_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_O3_AQI_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "AQI: %s", new_tuple->value->cstring );
      // App Sync keeps new_tuple in sync_buffer, so we may use it directly
      text_layer_set_text(o3_aqi_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_O3_AQI_LEVEL_KEY:
      text_layer_set_text(o3_aqi_level_layer, new_tuple->value->cstring);
      break;

    case AIR_QUALITY_CITY_KEY:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "CITY: %s", new_tuple->value->cstring );
      text_layer_set_text(city_layer, new_tuple->value->cstring);
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

  o3_icon_layer = bitmap_layer_create(GRect(0, 0, 80, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(o3_icon_layer));

  pm25_icon_layer = bitmap_layer_create(GRect(67, 0, 80, 80));
  layer_add_child(window_layer, bitmap_layer_get_layer(pm25_icon_layer));

  o3_aqi_layer = text_layer_create(GRect(0, 60, 67, 68));
  text_layer_set_text_color(o3_aqi_layer, GColorWhite);
  text_layer_set_background_color(o3_aqi_layer, GColorClear);
  text_layer_set_font(o3_aqi_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(o3_aqi_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(o3_aqi_layer));

  pm25_aqi_layer = text_layer_create(GRect(68, 60, 67, 68));
  text_layer_set_text_color(pm25_aqi_layer, GColorWhite);
  text_layer_set_background_color(pm25_aqi_layer, GColorClear);
  text_layer_set_font(pm25_aqi_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(pm25_aqi_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(pm25_aqi_layer));


  o3_aqi_level_layer = text_layer_create(GRect(0, 88, 144, 68));
  text_layer_set_text_color(o3_aqi_level_layer, GColorWhite);
  text_layer_set_background_color(o3_aqi_level_layer, GColorClear);
  text_layer_set_font(o3_aqi_level_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(o3_aqi_level_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(o3_aqi_level_layer));

  pm25_aqi_level_layer = text_layer_create(GRect(68, 88, 67, 68));
  text_layer_set_text_color(pm25_aqi_level_layer, GColorWhite);
  text_layer_set_background_color(pm25_aqi_level_layer, GColorClear);
  text_layer_set_font(pm25_aqi_level_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(pm25_aqi_level_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(pm25_aqi_level_layer));

  city_layer = text_layer_create(GRect(0, 115, 144, 68));
  text_layer_set_text_color(city_layer, GColorWhite);
  text_layer_set_background_color(city_layer, GColorClear);
  text_layer_set_font(city_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(city_layer));

  Tuplet initial_values[] = {
    TupletInteger(AIR_QUALITY_PM25_ICON_KEY, (uint8_t) 0),
    TupletCString(AIR_QUALITY_PM25_AQI_KEY, "0"),
    TupletCString(AIR_QUALITY_PM25_AQI_LEVEL_KEY, ""),
    TupletInteger(AIR_QUALITY_O3_ICON_KEY, (uint8_t) 0),
    TupletCString(AIR_QUALITY_O3_AQI_KEY, "0"),
    TupletCString(AIR_QUALITY_O3_AQI_LEVEL_KEY, ""),
    TupletCString(AIR_QUALITY_CITY_KEY, "Loading...")
    
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();
}

static void window_unload(Window *window) {
  app_sync_deinit(&sync);

  if (pm25_icon_bitmap) {
    gbitmap_destroy(pm25_icon_bitmap);
  }
  if (o3_icon_bitmap) {
    gbitmap_destroy(o3_icon_bitmap);
  }

  text_layer_destroy(city_layer);
  text_layer_destroy(pm25_aqi_layer);
  text_layer_destroy(pm25_aqi_level_layer);  
  bitmap_layer_destroy(pm25_icon_layer);
  text_layer_destroy(o3_aqi_layer);
  text_layer_destroy(o3_aqi_level_layer);  
  bitmap_layer_destroy(o3_icon_layer);
}

static void init(void) {
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  const int inbound_size = 64;
  const int outbound_size = 64;
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