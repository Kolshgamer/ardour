/*
 * Copyright (C) 2023 Paul Davis <paul@linuxaudiosystems.com>
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

#include "pbd/compose.h"
#include "pbd/debug.h"
#include "pbd/pthread_utils.h"
#include "pbd/semutils.h"

#include "ardour/audio_buffer.h"
#include "ardour/audiofilesource.h"
#include "ardour/butler.h"
#include "ardour/cliprec.h"
#include "ardour/debug.h"
#include "ardour/midi_track.h"
#include "ardour/session.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace PBD;

PBD::Thread* ClipRecProcessor::_thread (0);
bool ClipRecProcessor::thread_should_run (false);
PBD::Semaphore* ClipRecProcessor::_semaphore (0);

ClipRecProcessor::ClipRecProcessor (Session& s, Track& t, std::string const & name)
	: DiskIOProcessor (s, t,name, DiskIOProcessor::Recordable, Temporal::BeatTime)
	, _armed (false)
{
	if (!_thread) {
		thread_should_run = true;
		_semaphore = new PBD::Semaphore (X_("cliprec"), 0);
		_thread = PBD::Thread::create (&ClipRecProcessor::thread_work);
	}
}

void
ClipRecProcessor::set_armed (bool yn)
{
	if (_armed == yn) {
		if (_armed) {
			assert (currently_recording == this);
		}
		return;
	}

	if (!yn) {
		finish_recording ();
		assert (currently_recording == this);
		_armed = false;
		currently_recording = 0;
		ArmedChanged (); // EMIT SIGNAL
		return;
	}

	if (currently_recording) {
		currently_recording->set_armed (false);
		currently_recording = 0;
	}

	_armed = true;
	currently_recording = this;
	start_recording ();
	ArmedChanged (); // EMIT SIGNAL
}

void
ClipRecProcessor::start_recording ()
{
}

void
ClipRecProcessor::finish_recording ()
{
	boost::shared_ptr<ChannelList> c = channels.reader();
	for (auto & chan : *c) {
		Source::WriterLock lock((chan)->write_source->mutex());
		(chan)->write_source->mark_streaming_write_completed (lock);
		(chan)->write_source->done_with_peakfile_writes ();
	}

}

void
ClipRecProcessor::thread_work ()
{
	while (thread_should_run) {
		_semaphore->wait ();
		ClipRecProcessor* crp = currently_recording;
		if (crp) {
			(void) crp->pull_data ();
		}
	}
}

int
ClipRecProcessor::pull_data ()
{
	uint32_t to_write;
	int ret = 0;
	RingBufferNPT<Sample>::rw_vector vector;

	vector.buf[0] = 0;
	vector.buf[1] = 0;

	boost::shared_ptr<ChannelList> c = channels.reader();
	for (ChannelList::iterator chan = c->begin(); chan != c->end(); ++chan) {

		(*chan)->wbuf->get_read_vector (&vector);

		if (vector.len[0] + vector.len[1] == 0) {
			goto out;
		}

		to_write = vector.len[0];

		if ((!(*chan)->write_source) || (*chan)->write_source->write (vector.buf[0], to_write) != to_write) {
			error << string_compose(_("AudioDiskstream %1: cannot write to disk"), id()) << endmsg;
			return -1;
		}

		(*chan)->wbuf->increment_read_ptr (to_write);
		// (*chan)->curr_capture_cnt += to_write;

		to_write = vector.len[1];

		DEBUG_TRACE (DEBUG::ClipRecording, string_compose ("%1 additional write of %2\n", name(), to_write));

		if ((*chan)->write_source->write (vector.buf[1], to_write) != to_write) {
			error << string_compose(_("AudioDiskstream %1: cannot write to disk"), id()) << endmsg;
			return -1;
		}

		(*chan)->wbuf->increment_read_ptr (to_write);
		//(*chan)->curr_capture_cnt += to_write;
	}

#if 0
	/* MIDI*/

	if (_midi_write_source && _midi_buf) {

		const samplecnt_t total = g_atomic_int_get(&_samples_pending_write);

		if (total == 0 ||
		    _midi_buf->read_space() == 0 ||
		    (!force_flush && (total < _chunk_samples) && _was_recording)) {
			goto out;
		}

		/* if there are 2+ chunks of disk i/o possible for
		   this track), let the caller know so that it can arrange
		   for us to be called again, ASAP.

		   if we are forcing a flush, then if there is* any* extra
		   work, let the caller know.

		   if we are no longer recording and there is any extra work,
		   let the caller know too.
		*/

		if (total >= 2 * _chunk_samples || ((force_flush || !_was_recording) && total > _chunk_samples)) {
			ret = 1;
		}

		if (force_flush) {
			/* push out everything we have, right now */
			to_write = UINT32_MAX;
		} else {
			to_write = _chunk_samples;
		}

		if ((total > _chunk_samples) || force_flush) {
			Source::WriterLock lm(_midi_write_source->mutex());
			if (_midi_write_source->midi_write (lm, *_midi_buf, timepos_t (get_capture_start_sample (0)), timecnt_t (to_write)) != to_write) {
				error << string_compose(_("MidiDiskstream %1: cannot write to disk"), id()) << endmsg;
				return -1;
			}
			g_atomic_int_add(&_samples_pending_write, -to_write);
		}
	}
#endif

  out:
	return ret;
}

bool
ClipRecProcessor::can_support_io_configuration (const ChanCount& in, ChanCount& out)
{
	if (in.n_midi() != 0 && in.n_midi() != 1) {
		/* we only support zero or 1 MIDI stream */
		return false;
	}

	/* currently no way to deliver different channels that we receive */
	out = in;

	return true;
}

void
ClipRecProcessor::run (BufferSet& bufs, samplepos_t start_sample, samplepos_t end_sample, double speed, pframes_t nframes, bool result_required)
{
	if (!check_active()) {
		return;
	}

	const size_t n_buffers = bufs.count().n_audio();
	boost::shared_ptr<ChannelList> c = channels.reader();
	ChannelList::iterator chan;
	size_t n;

	if (n_buffers) {

		/* AUDIO */

		for (chan = c->begin(), n = 0; chan != c->end(); ++chan, ++n) {

			ChannelInfo* chaninfo (*chan);
			AudioBuffer& buf (bufs.get_audio (n%n_buffers));

			chaninfo->wbuf->get_write_vector (&chaninfo->rw_vector);

			if (nframes <= (samplecnt_t) chaninfo->rw_vector.len[0]) {

				Sample *incoming = buf.data ();
				memcpy (chaninfo->rw_vector.buf[0], incoming, sizeof (Sample) * nframes);

			} else {

				samplecnt_t total = chaninfo->rw_vector.len[0] + chaninfo->rw_vector.len[1];

				if (nframes > total) {
					DEBUG_TRACE (DEBUG::ClipRecording, string_compose ("%1 overrun in %2, rec_nframes = %3 total space = %4\n",
					                                            DEBUG_THREAD_SELF, name(), nframes, total));
					return;
				}

				Sample *incoming = buf.data ();
				samplecnt_t first = chaninfo->rw_vector.len[0];

				memcpy (chaninfo->rw_vector.buf[0], incoming, sizeof (Sample) * first);
				memcpy (chaninfo->rw_vector.buf[1], incoming + first, sizeof (Sample) * (nframes - first));
			}

			chaninfo->wbuf->increment_write_ptr (nframes);

			if (chaninfo->wbuf->read_space() > 10) {
				_semaphore->signal ();
			}
		}
	}

	/* MIDI */

	if (_midi_buf) {

		// Pump entire port buffer into the ring buffer (TODO: split cycles?)
		MidiBuffer& buf    = bufs.get_midi (0);
		MidiTrack* mt = dynamic_cast<MidiTrack*>(&_track);
		MidiChannelFilter* filter = mt ? &mt->capture_filter() : 0;

		assert (buf.size() == 0 || _midi_buf);

		for (MidiBuffer::iterator i = buf.begin(); i != buf.end(); ++i) {
			Evoral::Event<MidiBuffer::TimeType> ev (*i, false);
			if (ev.time() > nframes) {
				break;
			}

			bool skip_event = false;

			if (mt) {
				/* skip injected immediate/out-of-band events */
				MidiBuffer const& ieb (mt->immediate_event_buffer());
				for (MidiBuffer::const_iterator j = ieb.begin(); j != ieb.end(); ++j) {
					if (*j == ev) {
						skip_event = true;
					}
				}
			}

			if (!skip_event && (!filter || !filter->filter(ev.buffer(), ev.size()))) {
				const samplepos_t event_time = start_sample + ev.time();
				_midi_buf->write (event_time, ev.event_type(), ev.size(), ev.buffer());
			}
		}
	}
}

float
ClipRecProcessor::buffer_load () const
{
	boost::shared_ptr<ChannelList> c = channels.reader();

	if (c->empty ()) {
		return 1.0;
	}

	return (float) ((double) c->front()->wbuf->write_space()/
			(double) c->front()->wbuf->bufsize());
}

void
ClipRecProcessor::adjust_buffering ()
{
	boost::shared_ptr<ChannelList> c = channels.reader();

	for (ChannelList::iterator chan = c->begin(); chan != c->end(); ++chan) {
		(*chan)->resize (_session.butler()->audio_capture_buffer_size());
	}
}

void
ClipRecProcessor::configuration_changed ()
{
	/* nothing to do */
}



