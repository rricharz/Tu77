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

#define TIME_INTERVAL	40		// timer interval in msec

#define REEL1X			370
#define REEL1Y			98
#define REEL2X			365
#define REEL2Y			568
#define VC1X			150		// vacuum column 1 x position in dots
#define VC1Y			450		// vacuum column 1 y position in dots
#define VC1R			36		// valuum column 1 radius in dots
#define VC2X			295		// vacuum column 1 x position in dots
#define VC2Y			750		// vacuum column 1 y position in dots
#define VC2R			36		// valuum column 1 radius in dots

#define NUMANGLES		10		// number of angles simulated

#define MIN_TRADIUS		100		// tape radius in dots
#define MAX_TRADIUS		190		// tape radiusin dots
#define FULL_RPS		4.5		// in turns per second
#define MAX_DVC1		350		// maximal delta vacuum column 1 in dots
#define MAX_DVC2		350		// maximal delta vacuum column 2 in dots
#define SCALE_VC		4.0		// scaling for vacuum column


struct {
  cairo_surface_t *image;
  cairo_surface_t *reel1[NUMANGLES], *reel1bl[NUMANGLES];
  double scale;
  double delta_t;
  int remote_status, last_remote_status;
  int argFullscreen, argAudio;
  double requested_speed1, actual_speed1, requested_speed2, actual_speed2;
  double angle1, angle2;
  double radius1, radius2;
  double delta_vc1, delta_vc2;
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
	double delta_angle1 = glob.actual_speed1 * glob.delta_t * 0.25 * MAX_TRADIUS / glob.radius1;
	double delta_angle2 = glob.actual_speed2 * glob.delta_t * 0.25 * MAX_TRADIUS / glob.radius2;
	glob.angle1 = (double)((long)(glob.angle1 + delta_angle1) % 360);
	glob.angle2 = (double)((long)(glob.angle2 + delta_angle2) % 360);
	int index1 = glob.angle1 * NUMANGLES / 360;
	int index2 = glob.angle2 * NUMANGLES / 360;
	
	cairo_scale(cr,glob.scale,glob.scale);
		
	printf("dt=%0.0f, da1=%0.0f, da2=%0.0f\n",
		glob.delta_t, delta_angle1, delta_angle2);
		
	cairo_set_source_surface(cr, glob.image, 0, 0);
	cairo_paint(cr);
	
	if (glob.actual_speed1 != 0) {
	
		cairo_set_source_surface(cr, glob.reel1bl[index1], REEL1X, REEL1Y);
		cairo_paint(cr);
	
		cairo_set_source_surface(cr, glob.reel1bl[index1], REEL2X, REEL2Y);
		cairo_paint(cr);
	}
	else {
		
		cairo_set_source_surface(cr, glob.reel1[index1], REEL1X, REEL1Y);
		cairo_paint(cr);
	
		cairo_set_source_surface(cr, glob.reel1[index2], REEL2X, REEL2Y);
		cairo_paint(cr);
	}
	
	// draw the tape on the reels
		
	int w = cairo_image_surface_get_width(glob.reel1[0]);
	int h = cairo_image_surface_get_height(glob.reel1[0]);
	
	cairo_set_source_rgba(cr, 0.2, 0.1, 0.0, 0.3);
	
	int lw = glob.radius1 - MIN_TRADIUS;
	cairo_set_line_width(cr, lw);
	cairo_arc(cr, REEL1X  + w / 2, REEL1Y + h / 2, glob.radius1 - (lw / 2), 0.0, 2.0 * M_PI);
	cairo_stroke(cr);
	
	lw = glob.radius2 - MIN_TRADIUS;
	// printf("w = %d, radius2=%d, min=%d, lw=%d\n", w, glob.radius2, MIN_TRADIUS, lw);
	cairo_set_line_width(cr, lw);
	cairo_arc(cr, REEL2X  + w / 2, REEL2Y + h / 2, glob.radius2 - (lw / 2), 0.0, 2.0 * M_PI);
	cairo_stroke(cr);
	
	// draw the tape in the vacuum columns
	
	cairo_set_source_rgba(cr, 0.2, 0.1, 0.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_arc(cr, VC1X, VC1Y + glob.delta_vc1, VC1R, 0.0, M_PI);
	cairo_stroke(cr);
	cairo_arc(cr, VC2X, VC2Y + glob.delta_vc2, VC2R, 0.0, M_PI);
	cairo_stroke(cr);	
}

static void do_logic()
// logic and feedback circuit
{	

	glob.remote_status = getStatus();
	glob.delta_t = (double)d_mSeconds();
	
	if (glob.last_remote_status != glob.remote_status)
		printf("********** Remote state=0x%02x\n", glob.remote_status);
		
	// glob.remote_status = TSTATE_READ; // for debugging
		
	if ((glob.remote_status & TSTATE_WRITE) || (glob.remote_status & TSTATE_READ) || (glob.remote_status & TSTATE_SEEK)) {
		glob.requested_speed1 = (int)(FULL_RPS * MAX_TRADIUS / glob.radius1 + 0.5); // rps
		glob.requested_speed2 = (int)(FULL_RPS * MAX_TRADIUS / glob.radius2 + 0.5); 
	}
	else {
		glob.requested_speed1 = 0;
		glob.requested_speed2 = 0;
	}
	
	// Calculate the actual vacuum column deltas, based on speed differences
	
	glob.delta_vc1 -= SCALE_VC * (glob.requested_speed1 - glob.actual_speed1) * glob.delta_t / TIME_INTERVAL;
		
	//if (glob.delta_vc1 > 5.0) glob.delta_vc1 -= 5.0;
	//if (glob.delta_vc1 < -5.0) glob.delta_vc1 += 5.0;
	if (glob.delta_vc1 > MAX_DVC1) glob.delta_vc1 = MAX_DVC1;
	if (glob.delta_vc1 < -MAX_DVC1) glob.delta_vc1 = -MAX_DVC1;	
	if ((glob.actual_speed1 == 0) && (glob.requested_speed1 == 0)) glob.delta_vc1 = 0.0;
	
	glob.delta_vc2 += SCALE_VC * (glob.requested_speed2 - glob.actual_speed2) * glob.delta_t / TIME_INTERVAL;
		
	//if (glob.delta_vc2 > 5.0) glob.delta_vc2 -= 5.0;
	//if (glob.delta_vc2 < -5.0) glob.delta_vc2 += 5.0;
	if (glob.delta_vc2 > MAX_DVC2) glob.delta_vc2 = MAX_DVC2;
	if (glob.delta_vc2 < -MAX_DVC2) glob.delta_vc2 = -MAX_DVC2;	
	
	if ((glob.delta_vc1) || (glob.delta_vc1))
		printf("**dvc1=%0.0f, dvc2=%0.0f\n", glob.delta_vc1, glob.delta_vc2);
	
	// Linear acceleration based on speed differences
	// Vacuum column deltas are the integrals of the speed differences
	// Differentiating these again could have been used here,
	// but the same effect can be obtained by using the speed differences directly
	
	if (glob.actual_speed1 > glob.requested_speed1) glob.actual_speed1 -= 0.5;
	if (glob.actual_speed1 < glob.requested_speed1) glob.actual_speed1 += 0.5;	
	if (glob.actual_speed2 > glob.requested_speed2) glob.actual_speed2 -= 0.5;
	if (glob.actual_speed2 < glob.requested_speed2) glob.actual_speed2 += 0.5;	

	glob.last_remote_status = glob.remote_status;
}

static gboolean on_timer_event(GtkWidget *widget)
{
	static int moving = 0;
	
    do_logic();
    if ((glob.actual_speed1 != 0) || (glob.actual_speed2 != 0)) {		
		gtk_widget_queue_draw(widget);
		moving = 1;
	}
	else if (moving) { // draw the reels once more when moving stops
		gtk_widget_queue_draw(widget);
		glob.delta_vc1 = 0.0;
		glob.delta_vc2 = 0.0;
		moving = 0;
	}
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
  
  glob.requested_speed1 = 0;
  glob.actual_speed1 = 0,
  glob.requested_speed2 = 0;
  glob.actual_speed2 = 0,
  glob.last_remote_status = 0;
  glob.angle1 = 0;
  glob.angle2 = 100;
  glob.delta_vc1 = 0;
  glob.delta_vc2 = 0;
  glob.radius1 = MIN_TRADIUS + 20;
  glob.radius2 = MAX_TRADIUS -10;
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
