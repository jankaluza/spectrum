#ifndef _HI_EVENTLOOP_H
#define _HI_EVENTLOOP_H

#include <glib.h>
#include "eventloop.h"

#define READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

PurpleEventLoopUiOps * getEventLoopUiOps(void);

#endif
