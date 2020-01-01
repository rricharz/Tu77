/*
 * tu77.c
 *
 * A visual display of the magtape TU77 front panel
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
 
#define BORDER 80       // we need to make an educated guess to make sure that the full vertical
                        // space is used if required
                        // if BORDER is too small, we might end up with a too large window
                        // if BORDER is too large, the decorated window will be smaller than possible
                        // with a reasonable size BORDER, both are acceptable
                        // ideally, one could force the window manager to use a certain aspect ratio

#define TSTATE_ONLINE		1
#define TSTATE_DRIVE1		2
#define TSTATE_BACKWARDS	4
#define TSTATE_SEEK		8
#define TSTATE_READ		16
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

#define TIME_INTERVAL		40			// timer interval in msec

#define REEL1X			(130 + glob.xoffset)
#define REEL1Y			577
#define REEL2X			(150 + glob.xoffset)
#define REEL2Y			43
#define VC1X			(717 + glob.xoffset)	// vacuum column 1 x position in dots
#define VC1Y			600			// vacuum column 1 y position in dots
#define VC1R			39			// valuum column 1 radius in dots
#define VC2X			(807 + glob.xoffset)	// vacuum column 1 x position in dots
#define VC2Y			480			// vacuum column 1 y position in dots
#define VC2R			39			// valuum column 1 radius in dots
#define CAPSTANX		(616 + glob.xoffset)
#define CAPSTANY		836
#define VC1TOPL			850
#define VC1TOPR			850
#define VC2TOPL			163
#define VC2TOPR			163
#define BUTTONX			(194 + glob.xoffset)
#define BUTTONY			539
#define BUTTONSIZE		21
#define BUTTONOFFSET		41
#define NUM_BUTTONS		4
#define LABELW			120
#define LABELH			20
#define LABELP			135
#define LED_ONLINE_X		(267 + glob.xoffset)
#define LED_ONLINE_Y		515
#define LED_POWER_X		(161 + glob.xoffset)
#define LED_POWER_Y		515
#define LED_BOT_X		(208 + glob.xoffset)
#define LED_BOT_Y		515
#define LED_RADIUS		5

#define NUMWHEELS		3
int wheelx[NUMWHEELS] = {666, 718, 590};
int wheely[NUMWHEELS] = {128, 128, 395};		

#define NUMANGLES		10		// number of angles simulated
#define CAPACITY		2000000		// 2 Mbyte for now (need better value)
#define MIN_TRADIUS		100.0		// tape radius in dots
#define MAX_TRADIUS		190.0		// tape radius in dots
#define FULL_RPS		4.44		// in turns per second
#define MAX_DVC1		300		// maximal delta vacuum column 1 in dots
#define MAX_DVC2		300		// maximal delta vacuum column 2 in dots
#define SCALE_VC		2.2		// scaling for vacuum column
#define ACCELERATION		1.0		// adjust accelation 

struct {
  cairo_surface_t *image;
  cairo_surface_t *reel1[NUMANGLES], *reel1bl[NUMANGLES];
  cairo_surface_t *hub[NUMANGLES], *hubb[NUMANGLES];
  cairo_surface_t *capstan, *capstanb[2];
  cairo_surface_t *wheel, *wheelb[2];
  double scale;
  double delta_t;
  int remote_status, last_remote_status;
  int argFullscreen, argFullv, argUnit1;
  double requested_speed1, actual_speed1, requested_speed2, actual_speed2;
  double angle1, angle2;
  double radius1, radius2;
  double delta_vc1, delta_vc2;
  int position;
  double positions_per_msec;
  long tms;
  char *label;
  int xoffset;
  int buttonState[NUM_BUTTONS];
} glob;

long d_mSeconds()
// return time in msec since last call
{
        static int initialized = 0;
        static long lastTime;
        struct timeval tv;
        gettimeofday(&tv,NULL);
        glob.tms = (1000 * tv.tv_sec)  + (tv.tv_usec/1000);
        if (!initialized) lastTime = glob.tms;
        initialized = 1;
        long t1 = glob.tms - lastTime;
        lastTime = glob.tms;     
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
		fscanf(statusFile,"%d",&glob.position);
		fclose(statusFile);
		if (st >= 0)
			return st;
		else
			return 0;
	}
	else return 0;
}

static void do_drawing(cairo_t *);

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{      
	do_drawing(cr);

	return FALSE;
}

static void do_drawing(cairo_t *cr)
{
	static int capstan_index;
	double delta_angle1 = glob.actual_speed1 * glob.delta_t * 0.25 * MAX_TRADIUS / glob.radius1;
	double delta_angle2 = glob.actual_speed2 * glob.delta_t * 0.25 * MAX_TRADIUS / glob.radius2;
	glob.angle1 += delta_angle1;
	while (glob.angle1 >= 360.0) glob.angle1 -= 360.0;
	while (glob.angle1 < 0.0) glob.angle1 += 360.0;
	glob.angle2 += delta_angle2;
	while (glob.angle2 >= 360.0) glob.angle2 -= 360.0;
	while (glob.angle2 < 0.0) glob.angle2 += 360.0;	
	
	cairo_scale(cr,glob.scale,glob.scale);
		
	// printf("dt=%0.0f, da1=%0.0f, da2=%0.0f\n", glob.delta_t, delta_angle1, delta_angle2);

	// draw the drive
	cairo_set_source_surface(cr, glob.image, glob.xoffset, 0);
	cairo_paint(cr);
			
	// draw the capstan and the wheels
	if (glob.requested_speed1 != 0.0)
		cairo_set_source_surface(cr, glob.capstanb[capstan_index], CAPSTANX, CAPSTANY);
	else	
		cairo_set_source_surface(cr, glob.capstan, CAPSTANX, CAPSTANY);
	cairo_paint(cr);
	if (glob.requested_speed1 != 0.0) {
		for (int i = 0; i < NUMWHEELS; i++) {
			cairo_set_source_surface(cr, glob.wheelb[capstan_index],
				wheelx[i] + glob.xoffset, wheely[i]);
			cairo_paint(cr);
		}
	}
	else {	
		for (int i = 0; i < NUMWHEELS; i++) {
			cairo_set_source_surface(cr, glob.wheel,
				wheelx[i] + glob.xoffset, wheely[i]);
			cairo_paint(cr);
		}
	}
	capstan_index = (capstan_index) + 1 & 1;
	
	
	// draw the reels
	int index1 = glob.angle1 * NUMANGLES / 360;
	int index2 = glob.angle2 * NUMANGLES / 360;
	if (glob.actual_speed1 != 0) {	
		cairo_set_source_surface(cr, glob.reel1bl[index1], REEL1X, REEL1Y);
		cairo_paint(cr);
	}
	else {		
		cairo_set_source_surface(cr, glob.reel1[index1], REEL1X, REEL1Y);
		cairo_paint(cr);
	}
	if (glob.actual_speed2 != 0) {	
		cairo_set_source_surface(cr, glob.reel1bl[index2], REEL2X, REEL2Y);
		cairo_paint(cr);
	}
	else {
		cairo_set_source_surface(cr, glob.reel1[index2], REEL2X, REEL2Y);
		cairo_paint(cr);
	}
	
	// draw the hub
	
	if (glob.actual_speed2 != 0) {	
		cairo_set_source_surface(cr, glob.hubb[index2], REEL2X + 104, REEL2Y + 104);
		cairo_paint(cr);
	}
	else {
		cairo_set_source_surface(cr, glob.hub[index2], REEL2X + 104, REEL2Y + 104);
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
	cairo_set_line_width(cr, 2);
	cairo_arc(cr, VC1X, VC1Y - glob.delta_vc1, VC1R, 1.1 * M_PI, 1.9 * M_PI);
	cairo_stroke(cr);
	cairo_arc(cr, VC2X, VC2Y + glob.delta_vc2, VC2R, 0.1 * M_PI, 0.9 * M_PI);
	cairo_stroke(cr);
	
		// draw the red leds
	
	cairo_set_source_rgb(cr, 1.0, 0.3, 0.3);
	cairo_set_line_width(cr, 1);
	cairo_arc(cr, LED_POWER_X, LED_POWER_Y, LED_RADIUS, 0.0, 2.0 * M_PI);
	cairo_fill(cr);
	
	if (glob.buttonState[1]) {
		cairo_arc(cr, LED_ONLINE_X, LED_ONLINE_Y, LED_RADIUS, 0.0, 2.0 * M_PI);
		cairo_fill(cr);
	}
	if (glob.position == 0) {
		cairo_arc(cr, LED_BOT_X, LED_BOT_Y, LED_RADIUS, 0.0, 2.0 * M_PI);
		cairo_fill(cr);
	}	
	
	// draw a label onto the removable reel

	cairo_text_extents_t extent;
	
	if (glob.label[0] != 0) {

		if (glob.actual_speed2 != 0) {
			lw = LABELH * 1.2;
			cairo_set_line_width(cr, lw);
			cairo_set_source_rgba(cr, 0.3, 0.3, 0.8,0.08);
			cairo_arc(cr, REEL2X  + w / 2, REEL2Y + h / 2, LABELP - LABELH / 2.0,
				(-80.0 +index2 * 36.0) * M_PI / 180.0,
				(80.0 + index2 * 36.0) * M_PI / 180.0);
			cairo_set_source_rgba(cr, 0.3, 0.3, 0.8,0.15);
			cairo_stroke(cr);
			cairo_arc(cr, REEL2X  + w / 2, REEL2Y + h / 2, LABELP - LABELH / 2.0,
				(-50.0 +index2 * 36.0) * M_PI / 180.0,
				(50.0 + index2 * 36.0) * M_PI / 180.0);
			cairo_stroke(cr);
		}
		else {	
			cairo_set_source_rgb(cr, 0.3, 0.3, 0.8);
			cairo_set_line_width (cr, 2);
			cairo_translate(cr, REEL2X  + w / 2 , REEL2Y + h / 2);
			cairo_rotate(cr, (index2 * 36.0) * M_PI / 180.0);
			cairo_rectangle (cr,  -LABELW / 2 , - LABELP, LABELW, LABELH);
			cairo_stroke_preserve(cr);
			cairo_fill(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_select_font_face(cr, "Purisa", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size(cr, 12);
			cairo_text_extents(cr, glob.label, &extent);
			// printf("extent: %0.0f,%0.0f\n", extent.width, extent.height);
			cairo_move_to(cr, - extent.width / 2.0, - LABELP + (LABELH + extent.height) / 2.0);
			cairo_show_text(cr, glob.label);
			cairo_stroke (cr);
		}
	}	
}

static void do_logic()
// logic and feedback circuit
{	
	int lastPosition = glob.position;
	if (glob.buttonState[1])
		glob.remote_status = getStatus();
	else
		glob.remote_status = 0;
	
	if (glob.argUnit1 && ((glob.remote_status & TSTATE_DRIVE1) == 0)) {
		glob.remote_status = 0;
	 }
	if ((!glob.argUnit1) && ((glob.remote_status & TSTATE_DRIVE1) != 0)) {
		glob.remote_status = 0;
	}		
	
	glob.delta_t = (double)d_mSeconds();
	
	if ((glob.last_remote_status != glob.remote_status) || (glob.position != lastPosition)) {
		/* printf("*** SimH driver state=0x%02x(%c%c%c%c%c%c), target pos=%d\n",
			glob.remote_status,
			((glob.remote_status & TSTATE_ONLINE)? 'O':'-'),
			((glob.remote_status & TSTATE_WRITE)? 'W':'-'),
			((glob.remote_status & TSTATE_READ)? 'R':'-'),
			((glob.remote_status & TSTATE_SEEK)? 'S':'-'),
			((glob.remote_status & TSTATE_BACKWARDS)? '<':'>'),
			((glob.remote_status & TSTATE_DRIVE1)? '2':'1'),
			glob.position); */
		double dtime = (glob.position - lastPosition) * 0.1;
		if (dtime < 0.0) dtime = -dtime;
		dtime += 200.0;
		if (dtime > 20000.0) dtime = 20000.0;
		if (dtime == 0.0) glob.positions_per_msec;
		else glob.positions_per_msec = (glob.position - lastPosition) / dtime; 
	}
	if (glob.remote_status & TSTATE_SEEK) {
		int t = glob.position;
		glob.position = lastPosition;
		glob.position += glob.positions_per_msec * glob.delta_t;
		// printf("SEEK/REWIND, target position = %d, current position=%d\n", t, glob.position);
	}
		
	// calculate current tape radius for both reels
	// note: this is NOT a linear relation!
	
	if (glob.position < 0) glob.position = 0;
	if (glob.position > CAPACITY) glob.position = CAPACITY;
	double f0square = (MIN_TRADIUS / MAX_TRADIUS) * (MIN_TRADIUS / MAX_TRADIUS);
	double f1 = sqrt(((double)glob.position / CAPACITY) * (1.0 - f0square) + f0square);
	double f2 = sqrt(((CAPACITY - (double)glob.position) / CAPACITY) * (1.0 - f0square) + f0square);	

	glob.radius1 = f1 * MAX_TRADIUS;
	glob.radius2 = f2 * MAX_TRADIUS;
	
	// calculate requested reel speeds based on position
	
	if ((glob.remote_status & TSTATE_WRITE) || (glob.remote_status & TSTATE_READ) || (glob.remote_status & TSTATE_SEEK)) {
		glob.requested_speed1 = (int)(FULL_RPS * MAX_TRADIUS / glob.radius1 + 0.5);
		glob.requested_speed2 = (int)(FULL_RPS * MAX_TRADIUS / glob.radius2 + 0.5); 
	}
	else {
		glob.requested_speed1 = 0;
		glob.requested_speed2 = 0;
	}
	if (glob.remote_status & TSTATE_BACKWARDS) {
		glob.requested_speed1 = -glob.requested_speed1;
		glob.requested_speed2 = -glob.requested_speed2;
	}
	
	// Calculate the actual vacuum column deltas, based on speed differences
	
	glob.delta_vc1 += SCALE_VC * (glob.requested_speed1 - glob.actual_speed1) * glob.delta_t / TIME_INTERVAL;
	if (fabs(glob.requested_speed1 - glob.actual_speed1) < ACCELERATION) // move towards center
		glob.delta_vc1 *= 0.9;
	if (glob.actual_speed1 != 0.0) glob.delta_vc1 += (rand() & 7) - 4; // sligh jitter
	if (glob.delta_vc1 > MAX_DVC1) glob.delta_vc1 = MAX_DVC1;
	if (glob.delta_vc1 < -MAX_DVC1) glob.delta_vc1 = -MAX_DVC1;	
	
	glob.delta_vc2 -= SCALE_VC * (glob.requested_speed2 - glob.actual_speed2) * glob.delta_t / TIME_INTERVAL;		
	if (fabs(glob.requested_speed1 - glob.actual_speed1) < ACCELERATION) // move towards center
		glob.delta_vc2 *= 0.9;
	if (glob.actual_speed2 != 0.0) glob.delta_vc2 += (rand() & 7) - 4; // sligh jitter
	if (glob.delta_vc2 > MAX_DVC2) glob.delta_vc2 = MAX_DVC2;
	if (glob.delta_vc2 < -MAX_DVC2) glob.delta_vc2 = -MAX_DVC2;
	
	// if ((glob.delta_vc1) || (glob.delta_vc1))
	//	printf("**dvc1=%0.0f, dvc2=%0.0f\n", glob.delta_vc1, glob.delta_vc2);
	
	// Linear acceleration based on speed differences
	// Vacuum column deltas are the integrals of the speed differences
	// Differentiating these again could have been used here,
	// but the same effect can be obtained by using the speed differences directly
	
	if (glob.actual_speed1 > glob.requested_speed1) glob.actual_speed1 -= ACCELERATION;
	if (glob.actual_speed1 < glob.requested_speed1) glob.actual_speed1 += ACCELERATION;	
	if (glob.actual_speed2 > glob.requested_speed2) glob.actual_speed2 -= ACCELERATION;
	if (glob.actual_speed2 < glob.requested_speed2) glob.actual_speed2 += ACCELERATION;
	
	// make sure that the reels stop completely
	
	if (fabs(glob.actual_speed1) < ACCELERATION) glob.actual_speed1 = 0.0;
	if (fabs(glob.actual_speed2) < ACCELERATION) glob.actual_speed2 = 0.0;	

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
		moving = 0;
	}
	return TRUE;

}

static gboolean on_button_click_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	// event-button = 1: means left mouse button; button = 3 means right mouse button    
	// printf("on_button_click_event called, button %d, x = %d, y= %d\n", (int)event->button, (int)event->x, (int)event->y);
    
	int x = (int)((double)event->x / glob.scale);
	int y = (int)((double)event->y / glob.scale);
	if (event->button == 1) {
		for (int i = 0; i < NUM_BUTTONS; i++) {
			if ((x >= BUTTONX + i * BUTTONOFFSET)
					&& (x <= BUTTONX + i * BUTTONOFFSET + BUTTONSIZE)
					&& (y >= BUTTONY) && (y <= BUTTONY + BUTTONSIZE)) {
				glob.buttonState[i] = !glob.buttonState[i];
				do_logic();
				gtk_widget_queue_draw(widget);
				// printf("button state %d = %d\n",i, glob.buttonState[i]);
				return TRUE;
			} 
		}
	}
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
	
	// initialize random number generator (rand)
	srand((unsigned)time(NULL));	
  
	glob.argFullscreen = 0;
	glob.argFullv = 0;
	glob.argUnit1 = 0;
	int firstArg = 1;
	
	glob.label = "";
  
	char s[32];
  
	printf("tu77 version 0.4\n");
  
	while (firstArg < argc) {
		if (strcmp(argv[firstArg],"-full") == 0)
			glob.argFullscreen = 1;
		else if (strcmp(argv[firstArg],"-fullv") == 0)
			glob.argFullv = 1;  
		else if (strcmp(argv[firstArg],"-unit1") == 0)
			glob.argUnit1 = 1; 
		else if (strcmp(argv[firstArg],"-label") == 0) {
			if (firstArg + 1 < argc) {
				glob.label = argv[firstArg++ + 1];
			}	
		}           
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
	glob.radius1 = MIN_TRADIUS;
	glob.radius2 = MAX_TRADIUS;
	glob.position = 0;
	d_mSeconds(); // initialize delta timer
  
	glob.image     = readpng("Tu77-open.png");	
	int image_width = cairo_image_surface_get_width(glob.image);
	int image_height = cairo_image_surface_get_height(glob.image);
	for (int i = 0; i < NUMANGLES; i++) {
		sprintf(s,"reels/Reel1-0%d.png",i);
		glob.reel1[i] = readpng(s);
		sprintf(s,"reels/Reel1-0%dbl.png",i);
		glob.reel1bl[i] = readpng(s);
		sprintf(s,"reels/hub%d.png",i);
		glob.hub[i] = readpng(s);
		sprintf(s,"reels/hub%db.png",i);
		glob.hubb[i] = readpng(s);
	}
	glob.capstan   = readpng("reels/capstan2.png");
	glob.capstanb[0] = readpng("reels/capstan2b1.png");
	glob.capstanb[1] = readpng("reels/capstan2b2.png");
	glob.wheel   = readpng("reels/wheel.png");
	glob.wheelb[0] = readpng("reels/wheelb1.png");
	glob.wheelb[1] = readpng("reels/wheelb2.png");
	
	glob.buttonState[0] = 0;
	glob.buttonState[1] = 1;
	glob.buttonState[2] = 0;
	glob.buttonState[3] = 0;

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
		glob.scale = (double) screenHeight / (double)image_height;
		glob.xoffset = (int)((double)(screenWidth - image_width) / (2.0 * glob.scale));
	}
	else if (glob.argFullv) {
		gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
		int h = screenHeight - BORDER;
		int w = (int)((double)h * (double)image_width / (double)image_height);
		gtk_window_set_default_size(GTK_WINDOW(window), w, h);
		glob.scale = (double)w / image_width;
		glob.xoffset = 0;  		
	}
 
	else {
		// DISPLAY DECORATED WINDOW
		gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
		gtk_window_set_default_size(GTK_WINDOW(window), image_width / 2, image_height / 2);
		glob.scale = 0.5;
		glob.xoffset = 0;                
	} 
  
	gtk_window_set_title(GTK_WINDOW(window), "tu77");
  
	g_signal_connect(G_OBJECT(window), "button-press-event", G_CALLBACK(on_button_click_event), NULL);
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
