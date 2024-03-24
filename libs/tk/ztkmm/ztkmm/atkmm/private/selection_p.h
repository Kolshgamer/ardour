// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _ATKMM_SELECTION_P_H
#define _ATKMM_SELECTION_P_H


#include <atk/atkobject.h>

#include <glibmm/private/interface_p.h>

namespace Atk
{

class Selection_Class : public Glib::Interface_Class
{
public:
  typedef Selection CppObjectType;
  typedef AtkSelection BaseObjectType;
  typedef AtkSelectionIface BaseClassType;
  typedef Glib::Interface_Class CppClassParent;

  friend class Selection;

  const Glib::Interface_Class& init();

  static void iface_init_function(void* g_iface, void* iface_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.
  static void selection_changed_callback(AtkSelection* self);

  //Callbacks (virtual functions):
  static gboolean add_selection_vfunc_callback(AtkSelection* self, gint i);
  static gboolean clear_selection_vfunc_callback(AtkSelection* self);
  static AtkObject* ref_selection_vfunc_callback(AtkSelection* self, gint i);
  static gint get_selection_count_vfunc_callback(AtkSelection* self);
  static gboolean is_child_selected_vfunc_callback(AtkSelection* self, gint i);
  static gboolean remove_selection_vfunc_callback(AtkSelection* self, gint i);
  static gboolean select_all_selection_vfunc_callback(AtkSelection* self);
};


} // namespace Atk


#endif /* _ATKMM_SELECTION_P_H */

