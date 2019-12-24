/*
 * tu77.c
 *
 * A visual display of the DECtape TU77 front panel
 * 
 * for the Raspberry Pi and other Linux systems
 * 
 * Copyright 2019  rricharz
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#define TSTATE_ONLINE		 1
#define TSTATE_DRIVE1		 2
#define TSTATE_BACKWARDS	 4
#define TSTATE_SEEK		 	8
#define TSTATE_READ			16
#define TSTATE_WRITE		32

#define GDK_DISABLE_DEPRECATION_WARNINGS

#include <cairo.h>
#include <math.h>
#include <gtk/gtk.h>
#include <sys/time.h>

// The TU77 tape status is checked every TIME_INTERVAL milliseconds
// If the status changes, the display is updated immediately
//
// Blurred rotational motion pictures of the reels are only updated
// every TIME_INTERVAL * C_ANIMATION, to reduce CPU usage
//
// Tape motion on and off times are extended to a minimal time of
// TIME_INTERVAL * C_TAPE to make them visible 

#define TIME_INTERVAL    40		// timer interval in msec

#define REEL1X			370
#define REEL1Y			98
#define REEL2X			365
#define REEL2Y			568

#define NUMANGLES		10		// number of angles simulated


struct {
  cairo_surface_t *image;
  cairo_surface_t *reel1[NUMANGLES], *reel1bl[NUMANGLES];
  double scale;
  int remote_status, last_remote_status;
  int argFullscreen, argAudio;
  int requested_speed, actual_speed;
  long angle;
} glob;

long d_mSeconds()
// return time in msec since last call
{
        static int initialized = 0;
        static long lastTime;
        struct timeval tv;
        gettimeofday(&tv,NULL);
        long t = (1000 * tv.tv_sec)  + (tv.tv_usec/1000);
        if (!initialized) lastTime = t;
        initialized = 1;
        long t1 = t - lastTime;
        lastTime = t;     
        return t1;
}

int getStatus()
{
	static FILE *statusFile = 0;
	char *fname = "/tmp/tu56status";
	int st;
	
	// file needs to be opened again each time to read new status
	statusFile = fopen(fname, "r");
		
	if (statusFile != 0) {
		st = getc(statusFile) - 32;
		fclose(statusFile);
		if (st >= 0)
			return st;
		else
			return 0;
	}
	else return 0;
}

static void do_drawing(cairo_t *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, 
    gpointer user_data)
{      
  do_drawing(cr);

  return FALSE;
}

static void do_drawing(cairo_t *cr)
{
	long delta_t = d_mSeconds();
	long delta_angle = glob.actual_speed * delta_t * 36 / 100;
	glob.angle = (glob.angle + delta_angle) % 360;
	
	int index = glob.angle * NUMANGLES / 360;
	
	printf("do_drawing, delta_time = %ld ms, delta_angle = %ld deg, angle = %d, index = %d\n",
		delta_t, delta_angle, glob.angle, index);
	
	cairo_scale(cr,glob.scale,glob.scale);
	
	cairo_set_source_surface(cr, glob.image, 0, 0);
	cairo_paint(cr);
	
	cairo_set_source_surface(cr, glob.reel1bl[index], REEL1X, REEL1Y);
	cairo_paint(cr);
	
	cairo_set_source_surface(cr, glob.reel1bl[index], REEL2X, REEL2Y);
	cairo_paint(cr);	
}

static void do_logic()
{	

	glob.remote_status = getStatus();
	if (glob.last_remote_status != glob.remote_status)
		printf("remote state = 0x%02x\n", glob.remote_status);
		
	if ((glob.remote_status & TSTATE_WRITE) || (glob.remote_status & TSTATE_READ) || (glob.remote_status & TSTATE_SEEK))
		glob.requested_speed = 6; // rps
	else
		glob.requested_speed = 0;
		
	glob.requested_speed = 6; // for debugging
	
	// linear acceleration
	if (glob.actual_speed > glob.requested_speed) glob.actual_speed--;
	if (glob.actual_speed < glob.requested_speed) glob.actual_speed++;	
	
}

static gboolean on_timer_event(GtkWidget *widget)
{
	static int timer_counter = 0;
	
	timer_counter++;
	
    do_logic();
	gtk_widget_queue_draw(widget);
	return TRUE;

}

static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	// event-button = 1: means left mouse button; button = 3 means right mouse button    
    // printf("on_button_release_event called, button %d, x = %d, y= %d\n", (int)event->button, (int)event->x, (int)event->y);
	int x = (int)((double)event->x / glob.scale);
    int y = (int)((double)event->y / glob.scale);

/*
    if (event->button == 1) {
		if ((x >= buttonx[1]) && (x <= buttonx[1] + BUTTONXSIZE) &&
			(y >= buttony[1]) && (y <= buttony[1] + BUTTONYSIZE)) {
				glob.buttonstate[1] = 0;
				do_logic();
				if (glob.argAudio) system("/usr/bin/mpg321 -q sound/switch.mp3 2> /dev/null &");
				gtk_widget_queue_draw(widget);
				return TRUE;
		}
		if ((x >= buttonx[4]) && (x <= buttonx[4] + BUTTONXSIZE) &&
			(y >= buttony[4]) && (y <= buttony[4] + BUTTONYSIZE)) {
				glob.buttonstate[4] = 0;
				do_logic();
				if (glob.argAudio) system("/usr/bin/mpg321 -q sound/switch.mp3 2> /dev/null &");
				gtk_widget_queue_draw(widget);
				return TRUE;
		}
	}
*/
}

static gboolean on_button_click_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	// event-button = 1: means left mouse button; button = 3 means right mouse button    
    // printf("on_button_click_event called, button %d, x = %d, y= %d\n", (int)event->button, (int)event->x, (int)event->y);
    
    int x = (int)((double)event->x / glob.scale);
    int y = (int)((double)event->y / glob.scale);
/*    if (event->button == 1) {
		for (int i = 0; i < 6; i++) {
			if ((x >= buttonx[i]) && (x <= buttonx[i] + BUTTONXSIZE) &&
				(y >= buttony[i]) && (y <= buttony[i] + BUTTONYSIZE)) {
					
				if (y <= buttony[i] + (BUTTONYSIZE / 2)) {
					if (glob.argAudio) system("/usr/bin/mpg321 -q sound/switch.mp3 2> /dev/null &");
					if ((i == 0) || (i == 3))	// 2 state buttons
						glob.buttonstate[i] = 1;
					else if (glob.buttonstate[i] != 0)  // 3 state button
						glob.buttonstate[i] = 0;
					else
						glob.buttonstate[i] = 1;
						
				}
				else {
					if (glob.argAudio) system("/usr/bin/mpg321 -q sound/switch.mp3 2> /dev/null &");
					if ((i == 0) || (i == 3))	// 2 state buttons
					glob.buttonstate[i] = 2;
					else if (glob.buttonstate[i] != 0)  // 3 state button
						glob.buttonstate[i] = 0;
					else
						glob.buttonstate[i] = 2;
				}
				do_logic();
				gtk_widget_queue_draw(widget);
				// printf("button state %d: %d\n",i,glob.buttonstate[i]);
				return TRUE;
			} 
		}
	}
*/
    return TRUE;
}

static void on_quit_event()
{
	system("pkill mpg321");
	gtk_main_quit();
    exit(0);
}

static void on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    int ch;
    // printf("key pressed, state =%04X, keyval =%04X\n", event->state, event->keyval);
    if ((event->state == 0x4) && (event->keyval == 0x0063)) // ctrl-c
		on_quit_event();
	else if ((event->state == 0x4) && (event->keyval == 0x0071)) // ctrl-q
		on_quit_event();
	else if ((event->state == 0x0) && (event->keyval == 0xFF1B)) // esc
		on_quit_event();	
}        

cairo_surface_t* readpng(char* s)
{
	cairo_surface_t *t = cairo_image_surface_create_from_png(s);
	if ((t == 0) || cairo_surface_status(t)) {
		printf("Cannot load %s\n",s);
		exit(1);
	}
	return t;
}

int main(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *darea;
  
  glob.argFullscreen = 0;
  glob.argAudio = 0;
  int firstArg = 1;
  
  char s[32];
  
  printf("tu77 version 0.1\n");
  
  system("pkill mpg321");
  
  while (firstArg < argc) {
	if (strcmp(argv[firstArg],"-full") == 0)
		glob.argFullscreen = 1;
	else if (strcmp(argv[firstArg],"-audio") == 0)
		glob.argAudio = 1;            
    else {
        printf("tu77: unknown argument %s\n", argv[firstArg]);
        exit(1);
    }
    firstArg++;
  }
  
  glob.requested_speed = 0;
  glob.actual_speed = 0,
  glob.last_remote_status = 0;
  glob.angle = 0.0;
  d_mSeconds(); // initialize delta timer
  
  glob.image     = readpng("Te16-open.png");
  for (int i = 0; i < NUMANGLES; i++) {
	sprintf(s,"Reel1-0%d.png",i);
	glob.reel1[i] = readpng(s);
	sprintf(s,"Reel1-0%dbl.png",i);
	glob.reel1bl[i] = readpng(s);
  }

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  // set the background color
  GdkColor color;
  color.red   = 0;
  color.green = 0;
  color.blue  = 0;
  gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &color);

  darea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER (window), darea);

  g_signal_connect(G_OBJECT(darea), "draw", 
      G_CALLBACK(on_draw_event), NULL); 
  g_signal_connect(window, "destroy",
      G_CALLBACK (gtk_main_quit), NULL);

  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  
  GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
  int screenWidth = gdk_screen_get_width(screen);
  int screenHeight = gdk_screen_get_height(screen);
  printf("Screen dimensions: %d x %d\n", screenWidth, screenHeight);
        
  if (glob.argFullscreen) {        
    // DISPLAY UNDECORATED FULL SCREEN WINDOW
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_fullscreen(GTK_WINDOW(window));
	gtk_window_set_keep_above(GTK_WINDOW(window), FALSE);
	glob.scale = screenHeight / 1080.0;
  }
 
  else {
    // DISPLAY DECORATED WINDOW
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(window), 530, 540);
    glob.scale = 0.5;                
  } 
  
  gtk_window_set_title(GTK_WINDOW(window), "tu77");
  
  g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(on_button_click_event), NULL);
  g_signal_connect(G_OBJECT(window), "button-release-event", G_CALLBACK(on_button_release_event), NULL);
  g_signal_connect(G_OBJECT(window), "key_press_event", G_CALLBACK(on_key_press), NULL);
  
  if (TIME_INTERVAL > 0) {
		// Add timer event
		// Register the timer and set time in mS.
		// The timer_event() function is called repeatedly until it returns FALSE. 
		g_timeout_add(TIME_INTERVAL, (GSourceFunc) on_timer_event, (gpointer) window);
	}

  gtk_widget_show_all(window);

  gtk_main();

  cairo_surface_destroy(glob.image);

  return 0;
}
