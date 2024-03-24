// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_EDITABLE_P_H
#define _GTKMM_EDITABLE_P_H


#include <glibmm/private/interface_p.h>

#include <glibmm/private/interface_p.h>

namespace Gtk
{

class Editable_Class : public Glib::Interface_Class
{
public:
  typedef Editable CppObjectType;
  typedef GtkEditable BaseObjectType;
  typedef GtkEditableClass BaseClassType;
  typedef Glib::Interface_Class CppClassParent;

  friend class Editable;

  const Glib::Interface_Class& init();

  static void iface_init_function(void* g_iface, void* iface_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.
  static void insert_text_callback(GtkEditable* self, const gchar* text, gint length, gint* position);
  static void delete_text_callback(GtkEditable* self, gint p0, gint p1);
  static void changed_callback(GtkEditable* self);

  //Callbacks (virtual functions):
  static void do_insert_text_vfunc_callback(GtkEditable* self, const gchar* text, gint length, gint* position);
  static void do_delete_text_vfunc_callback(GtkEditable* self, gint start_pos, gint end_pos);
  static gchar* get_chars_vfunc_callback(GtkEditable* self, gint start_pos, gint end_pos);
  static void set_selection_bounds_vfunc_callback(GtkEditable* self, gint start_pos, gint end_pos);
  static gboolean get_selection_bounds_vfunc_callback(GtkEditable* self, gint* start_pos, gint* end_pos);
  static void set_position_vfunc_callback(GtkEditable* self, gint position);
  static gint get_position_vfunc_callback(GtkEditable* self);
};


} // namespace Gtk


#endif /* _GTKMM_EDITABLE_P_H */

