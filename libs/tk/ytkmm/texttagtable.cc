// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!


#include <glibmm.h>

#include <gtkmm/texttagtable.h>
#include <gtkmm/private/texttagtable_p.h>


// -*- c++ -*-
/* $Id: texttagtable.ccg,v 1.5 2006/04/12 10:18:43 murrayc Exp $ */

/* 
 *
 * Copyright 1998-2002 The gtkmm Development Team
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

#include <gtkmm/texttag.h>
#include <gtk/gtk.h>


static void SignalProxy_ForEach_gtk_callback(GtkTextTag* texttag, gpointer data)
{
  Gtk::TextTagTable::SlotForEach* the_slot = static_cast<Gtk::TextTagTable::SlotForEach*>(data);
  if(the_slot)
  {
    //Use Slot::operator():
    (*the_slot)(Glib::wrap(texttag, true)); //true = take_copy.
  }
}

namespace Gtk
{

void TextTagTable::foreach(const SlotForEach& slot)
{
  //The slot doesn't need to exist for longer than the function call.
  gtk_text_tag_table_foreach(gobj(),  &SignalProxy_ForEach_gtk_callback, (gpointer)&slot);
}


} /* namespace Gtk */


namespace
{


static void TextTagTable_signal_tag_changed_callback(GtkTextTagTable* self, GtkTextTag* p0,gboolean p1,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,const Glib::RefPtr<TextTag>&,bool > SlotType;

  TextTagTable* obj = dynamic_cast<TextTagTable*>(Glib::ObjectBase::_get_current_wrapper((GObject*) self));
  // Do not try to call a signal on a disassociated wrapper.
  if(obj)
  {
    try
    {
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0, true)
, p1
);
    }
    catch(...)
    {
       Glib::exception_handlers_invoke();
    }
  }
}

static const Glib::SignalProxyInfo TextTagTable_signal_tag_changed_info =
{
  "tag_changed",
  (GCallback) &TextTagTable_signal_tag_changed_callback,
  (GCallback) &TextTagTable_signal_tag_changed_callback
};


static void TextTagTable_signal_tag_added_callback(GtkTextTagTable* self, GtkTextTag* p0,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,const Glib::RefPtr<TextTag>& > SlotType;

  TextTagTable* obj = dynamic_cast<TextTagTable*>(Glib::ObjectBase::_get_current_wrapper((GObject*) self));
  // Do not try to call a signal on a disassociated wrapper.
  if(obj)
  {
    try
    {
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0, true)
);
    }
    catch(...)
    {
       Glib::exception_handlers_invoke();
    }
  }
}

static const Glib::SignalProxyInfo TextTagTable_signal_tag_added_info =
{
  "tag_added",
  (GCallback) &TextTagTable_signal_tag_added_callback,
  (GCallback) &TextTagTable_signal_tag_added_callback
};


static void TextTagTable_signal_tag_removed_callback(GtkTextTagTable* self, GtkTextTag* p0,void* data)
{
  using namespace Gtk;
  typedef sigc::slot< void,const Glib::RefPtr<TextTag>& > SlotType;

  TextTagTable* obj = dynamic_cast<TextTagTable*>(Glib::ObjectBase::_get_current_wrapper((GObject*) self));
  // Do not try to call a signal on a disassociated wrapper.
  if(obj)
  {
    try
    {
      if(sigc::slot_base *const slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(Glib::wrap(p0, true)
);
    }
    catch(...)
    {
       Glib::exception_handlers_invoke();
    }
  }
}

static const Glib::SignalProxyInfo TextTagTable_signal_tag_removed_info =
{
  "tag_removed",
  (GCallback) &TextTagTable_signal_tag_removed_callback,
  (GCallback) &TextTagTable_signal_tag_removed_callback
};


} // anonymous namespace


namespace Glib
{

Glib::RefPtr<Gtk::TextTagTable> wrap(GtkTextTagTable* object, bool take_copy)
{
  return Glib::RefPtr<Gtk::TextTagTable>( dynamic_cast<Gtk::TextTagTable*> (Glib::wrap_auto ((GObject*)(object), take_copy)) );
  //We use dynamic_cast<> in case of multiple inheritance.
}

} /* namespace Glib */


namespace Gtk
{


/* The *_Class implementation: */

const Glib::Class& TextTagTable_Class::init()
{
  if(!gtype_) // create the GType if necessary
  {
    // Glib::Class has to know the class init function to clone custom types.
    class_init_func_ = &TextTagTable_Class::class_init_function;

    // This is actually just optimized away, apparently with no harm.
    // Make sure that the parent type has been created.
    //CppClassParent::CppObjectType::get_type();

    // Create the wrapper type, with the same class/instance size as the base type.
    register_derived_type(gtk_text_tag_table_get_type());

    // Add derived versions of interfaces, if the C type implements any interfaces:

  }

  return *this;
}


void TextTagTable_Class::class_init_function(void* g_class, void* class_data)
{
  BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
  CppClassParent::class_init_function(klass, class_data);


  klass->tag_changed = &tag_changed_callback;
  klass->tag_added = &tag_added_callback;
  klass->tag_removed = &tag_removed_callback;
}


void TextTagTable_Class::tag_changed_callback(GtkTextTagTable* self, GtkTextTag* p0, gboolean p1)
{
  Glib::ObjectBase *const obj_base = static_cast<Glib::ObjectBase*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj_base && obj_base->is_derived_())
  {
    CppObjectType *const obj = dynamic_cast<CppObjectType* const>(obj_base);
    if(obj) // This can be NULL during destruction.
    {
      try // Trap C++ exceptions which would normally be lost because this is a C callback.
      {
        // Call the virtual member method, which derived classes might override.
        obj->on_tag_changed(Glib::wrap(p0, true)
, p1
);
        return;
      }
      catch(...)
      {
        Glib::exception_handlers_invoke();
      }
    }
  }

  BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_class_peek_parent(G_OBJECT_GET_CLASS(self)) // Get the parent class of the object class (The original underlying C class).
    );

  // Call the original underlying C function:
  if(base && base->tag_changed)
    (*base->tag_changed)(self, p0, p1);
}
void TextTagTable_Class::tag_added_callback(GtkTextTagTable* self, GtkTextTag* p0)
{
  Glib::ObjectBase *const obj_base = static_cast<Glib::ObjectBase*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj_base && obj_base->is_derived_())
  {
    CppObjectType *const obj = dynamic_cast<CppObjectType* const>(obj_base);
    if(obj) // This can be NULL during destruction.
    {
      try // Trap C++ exceptions which would normally be lost because this is a C callback.
      {
        // Call the virtual member method, which derived classes might override.
        obj->on_tag_added(Glib::wrap(p0, true)
);
        return;
      }
      catch(...)
      {
        Glib::exception_handlers_invoke();
      }
    }
  }

  BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_class_peek_parent(G_OBJECT_GET_CLASS(self)) // Get the parent class of the object class (The original underlying C class).
    );

  // Call the original underlying C function:
  if(base && base->tag_added)
    (*base->tag_added)(self, p0);
}
void TextTagTable_Class::tag_removed_callback(GtkTextTagTable* self, GtkTextTag* p0)
{
  Glib::ObjectBase *const obj_base = static_cast<Glib::ObjectBase*>(
      Glib::ObjectBase::_get_current_wrapper((GObject*)self));

  // Non-gtkmmproc-generated custom classes implicitly call the default
  // Glib::ObjectBase constructor, which sets is_derived_. But gtkmmproc-
  // generated classes can use this optimisation, which avoids the unnecessary
  // parameter conversions if there is no possibility of the virtual function
  // being overridden:
  if(obj_base && obj_base->is_derived_())
  {
    CppObjectType *const obj = dynamic_cast<CppObjectType* const>(obj_base);
    if(obj) // This can be NULL during destruction.
    {
      try // Trap C++ exceptions which would normally be lost because this is a C callback.
      {
        // Call the virtual member method, which derived classes might override.
        obj->on_tag_removed(Glib::wrap(p0, true)
);
        return;
      }
      catch(...)
      {
        Glib::exception_handlers_invoke();
      }
    }
  }

  BaseClassType *const base = static_cast<BaseClassType*>(
        g_type_class_peek_parent(G_OBJECT_GET_CLASS(self)) // Get the parent class of the object class (The original underlying C class).
    );

  // Call the original underlying C function:
  if(base && base->tag_removed)
    (*base->tag_removed)(self, p0);
}


Glib::ObjectBase* TextTagTable_Class::wrap_new(GObject* object)
{
  return new TextTagTable((GtkTextTagTable*)object);
}


/* The implementation: */

GtkTextTagTable* TextTagTable::gobj_copy()
{
  reference();
  return gobj();
}

TextTagTable::TextTagTable(const Glib::ConstructParams& construct_params)
:
  Glib::Object(construct_params)
{

}

TextTagTable::TextTagTable(GtkTextTagTable* castitem)
:
  Glib::Object((GObject*)(castitem))
{}


TextTagTable::~TextTagTable()
{}


TextTagTable::CppClassType TextTagTable::texttagtable_class_; // initialize static member

GType TextTagTable::get_type()
{
  return texttagtable_class_.init().get_type();
}


GType TextTagTable::get_base_type()
{
  return gtk_text_tag_table_get_type();
}


TextTagTable::TextTagTable()
:
  // Mark this class as non-derived to allow C++ vfuncs to be skipped.
  Glib::ObjectBase(0),
  Glib::Object(Glib::ConstructParams(texttagtable_class_.init()))
{
  

}

Glib::RefPtr<TextTagTable> TextTagTable::create()
{
  return Glib::RefPtr<TextTagTable>( new TextTagTable() );
}

void TextTagTable::add(const Glib::RefPtr<TextTag>& tag)
{
  gtk_text_tag_table_add(gobj(), Glib::unwrap(tag));
}

void TextTagTable::remove(const Glib::RefPtr<TextTag>& tag)
{
  gtk_text_tag_table_remove(gobj(), Glib::unwrap(tag));
}

Glib::RefPtr<TextTag> TextTagTable::lookup(const Glib::ustring& name)
{
  Glib::RefPtr<TextTag> retvalue = Glib::wrap(gtk_text_tag_table_lookup(gobj(), name.c_str()));
  if(retvalue)
    retvalue->reference(); //The function does not do a ref for us.
  return retvalue;
}

Glib::RefPtr<const TextTag> TextTagTable::lookup(const Glib::ustring& name) const
{
  return const_cast<TextTagTable*>(this)->lookup(name);
}

int TextTagTable::get_size() const
{
  return gtk_text_tag_table_get_size(const_cast<GtkTextTagTable*>(gobj()));
}


Glib::SignalProxy2< void,const Glib::RefPtr<TextTag>&,bool > TextTagTable::signal_tag_changed()
{
  return Glib::SignalProxy2< void,const Glib::RefPtr<TextTag>&,bool >(this, &TextTagTable_signal_tag_changed_info);
}


Glib::SignalProxy1< void,const Glib::RefPtr<TextTag>& > TextTagTable::signal_tag_added()
{
  return Glib::SignalProxy1< void,const Glib::RefPtr<TextTag>& >(this, &TextTagTable_signal_tag_added_info);
}


Glib::SignalProxy1< void,const Glib::RefPtr<TextTag>& > TextTagTable::signal_tag_removed()
{
  return Glib::SignalProxy1< void,const Glib::RefPtr<TextTag>& >(this, &TextTagTable_signal_tag_removed_info);
}


void Gtk::TextTagTable::on_tag_changed(const Glib::RefPtr<TextTag>& tag, bool size_changed)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->tag_changed)
    (*base->tag_changed)(gobj(),Glib::unwrap(tag),static_cast<int>(size_changed));
}
void Gtk::TextTagTable::on_tag_added(const Glib::RefPtr<TextTag>& tag)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->tag_added)
    (*base->tag_added)(gobj(),Glib::unwrap(tag));
}
void Gtk::TextTagTable::on_tag_removed(const Glib::RefPtr<TextTag>& tag)
{
  BaseClassType *const base = static_cast<BaseClassType*>(
      g_type_class_peek_parent(G_OBJECT_GET_CLASS(gobject_)) // Get the parent class of the object class (The original underlying C class).
  );

  if(base && base->tag_removed)
    (*base->tag_removed)(gobj(),Glib::unwrap(tag));
}


} // namespace Gtk


