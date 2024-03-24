// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_ACTIONGROUP_H
#define _GTKMM_ACTIONGROUP_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: actiongroup.hg,v 1.17 2006/04/12 11:11:25 murrayc Exp $ */

/* Copyright (C) 2003 The gtkmm Development Team
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

#include <gtkmm/widget.h>
#include <gtkmm/action.h>
#include <gtkmm/accelkey.h>
 

#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GtkActionGroup GtkActionGroup;
typedef struct _GtkActionGroupClass GtkActionGroupClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Gtk
{ class ActionGroup_Class; } // namespace Gtk
#endif //DOXYGEN_SHOULD_SKIP_THIS

namespace Gtk
{
  

class ActionGroup : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef ActionGroup CppObjectType;
  typedef ActionGroup_Class CppClassType;
  typedef GtkActionGroup BaseObjectType;
  typedef GtkActionGroupClass BaseClassType;

private:  friend class ActionGroup_Class;
  static CppClassType actiongroup_class_;

private:
  // noncopyable
  ActionGroup(const ActionGroup&);
  ActionGroup& operator=(const ActionGroup&);

protected:
  explicit ActionGroup(const Glib::ConstructParams& construct_params);
  explicit ActionGroup(GtkActionGroup* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~ActionGroup();

  /** Get the GType for this class, for use with the underlying GObject type system.
   */
  static GType get_type()      G_GNUC_CONST;

#ifndef DOXYGEN_SHOULD_SKIP_THIS


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GtkActionGroup*       gobj()       { return reinterpret_cast<GtkActionGroup*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GtkActionGroup* gobj() const { return reinterpret_cast<GtkActionGroup*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GtkActionGroup* gobj_copy();

private:


protected:
    explicit ActionGroup(const Glib::ustring& name =  Glib::ustring());


public:
  
  static Glib::RefPtr<ActionGroup> create(const Glib::ustring& name =  Glib::ustring());

  
  /** Gets the name of the action group.
   * 
   * @newin{2,4}
   * 
   * @return The name of the action group.
   */
  Glib::ustring get_name() const;

  
  /** Returns <tt>true</tt> if the group is sensitive.  The constituent actions
   * can only be logically sensitive (see is_sensitive()) if
   * they are sensitive (see get_sensitive()) and their group
   * is sensitive.
   * 
   * @newin{2,4}
   * 
   * @return <tt>true</tt> if the group is sensitive.
   */
  bool get_sensitive() const;
  
  /** Changes the sensitivity of @a action_group
   * 
   * @newin{2,4}
   * 
   * @param sensitive New sensitivity.
   */
  void set_sensitive(bool sensitive =  true);
  
  /** Returns <tt>true</tt> if the group is visible.  The constituent actions
   * can only be logically visible (see is_visible()) if
   * they are visible (see get_visible()) and their group
   * is visible.
   * 
   * @newin{2,4}
   * 
   * @return <tt>true</tt> if the group is visible.
   */
  bool get_visible() const;
  
  /** Changes the visible of @a action_group.
   * 
   * @newin{2,4}
   * 
   * @param visible New visiblity.
   */
  void set_visible(bool visible =  true);
                                      
  
  /** Looks up an action in the action group by name.
   * 
   * @newin{2,4}
   * 
   * @param action_name The name of the action.
   * @return The action, or <tt>0</tt> if no action by that name exists.
   */
  Glib::RefPtr<Action> get_action(const Glib::ustring& action_name);
  
  /** Looks up an action in the action group by name.
   * 
   * @newin{2,4}
   * 
   * @param action_name The name of the action.
   * @return The action, or <tt>0</tt> if no action by that name exists.
   */
  Glib::RefPtr<const Action> get_action(const Glib::ustring& action_name) const;

  
  /** Lists the actions in the action group.
   * 
   * @newin{2,4}
   * 
   * @return An allocated list of the action objects in the action group.
   */
  Glib::ListHandle< Glib::RefPtr<Action> > get_actions();
  
  /** Lists the actions in the action group.
   * 
   * @newin{2,4}
   * 
   * @return An allocated list of the action objects in the action group.
   */
  Glib::ListHandle< Glib::RefPtr<const Action> > get_actions() const;
    
  void add(const Glib::RefPtr<Action>& action);
  
  //We want it to always try to use the stock accelerator,
  //so we use gtk_action_group_add_action_with_accel(), instead of gtk_action_group_add_action(),
  //passing null for the accelerator.

  void add(const Glib::RefPtr<Action>& action, const AccelKey& accel_key);
  
  //We need to duplicate the gtk_action_group_add_action_with_accel() implementation, because we want to
  //use AccelKey, not just the accelerator string format that is _one_ of the ways to create an AccelKey.
   
  //TODO: Could this whole class have an STL-style interface?
  void add(const Glib::RefPtr<Action>& action, const Action::SlotActivate& slot);
  void add(const Glib::RefPtr<Action>& action, const AccelKey& accel_key, const Action::SlotActivate& slot);
  
  /** Removes an action object from the action group.
   * 
   * @newin{2,4}
   * 
   * @param action An action.
   */
  void remove(const Glib::RefPtr<Action>& action);
  
   //TODO: We probably need to use this in our add_actions() implementation:
  
  /** Translates a string using the specified translate_func(). This
   * is mainly intended for language bindings.
   * 
   * @newin{2,6}
   * 
   * @param string A string.
   * @return The translation of @a string.
   */
  Glib::ustring translate_string(const Glib::ustring& str) const;
  

  //These are just C convenience methods:
  
  //These are also just C convenience methods that are useless unless you are using the other convenience methods:
  

  /** The connect_proxy signal is emitted after connecting a proxy to 
   * an action in the group. Note that the proxy may have been connected 
   * to a different action before.
   *
   * This is intended for simple customizations for which a custom action
   * class would be too clumsy, e.g. showing tooltips for menuitems in the
   * statusbar.
   *
   * UIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * @param action the action
   * @param proxy the proxy
   *
   * @par Slot Prototype:
   * <tt>void on_my_%connect_proxy(const Glib::RefPtr<Action>& action, Widget* proxy)</tt>
   *
   */

  Glib::SignalProxy2< void,const Glib::RefPtr<Action>&,Widget* > signal_connect_proxy();

  
  /** The disconnect_proxy signal is emitted after disconnecting a proxy 
   * from an action in the group. 
   *
   * UIManager proxies the signal and provides global notification 
   * just before any action is connected to a proxy, which is probably more
   * convenient to use.
   *
   * @param action the action
   * @param proxy the proxy
   *
   * @par Slot Prototype:
   * <tt>void on_my_%disconnect_proxy(const Glib::RefPtr<Action>& action, Widget* proxy)</tt>
   *
   */

  Glib::SignalProxy2< void,const Glib::RefPtr<Action>&,Widget* > signal_disconnect_proxy();

  
  /** The pre_activate signal is emitted just before the @action in the
   * action_group is activated
   *
   * This is intended for UIManager to proxy the signal and provide global
   * notification just before any action is activated.
   *
   * @action the action
   *
   * @par Slot Prototype:
   * <tt>void on_my_%pre_activate(const Glib::RefPtr<Action>& action)</tt>
   *
   */

  Glib::SignalProxy1< void,const Glib::RefPtr<Action>& > signal_pre_activate();

  
  /** The post_activate signal is emitted just after the @action in the
   * @action_group is activated
   *
   * This is intended for UIManager to proxy the signal and provide global
   * notification just after any action is activated.
   *
   * @param action the action
   *
   * @par Slot Prototype:
   * <tt>void on_my_%post_activate(const Glib::RefPtr<Action>& action)</tt>
   *
   */

  Glib::SignalProxy1< void,const Glib::RefPtr<Action>& > signal_post_activate();

  
  /** A name for the action group.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< Glib::ustring > property_name() const;


  /** Whether the action group is enabled.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< bool > property_sensitive() ;

/** Whether the action group is enabled.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< bool > property_sensitive() const;

  /** Whether the action group is visible.
   *
   * @return A PropertyProxy that allows you to get or set the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy< bool > property_visible() ;

/** Whether the action group is visible.
   *
   * @return A PropertyProxy_ReadOnly that allows you to get the value of the property,
   * or receive notification when the value of the property changes.
   */
  Glib::PropertyProxy_ReadOnly< bool > property_visible() const;


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


};

} // namespace Gtk


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gtk::ActionGroup
   */
  Glib::RefPtr<Gtk::ActionGroup> wrap(GtkActionGroup* object, bool take_copy = false);
}


#endif /* _GTKMM_ACTIONGROUP_H */

