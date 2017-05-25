/*
    Copyright (C) 2009-2013 Paul Davis
    Author: Johannes Mueller

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#include <glibmm.h>

#include "pbd/compose.h"
#include "pbd/error.h"
#include "ardour/debug.h"
#include "ardour/session.h"
#include "pbd/i18n.h"

#include "shuttlepro.h"

using namespace ARDOUR;
using namespace PBD;
using namespace Glib;
using namespace std;

#include "pbd/abstract_ui.cc" // instantiate template


ShuttleproControlProtocol::ShuttleproControlProtocol (Session& session)
	: ControlProtocol (session, X_("Shuttlepro"))
	,  AbstractUI<ShuttleproControlUIRequest> ("shuttlepro")
	, _file_descriptor (-1)
	, _jog_position (-1)
{
}

ShuttleproControlProtocol::~ShuttleproControlProtocol ()
{
	stop ();
}

bool
ShuttleproControlProtocol::probe ()
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, "PROBE\n");
	return true;
}

int
ShuttleproControlProtocol::set_active (bool yn)
{
	int result;

	DEBUG_TRACE (DEBUG::ShuttleproControl, string_compose ("ShuttleproControlProtocol::set_active init with yn: '%1'\n", yn));

	/* do nothing if the active state is not changing */

	if (yn == active()) {
		return 0;
	}

	if (yn) {
		/* activate Shuttlepro control surface */
		result = start ();
	} else {
		/* deactivate Shuttlepro control surface */
		result = stop ();
	}

	ControlProtocol::set_active (yn);

	DEBUG_TRACE (DEBUG::ShuttleproControl, "ShuttleproControlProtocol::set_active done\n");

	return result;
}

XMLNode&
ShuttleproControlProtocol::get_state ()
{
	XMLNode& node (ControlProtocol::get_state());
	node.set_property (X_("feedback"), "0");
	return node;
}

int
ShuttleproControlProtocol::set_state (const XMLNode&, int)
{
	return 0;
}

void
ShuttleproControlProtocol::do_request (ShuttleproControlUIRequest* req)
{
	if (req->type == CallSlot) {
		DEBUG_TRACE (DEBUG::ShuttleproControl, "do_request type CallSlot");
		call_slot (MISSING_INVALIDATOR, req->the_slot);
	} else if (req->type == Quit) {
		DEBUG_TRACE (DEBUG::ShuttleproControl, "do_request type Quit");
		stop ();
	}
}

void
ShuttleproControlProtocol::thread_init () {
	DEBUG_TRACE (DEBUG::ShuttleproControl, "thread_init()");

	BasicUI::register_thread (X_("Shuttlepro"));

	_file_descriptor = open("/dev/input/by-id/usb-Contour_Design_ShuttlePRO_v2-event-if00", O_RDONLY);
	if (_file_descriptor < 0) {
		cout << "Could not open Shuttlepro" << endl; // FIXME error handling and discovery missing
		return;
	}

	_shuttle_position = 0;
	_old_shuttle_position = 0;
	_shuttle_event_recieved = false;

	Glib::RefPtr<Glib::IOSource> source = Glib::IOSource::create (_file_descriptor, IO_IN | IO_ERR);
	source->connect (sigc::bind (sigc::mem_fun (*this, &ShuttleproControlProtocol::input_event), _file_descriptor));
	source->attach (_main_loop->get_context ());

	_io_source = source->gobj ();
	g_source_ref (_io_source);

	DEBUG_TRACE (DEBUG::ShuttleproControl, "thread_init() fin")
}

int
ShuttleproControlProtocol::start ()
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, "start()\n");

	BaseUI::run();

	DEBUG_TRACE (DEBUG::ShuttleproControl, "start() fin\n");
	return 0;
}


int
ShuttleproControlProtocol::stop ()
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, "stop()\n");

	if (_io_source) {
		g_source_destroy (_io_source);
		g_source_unref (_io_source);
		_io_source = 0;

		close(_file_descriptor);
		_file_descriptor = -1;
	}

	BaseUI::quit();

	DEBUG_TRACE (DEBUG::ShuttleproControl, "stop() fin\n");
	return 0;
}

void
ShuttleproControlProtocol::handle_event (EV ev) {
	if (ev.type == 0) { // check if shuttle is turned
		if (_shuttle_event_recieved) {
			if (_shuttle_position != _old_shuttle_position) {
				shuttle_event (_shuttle_position);
				_old_shuttle_position = _shuttle_position;
			}
			_shuttle_event_recieved = false;
		} else {
			if (_shuttle_position != 0) {
				shuttle_event (0);
				_shuttle_position = 0;
				_old_shuttle_position = 0;
			}
		}
	}

	if (ev.type == 1) { // key
		if (ev.value == 1) {
			handle_key_press (ev.code-255);
		}
		return;
	}

	if (ev.code == 7) { // jog wheel
		if (_jog_position == -1) { // first jog event needed to get orientation
			_jog_position = ev.value;
			return;
		}
		if (ev.value - _jog_position < 0) {
			jog_event_backward();
		}
		if (ev.value - _jog_position > 0) {
			jog_event_forward();
		}
		_jog_position = ev.value;

		return;
	}

	if (ev.code == 8) { // shuttle wheel
		_shuttle_event_recieved = true;
		_shuttle_position = ev.value;
		return;
	}
}

/* The keys have the following layout
 *
 *          1   2   3   4
 *        5   6   7   8   9
 *
 *        14    Jog     15
 *
 *           10     11
 *           12     13
 */

enum Key {
	KEY01 = 1, KEY02 = 2, KEY03 = 3, KEY04 = 4, KEY05 = 5, KEY06 = 6, KEY07 = 7,
	KEY08 = 8, KEY09 = 9, KEY10 = 10, KEY11 = 11, KEY12 = 12, KEY13 = 13, KEY14 = 14, KEY15 = 15
};

void
ShuttleproControlProtocol::handle_key_press (unsigned short key)
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, string_compose ("Shuttlepro key number %1\n", key));
	switch (key) {
	case KEY14: goto_start (); break;
	case KEY15: goto_end (); break;
	default: break;
	}
}

void
ShuttleproControlProtocol::jog_event_backward()
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, "jog event backward\n");
	jump_by_seconds(-0.2);
}

void
ShuttleproControlProtocol::jog_event_forward()
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, "jog event forward\n");
	jump_by_seconds(0.2);
}

void
ShuttleproControlProtocol::shuttle_event(int position)
{
	DEBUG_TRACE (DEBUG::ShuttleproControl, string_compose ("shuttle event %1\n", position));
	set_transport_speed(double(position));
}

bool
ShuttleproControlProtocol::input_event (IOCondition ioc, int fd)
{
	if (ioc & ~IO_IN) {
		return false;
	}

	EV ev;
	size_t r;

	r = read (fd, &ev, sizeof (ev));
	if (r < sizeof (ev)) {
		DEBUG_TRACE (DEBUG::ShuttleproControl, "Received too small event, strange.\n");
		return false;
	}
	handle_event (ev);

	return true;
}
