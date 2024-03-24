// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_TOOLTIPS_H
#define _GTKMM_TOOLTIPS_H

#include <gtkmmconfig.h>

#ifndef GTKMM_DISABLE_DEPRECATED


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* Copyright (C) 1998-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 // This is for including the config header before any code (such as
// the #ifndef GTKMM_DISABLE_DEPRECATED in deprecated classes) is generated:


#include <gdkmm/color.h>
#include <gtkmm/object.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" { typedef struct _GtkTooltipsData GtkTooltipsData; }
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkTooltips GtkTooltips;
typedef struct _GtkTooltipsClass GtkTooltipsClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Gtk
{ class Tooltips_Class; } // namespace Gtk
namespace Gtk
{

class Widget;

/** Tooltips are the messages that appear next to a widget when the mouse
 * pointer is held over it for a short amount of time. They are especially
 * helpful for adding more verbose descriptions of things such as buttons
 * in a toolbar.
 *
 * This widget holds tooltips for other widgets.  You should only need one
 * Tooltip widget for all widgets you wish to add tips to.
 *
 * @deprecated Use the Gtk::Tooltip API instead.
 */

class Tooltips : public Object
{
  public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef Tooltips CppObjectType;
  typedef Tooltips_Class CppClassType;
  typedef GtkTooltips BaseObjectType;
  typedef GtkTooltipsClass BaseClassType;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  virtual ~Tooltips();

#ifndef DOXYGEN_SHOULD_SKIP_THIS

private:
  friend class Tooltips_Class;
  static CppClassType tooltips_class_;

  // noncopyable
  Tooltips(const Tooltips&);
  Tooltips& operator=(const Tooltips&);

protected:
  explicit Tooltips(const Glib::ConstructParams& construct_params);
  explicit Tooltips(GtkTooltips* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  static GType get_type()      G_GNUC_CONST;


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GtkObject.
  GtkTooltips*       gobj()       { return reinterpret_cast<GtkTooltips*>(gobject_); }

  ///Provides access to the underlying C GtkObject.
  const GtkTooltips* gobj() const { return reinterpret_cast<GtkTooltips*>(gobject_); }


public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


private:

  
public:
  Tooltips();

  
  void enable();
  
  void disable();

  
  /** Adds a tooltip containing the message @a tip_text to the specified Gtk::Widget.
   * Deprecated: 2.12:
   * 
   * @param widget The Gtk::Widget you wish to associate the tip with.
   * @param tip_text A string containing the tip itself.
   * @param tip_private A string of any further information that may be useful if the user gets stuck.
   */
  void set_tip(Widget& widget, const Glib::ustring& tip_text, const Glib::ustring& tip_private);
  void set_tip(Widget& widget, const Glib::ustring& tip_text);
  void unset_tip(Widget& widget);

protected:
  
  static GtkTooltipsData* data_get(Widget& widget);

public:
  
  void force_window();


};

} //namespace Gtk


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gtk::Tooltips
   */
  Gtk::Tooltips* wrap(GtkTooltips* object, bool take_copy = false);
} //namespace Glib


#endif // GTKMM_DISABLE_DEPRECATED


#endif /* _GTKMM_TOOLTIPS_H */

