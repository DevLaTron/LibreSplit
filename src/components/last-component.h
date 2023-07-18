#ifndef __LAST_COMPONENT_H__
#define __LAST_COMPONENT_H__

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "../last.h"

typedef struct _LASTComponent LASTComponent;
typedef struct _LASTComponentOps LASTComponentOps;
typedef struct _LASTComponentAvailable LASTComponentAvailable;

struct _LASTComponent
{
    LASTComponentOps *ops;
};

struct _LASTComponentOps
{
    void (*delete)(LASTComponent *self);
    GtkWidget *(*widget)(LASTComponent *self);

    void (*resize)(LASTComponent *self, int win_width, int win_height);
    void (*show_game)(LASTComponent *self, last_game *game, last_timer *timer);
    void (*clear_game)(LASTComponent *self);
    void (*draw)(LASTComponent *self, last_game *game, last_timer *timer);

    void (*start_split)(LASTComponent *self, last_timer *timer);
    void (*skip)(LASTComponent *self, last_timer *timer);
    void (*unsplit)(LASTComponent *self, last_timer *timer);
    void (*stop_reset)(LASTComponent *self, last_timer *timer);
    void (*cancel_run)(LASTComponent *self, last_timer *timer);
};

struct _LASTComponentAvailable
{
    char *name;
    LASTComponent *(*new)();
};

// A NULL-terminated array of all available components
extern LASTComponentAvailable last_components[];

// Utility functions
void add_class(GtkWidget *widget, const char *class);

void remove_class(GtkWidget *widget, const char *class);

#endif /* __LAST_COMPONENT_H__ */