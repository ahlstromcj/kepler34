#pragma once

class MidiPerformance;

#include "Globals.hpp"
#include "MidiEvent.hpp"
#include "MidiBus.hpp"
#include "MidiFile.hpp"
#include "MidiSequence.hpp"

#ifndef __WIN32__
#   include <unistd.h>
#endif
#include <pthread.h>

#include <QCoreApplication>
#include <QKeySequence>
#include <QDebug>

/* if we have jack, include the jack headers */
#ifdef JACK_SUPPORT
#include <jack/jack.h>
#include <jack/transport.h>
#ifdef JACK_SESSION
#include <jack/session.h>
#endif
#endif

class MidiControl
{
public:

    bool m_active;
    bool m_inverse_active;
    long m_status;
    long m_data;
    long m_min_value;
    long m_max_value;
};


const int c_status_replace  = 0x01;
const int c_status_snapshot = 0x02;
const int c_status_queue    = 0x04;
const int c_status_oneshot  = 0x08;

const int c_midi_track_ctrl = cSeqsInBank * 2;
const int c_midi_control_bpm_up       = c_midi_track_ctrl ;
const int c_midi_control_bpm_dn       = c_midi_track_ctrl + 1;
const int c_midi_control_ss_up        = c_midi_track_ctrl + 2;
const int c_midi_control_ss_dn        = c_midi_track_ctrl + 3;
const int c_midi_control_mod_replace  = c_midi_track_ctrl + 4;
const int c_midi_control_mod_snapshot = c_midi_track_ctrl + 5;
const int c_midi_control_mod_queue    = c_midi_track_ctrl + 6;
//andy midi_control_mod_mute_group
const int c_midi_control_mod_gmute    = c_midi_track_ctrl + 7;
//andy learn_mute_toggle_mode
const int c_midi_control_mod_glearn   = c_midi_track_ctrl + 8;
//andy play only this screen set
const int c_midi_control_play_ss      = c_midi_track_ctrl + 9;
const int c_midi_controls             = c_midi_track_ctrl + 10;//7


struct performcallback
{
    virtual void on_grouplearnchange(bool) {}
};

///
/// \brief The MidiPerformance class
///
/// Holds the set of sequences making up a song,
/// along with their surrounding data, and controls
/// their playback

class MidiPerformance
{
public:

    bool m_show_ui_sequence_key;

private:
    //the global current tick
    //moved out of output func so we can set position
    double current_tick;

    //andy mute group
    bool m_mute_group[c_gmute_tracks];
    bool m_tracks_mute_state[cSeqsInBank];
    bool m_mode_group;
    bool m_mode_group_learn;
    int m_mute_group_selected;
    //andy playing screen
    int m_playing_screen;

    /* vector of sequences */
    MidiSequence *m_seqs    [ c_max_sequence ];

    /* holds whether each sequence is active */
    bool m_seqs_active      [ c_max_sequence ];

    bool m_was_active_main  [ c_max_sequence ];
    bool m_was_active_edit  [ c_max_sequence ];
    bool m_was_active_perf  [ c_max_sequence ];
    bool m_was_active_names [ c_max_sequence ];
    bool m_sequence_state   [ c_max_sequence ];

    /* our midibus */
    MasterMidiBus m_master_bus;

    /* pthread info */
    pthread_t m_out_thread;
    pthread_t m_in_thread;
    bool m_out_thread_launched;
    bool m_in_thread_launched;

    bool m_running;
    bool m_inputing;
    bool m_outputing;
    bool m_looping;
    bool m_song_recording; //record live seq changes into
                            //the song data
    bool mSongRecordSnap; //snap recorded playback changes
                           //to seq length
    bool mResumeNoteOns; //whether to resume notes if the seq is toggled
                        //after note on

    /* whether we're in live or song mode */
    //TODO replace with enum
    bool m_playback_mode;

    bool m_modified;

    int thread_trigger_width_ms;

    long m_left_tick;
    long m_right_tick;
    long m_starting_tick;

    long m_tick;
    bool m_usemidiclock;
    bool m_midiclockrunning; // stopped or started
    int  m_midiclocktick;
    int  m_midiclockpos;

    void set_running( bool a_running );

    string m_screen_set_notepad[c_max_num_banks];

    MidiControl m_midi_cc_toggle[ c_midi_controls];
    MidiControl m_midi_cc_on[ c_midi_controls];
    MidiControl m_midi_cc_off[ c_midi_controls];

    int m_offset;
    int m_control_status; //TODO replace with enum
    int m_screen_set;
    int mEditorKeyHeight; //height of keys in the sequence editor
    int mEditorKeyboardHeight;

    condition_var m_condition_var;

    // do not access these directly, use set/lookup below
    std::map<int,long> key_events;
    std::map<int,long> key_groups;

    // reverse lookup, keep these in sync!!
    std:: map<long, int> key_events_rev;
    std:: map<long, int> key_groups_rev;

    thumb_colours_e mSequenceColours[c_max_sequence];
    edit_mode_e     mEditModes[c_max_sequence];

#ifdef JACK_SUPPORT

    jack_client_t *m_jack_client;
    jack_nframes_t m_jack_frame_current,
    m_jack_frame_last;
    jack_position_t m_jack_pos;
    jack_transport_state_t m_jack_transport_state;
    jack_transport_state_t m_jack_transport_state_last;
    double m_jack_tick;
#ifdef JACK_SESSION
public:
    jack_session_event_t *m_jsession_ev;
    bool jack_session_event();
private:
#endif
#endif

    bool m_jack_running;
    bool m_jack_master;

    void inner_start();
    void inner_stop();

public:
    bool is_running();
    bool is_learn_mode() const { return m_mode_group_learn; }

    /* get or set the playback mode
     * (i.e. do we/don't we play the song editor's data) */
    bool get_playback_mode();
    void set_playback_mode( bool a_playback_mode );

    /* start/stop recording live sequence changes
     * into the song data */
    void set_song_recording(bool new_state);
    bool get_song_recording();

    //set the colour used to represent the specified sequence
    void setSequenceColour(int seqId, thumb_colours_e newColour);
    thumb_colours_e getSequenceColour(int seqId);

    //get/set the editing mode of this seq
    void setEditMode(int seqId, edit_mode_e newMode);
    edit_mode_e getEditMode(int seqId);

    // can register here for events...
    std::vector<performcallback*> m_notify;

    int m_key_bpm_up;
    int m_key_bpm_dn;

    string m_key_keep_queue;
    string m_key_replace;
    string m_key_queue;
    string m_key_snapshot_1;
    string m_key_snapshot_2;

    int m_key_screenset_up;
    int m_key_screenset_dn;
    int m_key_set_playing_screenset;

    int m_key_group_on;
    int m_key_group_off;
    int m_key_group_learn;

    int mKeyTogglePlay;
    int mKeyRecord;

    bool show_ui_sequence_key() const { return m_show_ui_sequence_key; }


    MidiPerformance();
    ~MidiPerformance();

    void init();

    void clear_all();

    void launch_input_thread();
    void launch_output_thread();
    void init_jack();
    void deinit_jack();

    void add_sequence( MidiSequence *a_seq, int a_perf );
    void delete_sequence( int a_num );
    bool is_sequence_in_edit( int a_num );

    void clear_sequence_triggers( int a_seq  );

    long get_tick( ) { return m_tick; }

    void set_left_tick( long a_tick );
    long get_left_tick();

    void set_starting_tick( long a_tick );
    long get_starting_tick();

    void set_right_tick( long a_tick );
    long get_right_tick();

    void move_triggers( bool a_direction );
    void copy_triggers(  );

    void push_trigger_undo();
    void pop_trigger_undo();

    void push_trigger_redo();
    void pop_trigger_redo();

    void print();

    //when in panic, send NOTE_OFF on all keys on all channels
    void panic();

    MidiControl *get_midi_control_toggle( unsigned int a_seq );
    MidiControl *get_midi_control_on( unsigned int a_seq );
    MidiControl *get_midi_control_off( unsigned int a_seq );

    void handle_midi_control( int a_control, bool a_state );

    void setBankName( int bankNum, string *a_note );
    string *getBankName( int bank_num );

    void setBank( int newBank );
    int  getBank();

    void setPlayingBank();
    int getPlayingBank();

    void mute_group_tracks ();
    void select_and_mute_group (int a_g_group);
    void set_mode_group_mute ();
    void select_group_mute (int a_g_mute);
    void set_mode_group_learn ();
    void unset_mode_group_learn ();
    bool is_group_learning(void) { return m_mode_group_learn; }
    void select_mute_group ( int a_group );
    void unset_mode_group_mute ();

    void start();
    void stop();

    void start_jack();
    void stop_jack();
    void position_jack(long tick = 0);

    void off_sequences();
    void all_notes_off();

    void set_active(int a_sequence, bool a_active);
    void set_was_active( int a_sequence );

    //returns if this a valid sequence ID
    bool is_active(int a_sequence);

    bool is_dirty_main (int a_sequence);
    bool is_dirty_edit (int a_sequence);
    bool is_dirty_perf (int a_sequence);
    bool is_dirty_names (int a_sequence);

    void new_sequence( int a_sequence );

    /* plays all notes to current tick */
    void play( long a_tick );

    void set_orig_ticks( long a_tick  );

    MidiSequence * get_sequence( int MidiSequence );

    void reset_sequences();

    void set_bpm(int a_bpm);
    int  get_bpm( );

    void set_looping( bool a_looping ){ m_looping = a_looping; }

    /* set/unset the control status of the main window.
     *
     * Set with byte values. These can be:
     * - Replace    (c_status_replace)
     * - Queue      (c_status_queue)
     * - Snapshot   (c_status_snapshot)
     * - One-shot   (c_status_oneshot)
     *
     * constants representing these are up top ^^^ */
    void set_sequence_control_status( int a_status );
    void unset_sequence_control_status( int a_status );

    /* toggle the plackback state of a seq. Used for live input */
    void sequence_playing_toggle(int seqId );

    void sequence_playing_on( int a_sequence );
    void sequence_playing_off( int a_sequence );

    void set_group_mute_state (int a_g_track, bool a_mute_state);
    bool get_group_mute_state (int a_g_track);
    void mute_all_tracks();

    MasterMidiBus* get_master_midi_bus( );

    void output_func();
    void input_func();

    long get_max_trigger();

    void selectTriggersInRange(int seqL, int seqH, long tickS, long tickF);
    void unselectAllTriggers();

    void set_offset( int a_offset );

    void save_playing_state();
    void restore_playing_state();

    bool getModified();
    void setModified(bool modified);

    const std::map<int,long> *get_key_events(void) const
    {
        return &key_events;
    }
    const std::map<int,long> *get_key_groups(void) const
    {
        return &key_groups;
    }

    void set_key_event( int keycode, long sequence_slot );
    void set_key_group( int keycode, long group_slot );

    // getters of keyboard mapping for sequence and groups,
    // if not found, returns something "safe" (so use get_key()->count() to see if it's there first)
    int lookup_keyevent_key(long seqnum)
    {
        if (key_events_rev.count( seqnum ))
            return key_events_rev[seqnum];
        else
            return '?';
    }
    long lookup_keyevent_seq(int keycode)
    {
        if (key_events.count( keycode ))
            return key_events[keycode];
        else
            return 0;
    }
    int lookup_keygroup_key(long groupnum) {
        if (key_groups_rev.count(groupnum))
            return key_groups_rev[groupnum];
        else
            return '?';
    }
    long lookup_keygroup_group(int keycode)
    {
        if (key_groups.count(keycode))
            return key_groups[keycode];
        else
            return 0;
    }

    friend class MidiFile;
    friend class PreferencesFile;
    friend class PreferencesDialog;

#ifdef JACK_SUPPORT

    friend int jack_sync_callback(jack_transport_state_t state,
                                  jack_position_t *pos, void *arg);
    friend void jack_shutdown(void *arg);
    friend void jack_timebase_callback(jack_transport_state_t state, jack_nframes_t nframes,
                                       jack_position_t *pos, int new_pos, void *arg);
#endif
    bool getSongRecordSnap() const;
    void setSongRecordSnap(bool songRecordSnap);
    bool getResumeNoteOns() const;
    void setResumeNoteOns(bool value);
    int getKeyTogglePlay() const;
    void setKeyTogglePlay(int keyTogglePlay);
    int getEditorKeyHeight() const;
    void setEditorKeyHeight(int editorKeyHeight);
    int getEditorKeyboardHeight() const;

    void setTick(long tick);
};

/* located in perform.C */
extern void *output_thread_func(void *a_p);
extern void *input_thread_func(void *a_p);

#ifdef JACK_SUPPORT

int jack_sync_callback(jack_transport_state_t state,
                       jack_position_t *pos, void *arg);
void print_jack_pos( jack_position_t* jack_pos );
void jack_shutdown(void *arg);
void jack_timebase_callback(jack_transport_state_t state, jack_nframes_t nframes,
                            jack_position_t *pos, int new_pos, void *arg);
int jack_process_callback(jack_nframes_t nframes, void* arg);
#ifdef JACK_SESSION
void jack_session_callback(jack_session_event_t *ev, void *arg);
#endif
#endif


