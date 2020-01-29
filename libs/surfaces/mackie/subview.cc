/*
 * Copyright (C) 2006-2007 John Anderson
 * Copyright (C) 2007-2010 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2015-2016 Len Ovens <len@ovenwerks.net>
 * Copyright (C) 2015-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2016-2018 Ben Loftis <ben@harrisonconsoles.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
 
#include "pbd/convert.h"

#include "ardour/debug.h"
#include "ardour/monitor_control.h"
#include "ardour/phase_control.h"
#include "ardour/plugin.h"
#include "ardour/plugin_insert.h"
#include "ardour/route.h"
#include "ardour/solo_isolate_control.h"
#include "ardour/stripable.h"
#include "ardour/track.h"

#include "mackie_control_protocol.h"
#include "pot.h"
#include "strip.h"
#include "subview.h"
#include "surface.h"
 
using namespace ARDOUR;
using namespace ArdourSurface;
using namespace Mackie;
using namespace PBD;
	
#define ui_context() MackieControlProtocol::instance() /* a UICallback-derived object that specifies the event loop for signal handling */

SubviewFactory* SubviewFactory::_instance = 0;

SubviewFactory* SubviewFactory::instance() {
	if (!_instance) {
		_instance = new SubviewFactory();
	}
	return _instance;
}

SubviewFactory::SubviewFactory() {};

boost::shared_ptr<Subview> SubviewFactory::create_subview(
		SubViewMode svm, 
		MackieControlProtocol& mcp, 
		boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
{
	switch (svm) {
		case SubViewMode::EQ:
			return boost::make_shared<EQSubview>(mcp, subview_stripable);
		case SubViewMode::Dynamics:
			return boost::make_shared<DynamicsSubview>(mcp, subview_stripable);
		case SubViewMode::Sends:
			return boost::make_shared<SendsSubview>(mcp, subview_stripable);
		case SubViewMode::TrackView:
			return boost::make_shared<TrackViewSubview>(mcp, subview_stripable);
		case SubViewMode::Plugin:
			return boost::make_shared<PluginSubview>(mcp, subview_stripable);
		case SubViewMode::None:
		default:
			return boost::make_shared<NoneSubview>(mcp, subview_stripable);
	}
}


Subview::Subview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable)
	: _mcp(mcp)
	, _subview_stripable(subview_stripable)
{
	init_strip_vectors();
}

Subview::~Subview() {}

void Subview::handle_vselect_event(uint32_t global_strip_position) 
{
	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}
	
	if (!strip || !vpot || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control = vpot->control ();
	if (!control) {
		return;
	}

	Controllable::GroupControlDisposition gcd;
	if (_mcp.main_modifier_state() & MackieControlProtocol::MODIFIER_SHIFT) {
		gcd = Controllable::InverseGroup;
	} else {
		gcd = Controllable::UseGroup;
	}

	if (control->toggled()) {
		if (control->toggled()) {
			control->set_value (!control->get_value(), gcd);
		}

	} else if (control->desc().enumeration || control->desc().integer_step) {

		double val = control->get_value ();
		if (val <= control->upper() - 1.0) {
			control->set_value (val + 1.0, gcd);
		} else {
			control->set_value (control->lower(), gcd);
		}
	}
}

bool
Subview::subview_mode_would_be_ok (SubViewMode mode, boost::shared_ptr<Stripable> r, std::string& reason_why_not)
{
	switch (mode) {
	case SubViewMode::None:
		return NoneSubview::subview_mode_would_be_ok(r, reason_why_not);
	case SubViewMode::Sends:
		return SendsSubview::subview_mode_would_be_ok(r, reason_why_not);
	case SubViewMode::EQ:
		return EQSubview::subview_mode_would_be_ok(r, reason_why_not);
	case SubViewMode::Dynamics:
		return DynamicsSubview::subview_mode_would_be_ok(r, reason_why_not);
	case SubViewMode::TrackView:
		return TrackViewSubview::subview_mode_would_be_ok(r, reason_why_not);
	case SubViewMode::Plugin:
		return PluginSubview::subview_mode_would_be_ok(r, reason_why_not);
	}

	return false;
}

void
Subview::notify_subview_stripable_deleted ()
{
	_subview_stripable.reset ();
}

void
Subview::init_strip_vectors()
{
	_strips_over_all_surfaces.resize(_mcp.n_strips(), 0);
	_strip_vpots_over_all_surfaces.resize(_mcp.n_strips(), 0);
	_strip_pending_displays_over_all_surfaces.resize(_mcp.n_strips(), 0);
}

void
Subview::store_pointers(Strip* strip, Pot* vpot, std::string* pending_display, uint32_t global_strip_position) 
{
	if (global_strip_position >= _strips_over_all_surfaces.size() ||
		global_strip_position >= _strip_vpots_over_all_surfaces.size() ||
		global_strip_position >= _strip_pending_displays_over_all_surfaces.size())
	{
		return;
	}
	
	_strips_over_all_surfaces[global_strip_position] = strip;
	_strip_vpots_over_all_surfaces[global_strip_position] = vpot;
	_strip_pending_displays_over_all_surfaces[global_strip_position] = pending_display;
}

bool
Subview::retrieve_pointers(Strip** strip, Pot** vpot, std::string** pending_display, uint32_t global_strip_position)
{
	if (global_strip_position >= _strips_over_all_surfaces.size() ||
		global_strip_position >= _strip_vpots_over_all_surfaces.size() ||
		global_strip_position >= _strip_pending_displays_over_all_surfaces.size()) 
	{
		return false;
	}
	
	*strip = _strips_over_all_surfaces[global_strip_position];
	*vpot = _strip_vpots_over_all_surfaces[global_strip_position];
	*pending_display = _strip_pending_displays_over_all_surfaces[global_strip_position];
	return true;
}



NoneSubview::NoneSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{}

NoneSubview::~NoneSubview() 
{}

bool NoneSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	return true;
}

void NoneSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, off);
	_mcp.update_global_button (Button::Plugin, off);
	_mcp.update_global_button (Button::Eq, off);
	_mcp.update_global_button (Button::Dyn, off);
	_mcp.update_global_button (Button::Track, off);
	_mcp.update_global_button (Button::Pan, on);
}

void NoneSubview::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2])
{
	
}



EQSubview::EQSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{}

EQSubview::~EQSubview() 
{}

bool EQSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	if (r && r->eq_band_cnt() > 0) {
		return true;
	} 
	
	reason_why_not = "no EQ in the track/bus";
	return false;
}

void EQSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, off);
	_mcp.update_global_button (Button::Plugin, off);
	_mcp.update_global_button (Button::Eq, on);
	_mcp.update_global_button (Button::Dyn, off);
	_mcp.update_global_button (Button::Track, off);
	_mcp.update_global_button (Button::Pan, off);
}

void EQSubview::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2])
{
	const uint32_t global_strip_position = _mcp.global_index (*strip);
	store_pointers(strip, vpot, pending_display, global_strip_position);
		
	if (!_subview_stripable) {
		return;
	}
	
	
	boost::shared_ptr<AutomationControl> pc;
	std::string pot_id;

#ifdef MIXBUS
	int eq_band = -1;
	std::string band_name;
	if (_subview_stripable->is_input_strip ()) {

#ifdef MIXBUS32C
		switch (global_strip_position) {
			case 0:
			case 2:
			case 4:
			case 6:
				eq_band = global_strip_position / 2;
				pc = _subview_stripable->eq_freq_controllable (eq_band);
				band_name = _subview_stripable->eq_band_name (eq_band);
				pot_id = band_name + "Freq";
				break;
			case 1:
			case 3:
			case 5:
			case 7:
				eq_band = global_strip_position / 2;
				pc = _subview_stripable->eq_gain_controllable (eq_band);
				band_name = _subview_stripable->eq_band_name (eq_band);
				pot_id = band_name + "Gain";
				break;
			case 8: 
				pc = _subview_stripable->eq_shape_controllable(0);  //low band "bell" button
				band_name = "lo";
				pot_id = band_name + " Shp";
				break;
			case 9:
				pc = _subview_stripable->eq_shape_controllable(3);  //high band "bell" button
				band_name = "hi";
				pot_id = band_name + " Shp";
				break;
			case 10:
				pc = _subview_stripable->eq_enable_controllable();
				pot_id = "EQ";
				break;
		}

#else  //regular Mixbus channel EQ

		switch (global_strip_position) {
			case 0:
			case 2:
			case 4:
				eq_band = global_strip_position / 2;
				pc = _subview_stripable->eq_gain_controllable (eq_band);
				band_name = _subview_stripable->eq_band_name (eq_band);
				pot_id = band_name + "Gain";
				break;
			case 1:
			case 3:
			case 5:
				eq_band = global_strip_position / 2;
				pc = _subview_stripable->eq_freq_controllable (eq_band);
				band_name = _subview_stripable->eq_band_name (eq_band);
				pot_id = band_name + "Freq";
				break;
			case 6:
				pc = _subview_stripable->eq_enable_controllable();
				pot_id = "EQ";
				break;
			case 7:
				pc = _subview_stripable->filter_freq_controllable(true);
				pot_id = "HP Freq";
				break;
		}

#endif

	} else {  //mixbus or master bus ( these are currently the same for MB & 32C )
		switch (global_strip_position) {
			case 0:
			case 1:
			case 2:
				eq_band = global_strip_position;
				pc = _subview_stripable->eq_gain_controllable (eq_band);
				band_name = _subview_stripable->eq_band_name (eq_band);
				pot_id = band_name + "Gain";
				break;
		}
	}
#endif

	//If a controllable was found, connect it up, and put the labels in the display.
	if (pc) {
		pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&EQSubview::notify_change, this, boost::weak_ptr<AutomationControl>(pc), global_strip_position, false), ui_context());
		vpot->set_control (pc);

		if (!pot_id.empty()) {
			pending_display[0] = pot_id;
		} else {
			pending_display[0] = std::string();
		}
		
	} else {  //no controllable was found;  just clear this knob
		vpot->set_control (boost::shared_ptr<AutomationControl>());
		pending_display[0] = std::string();
		pending_display[1] = std::string();
	}
	
	notify_change (boost::weak_ptr<AutomationControl>(pc), global_strip_position, true);
}

void EQSubview::notify_change (boost::weak_ptr<ARDOUR::AutomationControl> pc, uint32_t global_strip_position, bool force) 
{
	if (!_subview_stripable) {
		return;
	}
	
	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}
	
	if (!strip || !vpot || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control = pc.lock ();
	if (control) {
		float val = control->get_value();
		//do_parameter_display (control->desc(), val, true);
		{
			bool screen_hold = true;
			pending_display[1] = Strip::format_paramater_for_display(
					control->desc(), 
					val, 
					strip->stripable(), 
					screen_hold
				);
	
			if (screen_hold) {
				/* we just queued up a parameter to be displayed.
					1 second from now, switch back to vpot mode display.
				*/
				strip->block_vpot_mode_display_for (1000);
			}
		}
		
		/* update pot/encoder */
		strip->surface()->write (vpot->set (control->internal_to_interface (val), true, Pot::wrap));
	}
}



DynamicsSubview::DynamicsSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{}

DynamicsSubview::~DynamicsSubview() 
{}

bool DynamicsSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	if (r && r->comp_enable_controllable()) {
		return true;
	}
	
	reason_why_not = "no dynamics in selected track/bus";
	return false;
}

void DynamicsSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, off);
	_mcp.update_global_button (Button::Plugin, off);
	_mcp.update_global_button (Button::Eq, off);
	_mcp.update_global_button (Button::Dyn, on);
	_mcp.update_global_button (Button::Track, off);
	_mcp.update_global_button (Button::Pan, off);
}

void DynamicsSubview::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2])
{
	const uint32_t global_strip_position = _mcp.global_index (*strip);
	store_pointers(strip, vpot, pending_display, global_strip_position);
	
	if (!_subview_stripable) {
		return;
	}
	
	boost::shared_ptr<AutomationControl> tc = _subview_stripable->comp_threshold_controllable ();
	boost::shared_ptr<AutomationControl> sc = _subview_stripable->comp_speed_controllable ();
	boost::shared_ptr<AutomationControl> mc = _subview_stripable->comp_mode_controllable ();
	boost::shared_ptr<AutomationControl> kc = _subview_stripable->comp_makeup_controllable ();
	boost::shared_ptr<AutomationControl> ec = _subview_stripable->comp_enable_controllable ();

#ifdef MIXBUS32C	//Mixbus32C needs to spill the filter controls into the comp section
	boost::shared_ptr<AutomationControl> hpfc = _subview_stripable->filter_freq_controllable (true);
	boost::shared_ptr<AutomationControl> lpfc = _subview_stripable->filter_freq_controllable (false);
	boost::shared_ptr<AutomationControl> fec = _subview_stripable->filter_enable_controllable (true); // shared HP/LP
#endif

	/* we will control the global_strip_position-th available parameter, from the list in the
	 * order shown above.
	 */

	std::vector<std::pair<boost::shared_ptr<AutomationControl>, std::string > > available;
	std::vector<AutomationType> params;

	if (tc) { available.push_back (std::make_pair (tc, "Thresh")); }
	if (sc) { available.push_back (std::make_pair (sc, mc ? _subview_stripable->comp_speed_name (mc->get_value()) : "Speed")); }
	if (mc) { available.push_back (std::make_pair (mc, "Mode")); }
	if (kc) { available.push_back (std::make_pair (kc, "Makeup")); }
	if (ec) { available.push_back (std::make_pair (ec, "on/off")); }

#ifdef MIXBUS32C	//Mixbus32C needs to spill the filter controls into the comp section
	if (hpfc) { available.push_back (std::make_pair (hpfc, "HPF")); }
	if (lpfc) { available.push_back (std::make_pair (lpfc, "LPF")); }
	if (fec)  { available.push_back (std::make_pair (fec, "FiltIn")); }
#endif

	if (global_strip_position >= available.size()) {
		/* this knob is not needed to control the available parameters */
		vpot->set_control (boost::shared_ptr<AutomationControl>());
		pending_display[0] = std::string();
		pending_display[1] = std::string();
		return;
	}

	boost::shared_ptr<AutomationControl> pc;

	pc = available[global_strip_position].first;
	std::string pot_id = available[global_strip_position].second;

	pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&DynamicsSubview::notify_change, this, boost::weak_ptr<AutomationControl>(pc), global_strip_position, false, true), ui_context());
	vpot->set_control (pc);

	if (!pot_id.empty()) {
		pending_display[0] = pot_id;
	} else {
		pending_display[0] = std::string();
	}

	notify_change (boost::weak_ptr<AutomationControl>(pc), global_strip_position, true, false);
}

void
DynamicsSubview::notify_change (boost::weak_ptr<ARDOUR::AutomationControl> pc, uint32_t global_strip_position, bool force, bool propagate_mode) 
{
	if (!_subview_stripable) 
	{
		return;
	}
	
	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}
	
	if (!strip || !vpot || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control= pc.lock ();
	bool reset_all = false;

	if (propagate_mode && reset_all) {
		// @TODO: this line can never be reached due to reset_all being set to false. What was intended here?
		strip->surface()->subview_mode_changed ();
	}

	if (control) {
		float val = control->get_value();
		if (control == _subview_stripable->comp_mode_controllable ()) {
			pending_display[1] = _subview_stripable->comp_mode_name (val);
		} else {
			//do_parameter_display (control->desc(), val, true);
			{
				bool screen_hold = true;
				pending_display[1] = Strip::format_paramater_for_display(
						control->desc(), 
						val, 
						strip->stripable(), 
						screen_hold
					);
		
				if (screen_hold) {
					/* we just queued up a parameter to be displayed.
						1 second from now, switch back to vpot mode display.
					*/
					strip->block_vpot_mode_display_for (1000);
				}
			}
		}
		/* update pot/encoder */
		strip->surface()->write (vpot->set (control->internal_to_interface (val), true, Pot::wrap));
	}
}



SendsSubview::SendsSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{}

SendsSubview::~SendsSubview() 
{}

bool SendsSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	if (r && r->send_level_controllable (0)) {
		return true;
	}
	
	reason_why_not = "no sends for selected track/bus";
	return false;
}

void SendsSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, on);
	_mcp.update_global_button (Button::Plugin, off);
	_mcp.update_global_button (Button::Eq, off);
	_mcp.update_global_button (Button::Dyn, off);
	_mcp.update_global_button (Button::Track, off);
	_mcp.update_global_button (Button::Pan, off);
}

void SendsSubview::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2])
{
	const uint32_t global_strip_position = _mcp.global_index (*strip);
	store_pointers(strip, vpot, pending_display, global_strip_position);
	
	if (!_subview_stripable) {
		return;
	}

	boost::shared_ptr<AutomationControl> pc = _subview_stripable->send_level_controllable (global_strip_position);

	if (!pc) {
		/* nothing to control */
		vpot->set_control (boost::shared_ptr<AutomationControl>());
		pending_display[0] = std::string();
		pending_display[1] = std::string();
		return;
	}

	pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&SendsSubview::notify_send_level_change, this, global_strip_position, false), ui_context());
	vpot->set_control (pc);

	pending_display[0] = PBD::short_version (_subview_stripable->send_name (global_strip_position), 6);

	notify_send_level_change (global_strip_position, true);
}

void
SendsSubview::notify_send_level_change (uint32_t global_strip_position, bool force)
{
	if (!_subview_stripable) {
		return;
	}
	
	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}
	
	if (!strip || !vpot || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control = _subview_stripable->send_level_controllable (global_strip_position);
	if (!control) {
		return;
	}

	if (control) {
		float val = control->get_value();
		//do_parameter_display (control->desc (), val); // BusSendLevel
		{
			bool screen_hold = false;
			pending_display[1] = Strip::format_paramater_for_display(
					control->desc(), 
					val, 
					strip->stripable(), 
					screen_hold
				);
	
			if (screen_hold) {
				/* we just queued up a parameter to be displayed.
					1 second from now, switch back to vpot mode display.
				*/
				strip->block_vpot_mode_display_for (1000);
			}
		}

		if (vpot->control() == control) {
			/* update pot/encoder */
			strip->surface()->write (vpot->set (control->internal_to_interface (val), true, Pot::wrap));
		}
	}
}

void SendsSubview::handle_vselect_event(uint32_t global_strip_position)
{
	/* Send mode: press enables/disables the relevant
		* send, but the vpot is bound to the send-level so we
		* need to lookup the enable/disable control
		* explicitly.
		*/

	if (!_subview_stripable)
	{
		return;
	}

	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}

	if (!strip || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control = _subview_stripable->send_enable_controllable(global_strip_position);

	if (control) {
		bool currently_enabled = (bool) control->get_value();
		Controllable::GroupControlDisposition gcd;

		if (_mcp.main_modifier_state() & MackieControlProtocol::MODIFIER_SHIFT) {
			gcd = Controllable::InverseGroup;
		} else {
			gcd = Controllable::UseGroup;
		}

		control->set_value (!currently_enabled, gcd);

		if (currently_enabled) {
			/* we just turned it off */
			pending_display[1] = "off";
		} else {
			/* we just turned it on, show the level
			*/
			control = _subview_stripable->send_level_controllable (global_strip_position);
			//do_parameter_display (control->desc(), control->get_value()); // BusSendLevel
			{
				bool screen_hold = false;
				pending_display[1] = Strip::format_paramater_for_display(
						control->desc(), 
						control->get_value(), 
						strip->stripable(), 
						screen_hold
					);
		
				if (screen_hold) {
					/* we just queued up a parameter to be displayed.
						1 second from now, switch back to vpot mode display.
					*/
					strip->block_vpot_mode_display_for (1000);
				}
			}
		}
	}
}



TrackViewSubview::TrackViewSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{}

TrackViewSubview::~TrackViewSubview() 
{}

bool TrackViewSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	if (r)  {
		return true;
	}
	
	reason_why_not = "no track view possible";
	return false;
}

void TrackViewSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, off);
	_mcp.update_global_button (Button::Plugin, off);
	_mcp.update_global_button (Button::Eq, off);
	_mcp.update_global_button (Button::Dyn, off);
	_mcp.update_global_button (Button::Track, on);
	_mcp.update_global_button (Button::Pan, off);
}

void TrackViewSubview::setup_vpot(
		Strip* strip, 
		Pot* vpot, 
		std::string pending_display[2])
{
	const uint32_t global_strip_position = _mcp.global_index (*strip);
	store_pointers(strip, vpot, pending_display, global_strip_position);
	
	if (global_strip_position > 4) {
		/* nothing to control */
		vpot->set_control (boost::shared_ptr<AutomationControl>());
		pending_display[0] = std::string();
		pending_display[1] = std::string();
		return;
	}
	
	if (!_subview_stripable) {
		return;
	}

	boost::shared_ptr<AutomationControl> pc;
	boost::shared_ptr<Track> track = boost::dynamic_pointer_cast<Track> (_subview_stripable);

	switch (global_strip_position) {
	case 0:
		pc = _subview_stripable->trim_control ();
		if (pc) {
			pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&TrackViewSubview::notify_change, this, TrimAutomation, global_strip_position, false), ui_context());
			pending_display[0] = "Trim";
			notify_change (TrimAutomation, global_strip_position, true);
		}
		break;
	case 1:
		if (track) {
			pc = track->monitoring_control();
			if (pc) {
				pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&TrackViewSubview::notify_change, this, MonitoringAutomation, global_strip_position, false), ui_context());
				pending_display[0] = "Mon";
				notify_change (MonitoringAutomation, global_strip_position, true);
			}
		}
		break;
	case 2:
		pc = _subview_stripable->solo_isolate_control ();
		if (pc) {
			pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&TrackViewSubview::notify_change, this, SoloIsolateAutomation, global_strip_position, false), ui_context());
			notify_change (SoloIsolateAutomation, global_strip_position, true);
			pending_display[0] = "S-Iso";
		}
		break;
	case 3:
		pc = _subview_stripable->solo_safe_control ();
		if (pc) {
			pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&TrackViewSubview::notify_change, this, SoloSafeAutomation, global_strip_position, false), ui_context());
			notify_change (SoloSafeAutomation, global_strip_position, true);
			pending_display[0] = "S-Safe";
		}
		break;
	case 4:
		pc = _subview_stripable->phase_control();
		if (pc) {
			pc->Changed.connect (_subview_connections, MISSING_INVALIDATOR, boost::bind (&TrackViewSubview::notify_change, this, PhaseAutomation, global_strip_position, false), ui_context());
			notify_change (PhaseAutomation, global_strip_position, true);
			pending_display[0] = "Phase";
		}
		break;
	}
	
	if (!pc) {
		pending_display[0] = std::string();
		pending_display[1] = std::string();
		return;
	}

	vpot->set_control (pc);
}

void
TrackViewSubview::notify_change (AutomationType type, uint32_t global_strip_position, bool force_update)
{
	if (!_subview_stripable) {
		return;
	}
	
	Strip* strip = 0;
	Pot* vpot = 0;
	std::string* pending_display = 0;
	if (!retrieve_pointers(&strip, &vpot, &pending_display, global_strip_position))
	{
		return;
	}
	
	if (!strip || !vpot || !pending_display)
	{
		return;
	}

	boost::shared_ptr<AutomationControl> control;
	boost::shared_ptr<Track> track = boost::dynamic_pointer_cast<Track> (_subview_stripable);
	bool screen_hold = false;

	switch (type) {
		case TrimAutomation:
			control = _subview_stripable->trim_control();
			screen_hold = true;
			break;
		case SoloIsolateAutomation:
			control = _subview_stripable->solo_isolate_control ();
			break;
		case SoloSafeAutomation:
			control = _subview_stripable->solo_safe_control ();
			break;
		case MonitoringAutomation:
			if (track) {
				control = track->monitoring_control();
				screen_hold = true;
			}
			break;
		case PhaseAutomation:
			control = _subview_stripable->phase_control ();
			screen_hold = true;
			break;
		default:
			break;
	}

	if (control) {
		float val = control->get_value();

		/* Note: all of the displayed controllables require the display
		 * of their *actual* ("internal") value, not the version mapped
		 * into the normalized 0..1.0 ("interface") range.
		 */

		//do_parameter_display (control->desc(), val, screen_hold);
		{
			pending_display[1] = Strip::format_paramater_for_display(control->desc(), val, strip->stripable(), screen_hold);
			DEBUG_TRACE (DEBUG::MackieControl, pending_display[1]);
	
			if (screen_hold) {
				/* we just queued up a parameter to be displayed.
					1 second from now, switch back to vpot mode display.
				*/
				strip->block_vpot_mode_display_for (1000);
			}
		}
		
		/* update pot/encoder */
		strip->surface()->write (vpot->set (control->internal_to_interface (val), true, Pot::wrap));
	}
}



PluginSubview::PluginSubview(MackieControlProtocol& mcp, boost::shared_ptr<ARDOUR::Stripable> subview_stripable) 
	: Subview(mcp, subview_stripable)
{
	_plugin_subview_state = boost::make_shared<PluginSelect>(*this);
}

PluginSubview::~PluginSubview() 
{}

bool PluginSubview::subview_mode_would_be_ok (boost::shared_ptr<ARDOUR::Stripable> r, std::string& reason_why_not) 
{
	if (r) {
		boost::shared_ptr<Route> route = boost::dynamic_pointer_cast<Route> (r);
		if (route && route->nth_plugin(0)) {
			return true;
		}
	}
	
	reason_why_not = "no plugins in selected track/bus";
	return false;
}

void PluginSubview::update_global_buttons() 
{
	_mcp.update_global_button (Button::Send, off);
	_mcp.update_global_button (Button::Plugin, on);
	_mcp.update_global_button (Button::Eq, off);
	_mcp.update_global_button (Button::Dyn, off);
	_mcp.update_global_button (Button::Track, off);
	_mcp.update_global_button (Button::Pan, off);
}

void PluginSubview::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2])
{
	const uint32_t global_strip_position = _mcp.global_index (*strip);
	store_pointers(strip, vpot, pending_display, global_strip_position);
	_plugin_subview_state->setup_vpot(strip, vpot, pending_display, global_strip_position, _subview_stripable);
}

void PluginSubview::handle_vselect_event(uint32_t global_strip_position) 
{
	_plugin_subview_state->handle_vselect_event(global_strip_position, _subview_stripable);
}

bool PluginSubview::handle_cursor_right_press() 
{
	return _plugin_subview_state->handle_cursor_right_press();
}
	
bool PluginSubview::handle_cursor_left_press()
{
	return _plugin_subview_state->handle_cursor_left_press();
}

void PluginSubview::set_state(boost::shared_ptr<PluginSubviewState> new_state)
{
	_plugin_subview_state = new_state;

	const uint32_t num_strips = _strips_over_all_surfaces.size();
	for (uint32_t strip_index = 0; strip_index < num_strips; strip_index++)
	{
		Strip* strip = 0;
		Pot* vpot = 0;
		std::string* pending_display = 0;
		if (!retrieve_pointers(&strip, &vpot, &pending_display, strip_index))
		{
			return;
		}
		
		if (!strip || !vpot || !pending_display)
		{
			return;
		}
		_plugin_subview_state->setup_vpot(strip, vpot, pending_display, strip_index, _subview_stripable);
	}
}




PluginSubviewState::PluginSubviewState(PluginSubview& context)
  : _context(context)
  , _bank_size(context.mcp().n_strips())
  , _current_bank(0)
{
}

PluginSubviewState::~PluginSubviewState()
{}

std::string
PluginSubviewState::shorten_display_text(const std::string& text, std::string::size_type target_length) 
{
	if (text.length() <= target_length) {
		return text;
	} 
	
	return PBD::short_version (text, target_length);
}

bool PluginSubviewState::handle_cursor_right_press() 
{
	_current_bank = _current_bank + 1;
	bank_changed();
	return true;
}
	
bool PluginSubviewState::handle_cursor_left_press()
{
	if (_current_bank >= 1)
	{
		_current_bank = _current_bank - 1;
	}
	bank_changed();
	return true;
}

uint32_t PluginSubviewState::calculate_virtual_strip_position(uint32_t strip_index)
{
	DEBUG_TRACE (DEBUG::MackieControl, string_compose ("virtual strip = %1 * %2 + %3 = %4\n",
								   _current_bank, _bank_size, strip_index, _current_bank * _bank_size + strip_index));
	return _current_bank * _bank_size + strip_index;
}



PluginSelect::PluginSelect(PluginSubview& context)
  : PluginSubviewState(context)
{}

PluginSelect::~PluginSelect()
{}

void PluginSelect::setup_vpot(
	    Strip* strip,
		Pot* vpot, 
		std::string pending_display[2],
		uint32_t global_strip_position,
		boost::shared_ptr<ARDOUR::Stripable> subview_stripable)
{
	if (!subview_stripable) {
		return;
	}
	
	boost::shared_ptr<Route> route = boost::dynamic_pointer_cast<Route> (subview_stripable);
	if (!route) {
		return;
	}

	uint32_t virtual_strip_position = calculate_virtual_strip_position(global_strip_position);
	
	boost::shared_ptr<Processor> plugin = route->nth_plugin(virtual_strip_position);
	
	if (plugin) {
		DEBUG_TRACE (DEBUG::MackieControl, string_compose ("plugin of strip %1 is %2\n", global_strip_position, plugin->display_name()));
		pending_display[0] = string_compose("Ins%1Pl", virtual_strip_position + 1);
		pending_display[1] = PluginSubviewState::shorten_display_text(plugin->display_name(), 6);
	}
	else {
		pending_display[0] = "";
		pending_display[1] = "";
	}
}

void PluginSelect::handle_vselect_event(uint32_t global_strip_position,
		boost::shared_ptr<ARDOUR::Stripable> subview_stripable)
{
	/* PluginSelect mode: press selects the plugin shown on the strip's LCD */
	if (!subview_stripable) {
		return;
	}
	
	boost::shared_ptr<Route> route = boost::dynamic_pointer_cast<Route> (subview_stripable);
	if (!route) {
		return;
	}

	uint32_t virtual_strip_position = calculate_virtual_strip_position(global_strip_position);
	
	boost::shared_ptr<Processor> processor = route->nth_plugin(virtual_strip_position);
	boost::shared_ptr<PluginInsert> plugin = boost::dynamic_pointer_cast<PluginInsert>(processor);
	processor->ShowUI();
	if (plugin) {
		_context.set_state(boost::make_shared<PluginEdit>(_context, plugin));
	}
}

void PluginSelect::bank_changed()
{
	_context.mcp().redisplay_subview_mode();
}



PluginEdit::PluginEdit(PluginSubview& context, boost::shared_ptr<PluginInsert> subview_plugin)
  : PluginSubviewState(context)
  , _subview_plugin_insert(subview_plugin)
{
	init(subview_plugin);
}

PluginEdit::~PluginEdit() 
{}

void PluginEdit::init(boost::shared_ptr<PluginInsert> plugin_insert)
{
	_subview_plugin = plugin_insert->plugin();
	_plugin_input_parameter_indices.clear();

	bool ok = false;
	// put only input controls into a vector
	uint32_t nplug_params = _subview_plugin->parameter_count();
	for (uint32_t ppi = 0; ppi < nplug_params; ++ppi) {
		uint32_t controlid = _subview_plugin->nth_parameter(ppi, ok);
		if (!ok) {
			continue;
		}
		if (_subview_plugin->parameter_is_input(controlid)) {
			_plugin_input_parameter_indices.push_back(ppi);
		}
	}
}

void PluginEdit::setup_vpot(
		Strip* strip,
		Pot* vpot, 
		std::string pending_display[2],
		uint32_t global_strip_position,
		boost::shared_ptr<ARDOUR::Stripable> subview_stripable)
{
	uint32_t virtual_strip_position = calculate_virtual_strip_position(global_strip_position);

	if (virtual_strip_position < _plugin_input_parameter_indices.size()) {
		uint32_t plugin_parameter_index = _plugin_input_parameter_indices[virtual_strip_position];
		bool ok = false;
		uint32_t controlid = _subview_plugin->nth_parameter(plugin_parameter_index, ok);
		if (!ok) {
			vpot->set_control (boost::shared_ptr<AutomationControl>());
			pending_display[0] = std::string();
			pending_display[1] = std::string();
		}

		boost::shared_ptr<AutomationControl> c = _subview_plugin_insert->automation_control(Evoral::Parameter(PluginAutomation, 0, controlid));
		//If a controllable was found, connect it up, and put the labels in the display.
		if (c) {
			c->Changed.connect (_context.subview_connections(), MISSING_INVALIDATOR, boost::bind (&PluginEdit::notify_parameter_change, this, strip, vpot, pending_display, global_strip_position), ui_context());
			vpot->set_control (c);
			ParameterDescriptor pd;
			_subview_plugin->get_parameter_descriptor(controlid, pd);
			pending_display[0] = PluginSubviewState::shorten_display_text(pd.label, 6);	
		} else {  //no controllable was found;  just clear this knob
			vpot->set_control (boost::shared_ptr<AutomationControl>());
			pending_display[0] = std::string();
			pending_display[1] = std::string();
			return;
		}

		notify_parameter_change (strip, vpot, pending_display, global_strip_position);
	}
	else {
		pending_display[0] = "";
		pending_display[1] = "";
	}
}


void PluginEdit::notify_parameter_change(Strip* strip, Pot* vpot, std::string pending_display[2], uint32_t /*global_strip_position*/) 
{
	boost::shared_ptr<AutomationControl> control = vpot->control();
	if (!control) 
	{
		return;
	}

	float val = control->get_value();
	{
		bool screen_hold = false;
		pending_display[1] = Strip::format_paramater_for_display(control->desc(), val, _context.subview_stripable(), screen_hold);
	}
	
	/* update pot/encoder */
	strip->surface()->write(vpot->set (control->internal_to_interface (val), true, Pot::wrap));
}

void PluginEdit::handle_vselect_event(uint32_t global_strip_position, boost::shared_ptr<ARDOUR::Stripable> subview_stripable)
{
}

void PluginEdit::bank_changed()
{
	_context.mcp().redisplay_subview_mode();
}

