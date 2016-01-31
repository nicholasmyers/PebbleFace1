#include <pebble.h>
#include "main.h"
static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_weather_layer;
static GFont s_time_font, s_weather_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void main_window_load(Window *window) {
  
	//create font
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
	s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
	
  //get info about window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
	
	//create GBitmap background
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BITMAP_FUN_BACKGROUND);
	//create BitmapLayer to display GBitmap
	s_background_layer = bitmap_layer_create(bounds);
	//set the bitmap onto the layer and add to the window
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	
	/* TIME LAYER */
  //create the time layer with specific bounds
  s_time_layer = text_layer_create(
    GRect(2, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  
  //improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	
	/* DATE LAYER */
	//create date layer
	s_date_layer = text_layer_create(
		GRect(2, PBL_IF_ROUND_ELSE(134, 130), bounds.size.w, 50));
	
	//add date layer information
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_font(s_date_layer, s_weather_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	
	/* WEATHER LAYER */
	//create temperature layer
	s_weather_layer = text_layer_create(
		GRect(2, PBL_IF_ROUND_ELSE(8, 10), bounds.size.w, 25));
	
	//style the text
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorWhite);
	text_layer_set_font(s_weather_layer, s_weather_font);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
	text_layer_set_text(s_weather_layer, "Loading...");
  
	
  //add both as children layers to the window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window) {
	
	//unload GFont
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_weather_font);
  //destroy TextLayer
  text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	
	//destroy GBitmap
	gbitmap_destroy(s_background_bitmap);
	//destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
}

static void update_time() {
  //get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  //write the current hours and minutes into a buffer
  static char s_buffer[9];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  //display this time on TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}
		
static void update_date() {
	//get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_date = localtime(&temp);
	
	//write current weekday and day into buffer
	static char s_buffer[12];
	strftime(s_buffer, sizeof(s_buffer), "%a %b. %d", tick_date);
	
	//display this date on TextLayer
	text_layer_set_text(s_date_layer, s_buffer);
}

//allows access to current time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
	
	//get weather update every 30 mins
	if(tick_time->tm_min % 30 == 0) {
		//begin dictionary
		DictionaryIterator *iter;
		app_message_outbox_begin(&iter);
		
		//add a key-value pair
		dict_write_uint8(iter, 0, 0);
		
		//send the message
		app_message_outbox_send();
	}
}
		
static void date_handler(struct tm *tick_date, TimeUnits units_changed) {
	update_date();
}

//app CallBack function to process incoming messages and errors from JS (get weather)
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	//store incoming information
	static char temperature_buffer[8];
	static char conditions_buffer[32];
	static char weather_layer_buffer[32];
	
	//read tuples for data
	Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
	Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
	
	//if all data is available, use it
	if(temp_tuple && conditions_tuple) {
		snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
		snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
		
		//assemble full string and display
		snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
		text_layer_set_text(s_weather_layer, weather_layer_buffer);
	}
}

//see outcomes and any errors that occur during app callback
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  //create main window element and assign to pointer
  s_main_window = window_create();
	
	window_set_background_color(s_main_window, GColorBlack);
  
  //set handlers to manage the elements inside the window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  //show the window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  //register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	
	tick_timer_service_subscribe(DAY_UNIT, date_handler);
  
  //make sure time and date are displayed from start
  update_time();
	update_date();
	
	//register callbacks (before opening appMessage to prevent missing messages)
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	
	//open appMessage to allow the watchface to receive incoming messages
	//the functions in param 0, 1 obtain maximum available buffer sizes
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  //destroy window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}