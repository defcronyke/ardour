/*
 * Copyright (C) 2006-2016 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2009-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2013 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2017-2019 Robin Gareus <robin@gareus.org>
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

#ifndef __ardour_midi_playlist_h__
#define __ardour_midi_playlist_h__

#include <vector>
#include <list>

#include <boost/utility.hpp>

#include "ardour/ardour.h"
#include "ardour/midi_cursor.h"
#include "ardour/midi_model.h"
#include "ardour/midi_state_tracker.h"
#include "ardour/note_fixer.h"
#include "ardour/playlist.h"
#include "evoral/Note.hpp"
#include "evoral/Parameter.hpp"

namespace Evoral {
template<typename Time> class EventSink;
class                         Beats;
}

namespace ARDOUR
{

class BeatsSamplesConverter;
class MidiChannelFilter;
class MidiRegion;
class Session;
class Source;

template<typename T> class MidiRingBuffer;

class LIBARDOUR_API MidiPlaylist : public ARDOUR::Playlist
{
public:
	MidiPlaylist (Session&, const XMLNode&, bool hidden = false);
	MidiPlaylist (Session&, std::string name, bool hidden = false);
	MidiPlaylist (boost::shared_ptr<const MidiPlaylist> other, std::string name, bool hidden = false);

	/** This constructor does NOT notify others (session) */
	MidiPlaylist (boost::shared_ptr<const MidiPlaylist> other,
	              samplepos_t                           start,
	              samplecnt_t                           cnt,
	              std::string                           name,
	              bool                                  hidden = false);

	~MidiPlaylist ();

	/** Read a range from the playlist into an event sink.
	 *
	 * @param buf Destination for events.
	 * @param start First sample of read range.
	 * @param cnt Number of samples in read range.
	 * @param loop_range If non-null, all event times will be mapped into this loop range.
	 * @param chan_n Must be 0 (this is the audio-style "channel", where each
	 * channel is backed by a separate region, not MIDI channels, which all
	 * exist in the same region and are not handled here).
	 * @return The number of samples read (time, not an event count).
	 */
	samplecnt_t read (Evoral::EventSink<samplepos_t>& buf,
	                 samplepos_t                      start,
	                 samplecnt_t                      cnt,
	                 Evoral::Range<samplepos_t>*      loop_range,
	                 uint32_t                         chan_n = 0,
	                 MidiChannelFilter*               filter = NULL);

	int set_state (const XMLNode&, int version);

	bool destroy_region (boost::shared_ptr<Region>);
	void _split_region (boost::shared_ptr<Region>, const MusicSample& position);

	void set_note_mode (NoteMode m) { _note_mode = m; }

	std::set<Evoral::Parameter> contained_automation();

	/** Handle a region edit during read.
	 *
	 * This must be called before the command is applied to the model.  Events
	 * are injected into the playlist output to compensate for edits to active
	 * notes and maintain coherent output and tracker state.
	 */
	void region_edited(boost::shared_ptr<Region>         region,
	                   const MidiModel::NoteDiffCommand* cmd);

	/** Clear all note trackers. */
	void reset_note_trackers ();

	/** Resolve all pending notes and clear all note trackers.
	 *
	 * @param dst Sink to write note offs to.
	 * @param time Time stamp of all written note offs.
	 */
	void resolve_note_trackers (Evoral::EventSink<samplepos_t>& dst, samplepos_t time);

protected:
	void remove_dependents (boost::shared_ptr<Region> region);
	void region_going_away (boost::weak_ptr<Region> region);

private:
	typedef Evoral::Note<Temporal::Beats> Note;
	typedef Evoral::Event<samplepos_t>   Event;

	struct RegionTracker : public boost::noncopyable {
		MidiCursor       cursor;   ///< Cursor (iterator and read state)
		MidiStateTracker tracker;  ///< Active note tracker
		NoteFixer        fixer;    ///< Edit compensation
	};

	typedef std::map< Region*, boost::shared_ptr<RegionTracker> > NoteTrackers;

	void dump () const;

	NoteTrackers _note_trackers;
	NoteMode     _note_mode;
	samplepos_t  _read_end;
};

} /* namespace ARDOUR */

#endif /* __ardour_midi_playlist_h__ */
