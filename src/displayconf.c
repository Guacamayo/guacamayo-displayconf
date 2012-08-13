/*
 * Copyright © 2007, 2008, Red Hat, Inc.
 * Copyright © 2010, 2011 Intel Corporation.
 * Copyright © 2012, sleep(5) ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

/*
 * Display configuration applet for MEX; based on the original Mex applet, but
 * without the gnome dependency.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

int
size_sorter (gconstpointer a, gconstpointer b)
{
  const XRRModeInfo *m_a = a, *m_b = b;

  return (m_b->height - 720) < (m_a->height - 720);
}

/*
 * From libgnome-desktop
 */
static gboolean
output_is_laptop (const char *name)
{
  if (!name)
    return FALSE;

  if (strstr (name, "lvds") ||  /* Most drivers use an "LVDS" prefix... */
      strstr (name, "LVDS") ||
      strstr (name, "Lvds") ||
      strstr (name, "LCD")  ||  /* ... but fglrx uses "LCD" in some versions. */
      strstr (name, "eDP")  ||  /* eDP is for internal panel connections */
      strstr (name, "default")) /* Finally, NVidia and others */
    return TRUE;

  return FALSE;
}

static XRROutputInfo *
pick_output (Display *xdpy, XRRScreenResources *screen_res)
{
  XRROutputInfo *laptop_output   = NULL;
  XRROutputInfo *external_output = NULL;
  int            i;

  for (i = 0; i < screen_res->noutput; i++)
    {
      XRROutputInfo *oi;
      RROutput       o = screen_res->outputs[i];

      oi = XRRGetOutputInfo (xdpy, screen_res, o);

      if (oi->connection != RR_Connected)
        {
          XRRFreeOutputInfo (oi);
          continue;
        }

      if (output_is_laptop (oi->name))
        {
          if (laptop_output)
            XRRFreeOutputInfo (laptop_output);

          laptop_output = oi;
        }
      else
        {
          if (external_output)
            XRRFreeOutputInfo (external_output);

          external_output = oi;
        }
    }

  g_assert (laptop_output || external_output);

  return external_output ? external_output : laptop_output;
}

int
main (int argc, char **argv)
{
  Display            *xdpy;
  XRRScreenResources *screen_res = NULL;
  XRROutputInfo      *output     = NULL;
  XRRCrtcInfo        *ci         = NULL;
  RRMode              mode;
  GList              *modes      = NULL;
  int                 i, j;
  int                 retval = 1;

  xdpy = XOpenDisplay (g_getenv("DISPLAY"));
  g_assert (xdpy);

  screen_res = XRRGetScreenResourcesCurrent (xdpy, DefaultRootWindow (xdpy));

  output = pick_output (xdpy, screen_res);

  for (i = 0; i < screen_res->nmode; i++)
    {
      int          ratio;
      gboolean     widescreen = FALSE;
      gboolean     valid      = FALSE;
      XRRModeInfo *mi         = &screen_res->modes[i];

      /* Skip modes with a height less than 720 straight away */
      if (mi->width < 720)
        continue;

      /*
       * Check whether this mode is available on our selected output
       */
      for (j = 0; j < output->nmode; ++j)
        {
          if (output->modes[i] == mi->id)
            {
              valid = TRUE;
              break;
            }
        }

      if (!valid)
        continue;

    /*
     * Build a list of all modes that are widescreen.
     *
     * Multiply by ten and truncate into an integer to avoid annoying float
     * comparisons.
     */
    ratio = ((double)mi->width / mi->height) * 10;
    switch (ratio)
      {
      default:
        g_debug ("Unknown ratio for %d x %d", mi->width, mi->height);
      case 13: /* 1.33, or 4:3 */
        widescreen = FALSE;
        break;
      case 16: /* 1.6, or 16:10 */
      case 17: /* 1.777, or 16;9 */
      case 23: /* 2.333, or 21:9 */
        g_debug ("Found widescreen ratio %d", ratio);
        widescreen = TRUE;
        break;
      }

    if (widescreen)
      modes = g_list_prepend (modes, mi);
  }

  if (!modes)
    {
      XRRFreeOutputInfo (output);
      g_warning ("No usable modes detected!\n");
      goto done;
    }

  /* Now sort it so they are sorted in order of distance from 720 */
  modes = g_list_sort (modes, size_sorter);
  mode = ((XRRModeInfo*)(modes->data))->id;

  g_debug ("Chose %d x %d",
           ((XRRModeInfo*)(modes->data))->width,
           ((XRRModeInfo*)(modes->data))->height);

  ci = XRRGetCrtcInfo (xdpy, screen_res, output->crtc);

  if (ci->mode != mode)
    {
      if (Success != XRRSetCrtcConfig (xdpy,
                                       screen_res,
                                       output->crtc,
                                       CurrentTime,
                                       0, 0,
                                       ((XRRModeInfo*)(modes->data))->id,
                                       RR_Rotate_0,
                                       ci->outputs, ci->noutput))
        {
          g_warning ("Failed to apply chosen mode");
        }
      else
        retval = 0;

    }
  else
    {
      retval = 0;
      g_debug ("Already in best mode");
    }

 done:
  if (output)
    XRRFreeOutputInfo (output);

  if (ci)
    XRRFreeCrtcInfo (ci);

  if (screen_res)
    XRRFreeScreenResources (screen_res);

  g_list_free (modes);

  XCloseDisplay (xdpy);

  return retval;
}
