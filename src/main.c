#include <pebble.h>
#include "autoconfig.h"

static Window *window;
static Layer  *line_layer;
static InverterLayer* inverter_layer;

GFont custom_font;

static GPath *hour_arrow[2];
static const GPathInfo HOUR_HAND_POINTS[2] = {
	{
  	4,
    (GPoint []) {
      {-3, 0},
      {-3, -300},
	  {3, -300},
	  {3, 0},
    }
	},
	{
  	4,
    (GPoint []) {
      {-3, 0},
	  {3, 0},
	  {3, -300},
      {-3, -300}, 
    }
	}
};

static char* txt[] = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23"};

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
 layer_mark_dirty(window_get_root_layer(window));
}

static void drawClock(GPoint center, int hour, GContext *ctx){
	GPoint segA;
	GPoint segC;
	
	const int16_t secondHandLengthA = 150;
	const int16_t secondHandLengthC = 180;

	graphics_context_set_fill_color(ctx, getColor() ? GColorBlack : GColorWhite);
	graphics_fill_circle(ctx, center, 180);
	
	graphics_context_set_text_color(ctx, getColor() ? GColorWhite : GColorBlack );
	
	int minhour=(hour+24)-2;
	int maxhour=hour+24+3;
	
	for(int i=minhour*4; i<maxhour*4; i++){
		int32_t angle = TRIG_MAX_ANGLE * (i % (12*4)) / (12*4);
 		segA.y = (int16_t)(-cos_lookup(angle) * (int32_t)secondHandLengthA / TRIG_MAX_RATIO) + center.y;
 		segA.x = (int16_t)(sin_lookup(angle) * (int32_t)secondHandLengthA / TRIG_MAX_RATIO) + center.x;
		segC.y = (int16_t)(-cos_lookup(angle) * (int32_t)secondHandLengthC / TRIG_MAX_RATIO) + center.y;
 		segC.x = (int16_t)(sin_lookup(angle) * (int32_t)secondHandLengthC / TRIG_MAX_RATIO) + center.x;
		
		if(i % 4 == 0) {
			//graphics_draw_line(ctx, segA, segC);
			graphics_context_set_fill_color(ctx, getColor() ? GColorBlack : GColorWhite);
			graphics_fill_circle(	ctx, segC, 8);
			graphics_context_set_fill_color(ctx, getColor() ? GColorWhite : GColorBlack);
			graphics_fill_circle(	ctx, segC, 6);
			
			if(clock_is_24h_style()){
			graphics_draw_text(ctx,
      			txt[(i % (24*4))/4],
      			custom_font,
      			GRect(segA.x-25, segA.y-25, 50, 50),
		  		GTextOverflowModeWordWrap,
      			GTextAlignmentCenter,
      			NULL);	
			}
			else {
			graphics_draw_text(ctx,
      			(i % (12*4))/4 == 0 ? "12" : txt[(i % (12*4))/4],
      			custom_font,
      			GRect(segA.x-25, segA.y-25, 50, 50),
		  		GTextOverflowModeWordWrap,
      			GTextAlignmentCenter,
      			NULL);
			}
		}
		else {
			graphics_context_set_fill_color(ctx, getColor() ? GColorWhite : GColorBlack);
			graphics_fill_circle(	ctx, segC, 3);
		}
	}
}

static void linelayer_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	const GPoint center = grect_center_point(&bounds);
	const int16_t secondHandLength = 150;

	GPoint secondHand;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	
	int32_t second_angle = TRIG_MAX_ANGLE * ((t->tm_hour % 12) * 60 + t->tm_min) / (12 * 60);

	secondHand.y = (int16_t)(cos_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.y;
	secondHand.x = (int16_t)(-sin_lookup(second_angle) * (int32_t)secondHandLength / TRIG_MAX_RATIO) + center.x;
	
	drawClock(secondHand, t->tm_hour, ctx);
	
  	graphics_context_set_stroke_color(ctx, getColor() ? GColorBlack : GColorWhite);
	graphics_context_set_fill_color(ctx, getColor() ? GColorWhite : GColorBlack);
	
	for(int i=0; i<2; i++){
		gpath_move_to(hour_arrow[i], secondHand);
		gpath_rotate_to(hour_arrow[i], second_angle);
		gpath_draw_filled(ctx, hour_arrow[i]);
  		gpath_draw_outline(ctx, hour_arrow[i]);
	}
}

static void window_load(Window *window) {
 Layer *window_layer = window_get_root_layer(window);
 GRect bounds = layer_get_bounds(window_layer);
	
 line_layer = layer_create(bounds);
 layer_set_update_proc(line_layer, linelayer_update_proc);
 layer_add_child(window_layer, line_layer);
	
 inverter_layer = inverter_layer_create	(bounds);
 layer_set_hidden(inverter_layer_get_layer(inverter_layer), !getInverted());
 layer_add_child(window_layer, inverter_layer_get_layer(inverter_layer));

}

static void window_unload(Window *window) {
 layer_destroy(line_layer);
 inverter_layer_destroy(inverter_layer);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  // call autoconf_in_received_handler
  autoconfig_in_received_handler(iter, context);

  layer_set_hidden(inverter_layer_get_layer(inverter_layer), !getInverted());
  layer_mark_dirty(line_layer);
}

static void init(void) {
	autoconfig_init();
    window = window_create();
	
	window_set_window_handlers(window, (WindowHandlers) {
   	.load = window_load,
   	.unload = window_unload,	
    });

	custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_COMFORTAA_40));

	for(int i=0; i<2; i++){
		hour_arrow[i] = gpath_create(&HOUR_HAND_POINTS[i]);
	}	

	app_message_register_inbox_received(in_received_handler);

	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

	window_stack_push(window, true);
}

static void deinit(void) {
	autoconfig_deinit();
	fonts_unload_custom_font(custom_font);
	for(int i=0; i<2; i++){
		gpath_destroy(hour_arrow[i]);
	}
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}