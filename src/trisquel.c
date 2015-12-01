#include <pebble.h>

static void init();
static void deinit();
static void main_window_load(Window *);
static void main_window_unload(Window *);
static void tick_handler(struct tm *, TimeUnits);
static void update_time();
static void bluetooth_callback(bool);

static Window *s_window;
static TextLayer *s_time;
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;

int main(void) {
	init();
  	app_event_loop();
  	deinit();
}

static void init()
{
	// create window
	s_window = window_create();

	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_set_background_color(s_window, GColorWhite);
	window_stack_push(s_window, true);
	update_time();
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// register for Bluetooth connection updates
	#ifdef PBL_SDK_2
	bluetooth_connection_service_subscribe(bluetooth_callback);
	#elif PBL_SDK_3
	connection_service_subscribe((ConnectionHandlers) {
		.pebble_app_connection_handler = bluetooth_callback
	});
	#endif
}

static void deinit()
{
	// destroy window
	window_destroy(s_window);
}

static void main_window_load(Window *window)
{
    // get information about the window
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    // create GBitmap
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

    // create BitmapLayer to display the GBitmap
    s_background_layer = bitmap_layer_create(bounds);

    // set the bitmap onto the layer and add to the window
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

    // create the Bluetooth icon GBitmap
    s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

    // create the BitmapLayer to display the GBitmap
    s_bt_icon_layer = bitmap_layer_create(GRect(0, 0, 30, 30));
    bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

    

    // create the textlayer
    s_time = text_layer_create(
      GRect(0, 120, bounds.size.w, 50));

    // improve the layout
    text_layer_set_font(s_time, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
    text_layer_set_background_color(s_time, GColorClear);
    text_layer_set_text_color(s_time, GColorWhite);
    text_layer_set_text(s_time, "00:00");
    text_layer_set_text_alignment(s_time, GTextAlignmentCenter);

    // add it as a child layer to the window's root layer
    layer_add_child(window_layer, text_layer_get_layer(s_time));

    #if defined(PBL_SDK_2)
  	bluetooth_callback(bluetooth_connection_service_peek());
	#elif defined(PBL_SDK_3)
  	bluetooth_callback(connection_service_peek_pebble_app_connection());
	#endif
}

static void main_window_unload(Window *window)
{
	// destroy textlayer
	text_layer_destroy(s_time);

	// destroy GBitmap
	gbitmap_destroy(s_background_bitmap);

	gbitmap_destroy(s_bt_icon_bitmap);
	bitmap_layer_destroy(s_bt_icon_layer);

	// destroy BitmapLayer
	bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	update_time();
}

static void update_time()
{
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);

	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
	text_layer_set_text(s_time, s_buffer);
}

static void bluetooth_callback(bool connected)
{
	// show icon if disconnected
	layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

	if (!connected) {
		// Issue a vibrating alert
		vibes_double_pulse();
	}
}