/* main window */

#include <cctype>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <gtk/gtkversion.h>

#include "mainwnd.h"
#include "perform.h"
#include "midifile.h"
#include "perfedit.h"

#include "pixmaps/play2.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/learn.xpm"
#include "pixmaps/learn2.xpm"
#include "pixmaps/perfedit.xpm"
#include "pixmaps/seq24.xpm"
#include "pixmaps/seq24_32.xpm"

bool is_pattern_playing = false;

// tooltip helper, for old vs new gtk...
#if GTK_MINOR_VERSION >= 12
#   define add_tooltip( obj, text ) obj->set_tooltip_text( text);
#else
#   define add_tooltip( obj, text ) m_tooltips->set_tip( *obj, text );
#endif

mainwnd::mainwnd(perform *a_p):
    m_mainperf(a_p),
    m_modified(false),
    m_options(NULL)
{
    set_icon(Gdk::Pixbuf::create_from_xpm_data(seq24_32_xpm));

    /* register for notification */
    m_mainperf->m_notify.push_back( this );

    /* main window */
    update_window_title();

#if GTK_MINOR_VERSION < 12
    m_tooltips = manage( new Tooltips() );
#endif
    m_main_wid = manage( new mainwid( m_mainperf ));
    m_main_time = manage( new maintime( ));

    m_menubar = manage(new MenuBar());

    m_menu_file = manage(new Menu());
    m_menubar->items().push_front(MenuElem("_File", *m_menu_file));

    m_menu_view = manage( new Menu());
    m_menubar->items().push_back(MenuElem("_View", *m_menu_view));

    m_menu_help = manage( new Menu());
    m_menubar->items().push_back(MenuElem("_Help", *m_menu_help));

    /* file menu items */
    m_menu_file->items().push_back(MenuElem("_New",
                Gtk::AccelKey("<control>N"),
                mem_fun(*this, &mainwnd::file_new)));

    m_menu_file->items().push_back(MenuElem("_Open...",
                Gtk::AccelKey("<control>O"),
                mem_fun(*this, &mainwnd::file_open)));

    /* recent files sub-menu */
    m_menu_file_recent = manage (new Menu());

    /* only add if a path is actually contained in each slot */
    //TODO add the new CtrlR accelerator to documentation
    if (recent_files[0]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[0],
                Gtk::AccelKey("<control>R"),
                mem_fun(*this, &mainwnd::load_recent_1)));
    else
    m_menu_file_recent->items().push_back(MenuElem("(no recent files)"));

    if (recent_files[1]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[1],
                mem_fun(*this, &mainwnd::load_recent_2)));

    if (recent_files[2]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[2],
                mem_fun(*this, &mainwnd::load_recent_3)));

    if (recent_files[3]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[3],
                mem_fun(*this, &mainwnd::load_recent_4)));

    if (recent_files[4]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[4],
                mem_fun(*this, &mainwnd::load_recent_5)));

    if (recent_files[5]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[5],
                mem_fun(*this, &mainwnd::load_recent_6)));

    if (recent_files[6]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[6],
                mem_fun(*this, &mainwnd::load_recent_7)));

    if (recent_files[7]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[7],
                mem_fun(*this, &mainwnd::load_recent_8)));

    if (recent_files[8]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[8],
                mem_fun(*this, &mainwnd::load_recent_9)));

    if (recent_files[9]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[9],
                mem_fun(*this, &mainwnd::load_recent_10)));

    m_menu_file->items().push_back(MenuElem("Open Recent...",
                *m_menu_file_recent));

    m_menu_file->items().push_back(MenuElem("_Save",
                Gtk::AccelKey("<control>S"),
                mem_fun(*this, &mainwnd::file_save)));

    m_menu_file->items().push_back(MenuElem("Save _As...",
                mem_fun(*this, &mainwnd::file_save_as)));

    m_menu_file->items().push_back(SeparatorElem());

    m_menu_file->items().push_back(MenuElem("_Import...",
                mem_fun(*this, &mainwnd::file_import_dialog)));

    m_menu_file->items().push_back(MenuElem("_Preferences...",
                Gtk::AccelKey("<control>P"),
                mem_fun(*this,&mainwnd::prefs_dialog)));

    m_menu_file->items().push_back(SeparatorElem());

    m_menu_file->items().push_back(MenuElem("E_xit",
                Gtk::AccelKey("<control>Q"),
                mem_fun(*this, &mainwnd::file_exit)));

    /* view menu items */
    m_menu_view->items().push_back(MenuElem("_Song Editor...",
                Gtk::AccelKey("<control>E"),
                mem_fun(*this, &mainwnd::open_performance_edit)));

    /* help menu items */
    m_menu_help->items().push_back(MenuElem("_About...",
                mem_fun(*this, &mainwnd::about_dialog)));

    /* original seq24 display mode */
    if (global_display_mode == e_classic_display){

        /* top line items */
        HBox *tophbox = manage( new HBox( false, 0 ) );
        tophbox->pack_start(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data(seq24_xpm))),
            false, false);

        // adjust placement...
        VBox *vbox_b = manage( new VBox() );
        HBox *hbox3 = manage( new HBox( false, 0 ) );
        vbox_b->pack_start( *hbox3, false, false );
        tophbox->pack_end( *vbox_b, false, false );
        hbox3->set_spacing( 10 );

        /* timeline */
        hbox3->pack_start( *m_main_time, false, false );

        /* group learn button */
        m_button_learn = manage( new Button( ));
        m_button_learn->set_focus_on_click( false );
        m_button_learn->set_flags( m_button_learn->get_flags() & ~Gtk::CAN_FOCUS );
        m_button_learn->set_image(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data( learn_xpm ))));
        m_button_learn->signal_clicked().connect(
            mem_fun(*this, &mainwnd::learn_toggle));
        add_tooltip( m_button_learn, "Mute Group Learn\n\n"
            "Click 'L' then press a mutegroup key to store the mute state of "
            "the sequences in that key.\n\n"
            "(see File/Options/Keyboard for available mutegroup keys "
            "and the corresponding hotkey for the 'L' button)" );
        hbox3->pack_start( *m_button_learn, false, false );

        /*this seems to be a dirty hack:*/
        Button w;
        hbox3->set_focus_child( w ); // clear the focus not to trigger L via keys

        /* bottom line items */
        HBox *bottomhbox = manage( new HBox(false, 10));

        /* container for start+stop buttons */
        HBox *startstophbox = manage(new HBox(false, 4));
        bottomhbox->pack_start(*startstophbox, Gtk::PACK_SHRINK);

        /* stop button */
        m_button_stop = manage( new Button());
        m_button_stop->add(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data( stop_xpm ))));
        m_button_stop->signal_clicked().connect(
            mem_fun(*this, &mainwnd::stop_playing));
        add_tooltip( m_button_stop, "Stop playing MIDI sequence" );
        startstophbox->pack_start(*m_button_stop, Gtk::PACK_SHRINK);

        /* play button */
        m_button_play = manage(new Button() );
        m_button_play->add(*manage(new Image(
                    Gdk::Pixbuf::create_from_xpm_data( play2_xpm ))));
        m_button_play->signal_clicked().connect(
            mem_fun( *this, &mainwnd::start_playing));
        add_tooltip( m_button_play, "Play in live mode, ignoring the song data" );
        startstophbox->pack_start(*m_button_play, Gtk::PACK_SHRINK);

        /* bpm spin button with label*/
        HBox *bpmhbox = manage(new HBox(false, 4));
        bottomhbox->pack_start(*bpmhbox, Gtk::PACK_SHRINK);

        m_adjust_bpm = manage(new Adjustment(m_mainperf->get_bpm(), 20, 500, 1));
        m_spinbutton_bpm = manage( new SpinButton( *m_adjust_bpm ));
        m_spinbutton_bpm->set_editable( false );
        m_adjust_bpm->signal_value_changed().connect(
            mem_fun(*this, &mainwnd::adj_callback_bpm));
        add_tooltip( m_spinbutton_bpm, "Adjust beats per minute (BPM) value");
        Label* bpmlabel = manage(new Label("_bpm", true));
        bpmlabel->set_mnemonic_widget(*m_spinbutton_bpm);
        bpmhbox->pack_start(*bpmlabel, Gtk::PACK_SHRINK);
        bpmhbox->pack_start(*m_spinbutton_bpm, Gtk::PACK_SHRINK);

        /* screen set name edit line */
        HBox *notebox = manage(new HBox(false, 4));
        bottomhbox->pack_start(*notebox, Gtk::PACK_EXPAND_WIDGET);

        m_entry_notes = manage( new Entry());
        m_entry_notes->signal_changed().connect(
            mem_fun(*this, &mainwnd::edit_callback_notepad));
        m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset()));
        add_tooltip( m_entry_notes, "Enter screen set name" );
        Label* notelabel = manage(new Label("_Name", true));
        notelabel->set_mnemonic_widget(*m_entry_notes);
        notebox->pack_start(*notelabel, Gtk::PACK_SHRINK);
        notebox->pack_start(*m_entry_notes, Gtk::PACK_EXPAND_WIDGET);

        /* sequence set spin button */
        HBox *sethbox = manage(new HBox(false, 4));
        bottomhbox->pack_start(*sethbox, Gtk::PACK_SHRINK);

        m_adjust_ss = manage( new Adjustment( 0, 0, c_max_sets - 1, 1 ));
        m_spinbutton_ss = manage( new SpinButton( *m_adjust_ss ));
        m_spinbutton_ss->set_editable( false );
        m_spinbutton_ss->set_wrap( true );
        m_adjust_ss->signal_value_changed().connect(
            mem_fun(*this, &mainwnd::adj_callback_ss ));
        add_tooltip( m_spinbutton_ss, "Select screen set" );
        Label* setlabel = manage(new Label("_Set", true));
        setlabel->set_mnemonic_widget(*m_spinbutton_ss);
        sethbox->pack_start(*setlabel, Gtk::PACK_SHRINK);
        sethbox->pack_start(*m_spinbutton_ss, Gtk::PACK_SHRINK);

        /* song edit button */
        m_button_perfedit = manage( new Button( ));
        m_button_perfedit->add( *manage( new Image(
                    Gdk::Pixbuf::create_from_xpm_data( perfedit_xpm  ))));
        m_button_perfedit->signal_clicked().connect(
            mem_fun( *this, &mainwnd::open_performance_edit ));
        add_tooltip( m_button_perfedit, "Show or hide song editor window" );
        bottomhbox->pack_end(*m_button_perfedit, Gtk::PACK_SHRINK);

        /* vertical layout container for window content*/
        vbox_live_tab = new VBox();
        vbox_live_tab->set_spacing(10);
        vbox_live_tab->set_border_width(10);
        vbox_live_tab->pack_start(*tophbox, Gtk::PACK_SHRINK);
        vbox_live_tab->pack_start(*m_main_wid, Gtk::PACK_SHRINK);
        vbox_live_tab->pack_start(*bottomhbox, Gtk::PACK_SHRINK);

        /* main container for menu and window content */
        vbox_main = new VBox();

        vbox_main->pack_start(*m_menubar, false, false );
        vbox_main->pack_start( *vbox_live_tab );

        /* add main layout box */
        this->add (*vbox_main);

        /* show everything */
        show_all();

    } else { /* single window mode */

        /* top line items */
        HBox *hbox_top = manage( new HBox( false, 0 ) );
//        VBox *vbox_top = manage( new VBox( false, 0 ) );

        // adjust placement...
        VBox *vbox_time_mutegroup = manage( new VBox() );
        HBox *hbox_time_mutegroup = manage( new HBox( false, 0 ) );
        vbox_time_mutegroup->pack_start( *hbox_time_mutegroup, false, false );
        hbox_top->pack_end( *vbox_time_mutegroup, false, false );
        hbox_time_mutegroup->set_spacing( 10 );

        /* timeline */
        hbox_time_mutegroup->pack_start( *m_main_time, false, false );

        /* group learn button */
        m_button_learn = manage( new Button( ));
        m_button_learn->set_focus_on_click( false );
        m_button_learn->set_flags( m_button_learn->get_flags() & ~Gtk::CAN_FOCUS );
        m_button_learn->set_image(*manage(new Image(
                        Gdk::Pixbuf::create_from_xpm_data( learn_xpm ))));
        m_button_learn->signal_clicked().connect(
                mem_fun(*this, &mainwnd::learn_toggle));
        add_tooltip( m_button_learn, "Mute Group Learn\n\n"
                "Click 'L' then press a mutegroup key to store the mute state of "
                "the sequences in that key.\n\n"
                "(see File/Options/Keyboard for available mutegroup keys "
                "and the corresponding hotkey for the 'L' button)" );
        hbox_time_mutegroup->pack_start( *m_button_learn, false, false );

        /*this seems to be a dirty hack:*/
        Button w;
        hbox_time_mutegroup->set_focus_child( w ); // clear the focus not to trigger L via keys

        /* bottom line items */
        HBox *hbox_bottom = manage( new HBox(false, 10));

        /* container for start+stop buttons */
        HBox *hbox_start_stop = manage(new HBox(false, 4));
        hbox_bottom->pack_start(*hbox_start_stop, Gtk::PACK_SHRINK);

        /* stop button */
        m_button_stop = manage( new Button());
        m_button_stop->add(*manage(new Image(
                        Gdk::Pixbuf::create_from_xpm_data( stop_xpm ))));
        m_button_stop->signal_clicked().connect(
                mem_fun(*this, &mainwnd::stop_playing));
        add_tooltip( m_button_stop, "Stop playing MIDI sequence" );
        hbox_start_stop->pack_start(*m_button_stop, Gtk::PACK_SHRINK);

        /* play button */
        m_button_play = manage(new Button() );
        m_button_play->add(*manage(new Image(
                        Gdk::Pixbuf::create_from_xpm_data( play2_xpm ))));
        m_button_play->signal_clicked().connect(
                mem_fun( *this, &mainwnd::start_playing));
        add_tooltip( m_button_play, "Play in live mode, ignoring the song data" );
        hbox_start_stop->pack_start(*m_button_play, Gtk::PACK_SHRINK);

        /* bpm spin button with label*/
        HBox *bpmhbox = manage(new HBox(false, 4));
        hbox_bottom->pack_start(*bpmhbox, Gtk::PACK_SHRINK);

        m_adjust_bpm = manage(new Adjustment(m_mainperf->get_bpm(), 20, 500, 1));
        m_spinbutton_bpm = manage( new SpinButton( *m_adjust_bpm ));
        m_spinbutton_bpm->set_editable( false );
        m_adjust_bpm->signal_value_changed().connect(
                mem_fun(*this, &mainwnd::adj_callback_bpm));
        add_tooltip( m_spinbutton_bpm, "Adjust beats per minute (BPM) value");
        Label* bpmlabel = manage(new Label("_bpm", true));
        bpmlabel->set_mnemonic_widget(*m_spinbutton_bpm);
        bpmhbox->pack_start(*bpmlabel, Gtk::PACK_SHRINK);
        bpmhbox->pack_start(*m_spinbutton_bpm, Gtk::PACK_SHRINK);

        /* screen set name edit line */
        HBox *notebox = manage(new HBox(false, 4));
        hbox_bottom->pack_start(*notebox, Gtk::PACK_EXPAND_WIDGET);

        m_entry_notes = manage( new Entry());
        m_entry_notes->signal_changed().connect(
                mem_fun(*this, &mainwnd::edit_callback_notepad));
        m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                    m_mainperf->get_screenset()));
        add_tooltip( m_entry_notes, "Enter screen set name" );
        Label* notelabel = manage(new Label("_Name", true));
        notelabel->set_mnemonic_widget(*m_entry_notes);
        notebox->pack_start(*notelabel, Gtk::PACK_SHRINK);
        notebox->pack_start(*m_entry_notes, Gtk::PACK_EXPAND_WIDGET);

        /* sequence set spin button */
        HBox *sethbox = manage(new HBox(false, 4));
        hbox_bottom->pack_start(*sethbox, Gtk::PACK_SHRINK);

        m_adjust_ss = manage( new Adjustment( 0, 0, c_max_sets - 1, 1 ));
        m_spinbutton_ss = manage( new SpinButton( *m_adjust_ss ));
        m_spinbutton_ss->set_editable( false );
        m_spinbutton_ss->set_wrap( true );
        m_adjust_ss->signal_value_changed().connect(
                mem_fun(*this, &mainwnd::adj_callback_ss ));
        add_tooltip( m_spinbutton_ss, "Select screen set" );
        Label* setlabel = manage(new Label("_Set", true));
        setlabel->set_mnemonic_widget(*m_spinbutton_ss);
        sethbox->pack_start(*setlabel, Gtk::PACK_SHRINK);
        sethbox->pack_start(*m_spinbutton_ss, Gtk::PACK_SHRINK);

        /* song edit button */
//        m_button_perfedit = manage( new Button( ));
//        m_button_perfedit->add( *manage( new Image(
//                        Gdk::Pixbuf::create_from_xpm_data( perfedit_xpm  ))));
//        m_button_perfedit->signal_clicked().connect(
//                mem_fun( *this, &mainwnd::open_performance_edit ));
//        add_tooltip( m_button_perfedit, "Show or hide song editor window" );
//        bottomhbox->pack_end(*m_button_perfedit, Gtk::PACK_SHRINK);

        /* vertical layout container for live tab*/
        vbox_live_tab = new VBox();
        vbox_live_tab->set_spacing(10);
        vbox_live_tab->set_border_width(10);
        vbox_live_tab->pack_start(*m_main_wid, Gtk::PACK_SHRINK);

        /* vertical layout container for song tab*/
        vbox_song_tab = new VBox();
        vbox_song_tab->set_spacing(10);
        vbox_song_tab->set_border_width(10);
        vbox_song_tab->pack_start(*m_mainperf->perform, Gtk::PACK_SHRINK);

        /* vertical layout container for sequence editor tab*/
        vbox_edit_tab = new VBox();
        vbox_edit_tab->set_spacing(10);
        vbox_edit_tab->set_border_width(10);

        /* notebook for tabbed functionality */
        notebook = manage (new Notebook ());
        notebook->set_tab_pos(Gtk::POS_BOTTOM);
        notebook->append_page(*vbox_live_tab,"_Live",true);
        notebook->append_page(*vbox_song_tab,"_Song",true);
        notebook->append_page(*vbox_edit_tab,"_Edit",true);

        /*main container for menu and window content */
        vbox_main = new VBox();
        vbox_main->set_border_width(6);
        vbox_main->pack_start(*hbox_top, Gtk::PACK_SHRINK);
        vbox_main->pack_start(*m_menubar, false, false );
        vbox_main->pack_start( *notebook );
        vbox_main->pack_start(*hbox_bottom, Gtk::PACK_SHRINK);

        /* add main layout box */
        this->add (*vbox_main);

        /* show everything */
        show_all();
    }
    add_events( Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK );

    m_timeout_connect = Glib::signal_timeout().connect(
            mem_fun(*this, &mainwnd::timer_callback), 25);

    m_perf_edit = new perfedit( m_mainperf );

    m_sigpipe[0] = -1;
    m_sigpipe[1] = -1;
    install_signal_handlers();
}


mainwnd::~mainwnd()
{
    delete m_perf_edit;
    delete m_options;

    if (m_sigpipe[0] != -1)
        close(m_sigpipe[0]);

    if (m_sigpipe[1] != -1)
        close(m_sigpipe[1]);
}


// This is the GTK timer callback, used to draw our current time and bpm
// ondd_events( the main window
bool
mainwnd::timer_callback(  )
{
    long ticks = m_mainperf->get_tick();

    m_main_time->idle_progress( ticks );
    m_main_wid->update_markers( ticks );

    if ( m_adjust_bpm->get_value() != m_mainperf->get_bpm()){
        m_adjust_bpm->set_value( m_mainperf->get_bpm());
    }

    if ( m_adjust_ss->get_value() !=  m_mainperf->get_screenset() )
    {
        m_main_wid->set_screenset(m_mainperf->get_screenset());
        m_adjust_ss->set_value( m_mainperf->get_screenset());
        m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                    m_mainperf->get_screenset()));
    }

    return true;
}


void
mainwnd::open_performance_edit()
{
    if (m_perf_edit->is_visible())
        m_perf_edit->hide();
    else {
        m_perf_edit->init_before_show();
        m_perf_edit->show_all();
        m_modified = true;
    }
}


void
mainwnd::prefs_dialog()
{
    delete m_options;
    m_options = new options( *this,  m_mainperf );
    m_options->show_all();
}


void
mainwnd::start_playing()
{
    /* clicking play in the main window causes
     * the song data to be ignored (Live mode) */
    m_mainperf->set_playback_mode( false );
    m_mainperf->position_jack( false );
    m_mainperf->start( false );
    m_mainperf->start_jack( );
    is_pattern_playing = true;
}


void
mainwnd::stop_playing()
{
    m_mainperf->stop_jack();
    m_mainperf->stop();
    m_main_wid->update_sequences_on_window();
    is_pattern_playing = false;
}

void
mainwnd::on_grouplearnchange(bool state)
{
    /* respond to learn mode change from m_mainperf */
    m_button_learn->set_image(*manage(new Image(
        Gdk::Pixbuf::create_from_xpm_data( state ? learn2_xpm : learn_xpm))));
}

void
mainwnd::learn_toggle()
{
    if (m_mainperf->is_group_learning())
    {
        m_mainperf->unset_mode_group_learn();
    }
    else
    {
        m_mainperf->set_mode_group_learn();
    }
}

/* callback function */
void mainwnd::file_new()
{
    if (is_save())
        new_file();
}

void mainwnd::new_file()
{
    m_mainperf->clear_all();

    m_main_wid->reset();
    m_entry_notes->set_text( * m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset() ));

    global_filename = "";
    update_window_title();
    m_modified = false;
}


/* callback function */
void mainwnd::file_save()
{
    save_file();
}


/* callback function */
void mainwnd::file_save_as()
{
    Gtk::FileChooserDialog dialog("Save file as",
                      Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);
    int result = dialog.run();

    switch (result) {
        case Gtk::RESPONSE_OK:
        {
            std::string fname = dialog.get_filename();
            Gtk::FileFilter* current_filter = dialog.get_filter();

            if ((current_filter != NULL) &&
                    (current_filter->get_name() == "MIDI files")) {

                // check for MIDI file extension; if missing, add .midi
                std::string suffix = fname.substr(
                        fname.find_last_of(".") + 1, std::string::npos);
                toLower(suffix);
                if ((suffix != "midi") && (suffix != "mid"))
                    fname = fname + ".midi";
            }

            if (Glib::file_test(fname, Glib::FILE_TEST_EXISTS)) {
                Gtk::MessageDialog warning(*this,
                        "File already exists!\n"
                        "Do you want to overwrite it?",
                        false,
                        Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO, true);
                auto result = warning.run();

                if (result == Gtk::RESPONSE_NO)
                    return;
            }
            global_filename = fname;
            update_window_title();
            save_file();
            break;
        }

        default:
            break;
    }
}


void mainwnd::open_file(const Glib::ustring& fn)
{
    bool result;

    m_mainperf->clear_all();

    midifile f(fn);
    result = f.parse(m_mainperf, 0);
    m_modified = !result;

    if (!result) {
        Gtk::MessageDialog errdialog(*this,
                "Error reading file: " + fn, false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        errdialog.run();
        return;
    }

    last_used_dir = fn.substr(0, fn.rfind("/") + 1);
    global_filename = fn;
    update_window_title();

    /* add to recent files list */
    m_options->add_recent_file(fn);
    /* update recent menu */
    redraw_menu();

    m_main_wid->reset();
    m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset()));
    m_adjust_bpm->set_value( m_mainperf->get_bpm());
}


/*callback function*/
void mainwnd::file_open()
{
    if (is_save())
        choose_file();
}


void mainwnd::choose_file()
{
    Gtk::FileChooserDialog dialog("Open MIDI file",
                      Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);

    int result = dialog.run();

    switch(result) {
        case(Gtk::RESPONSE_OK):
            open_file(dialog.get_filename());

        default:
            break;
    }
}


bool mainwnd::save_file()
{
    bool result = false;

    if (global_filename == "") {
        file_save_as();
        return true;
    }

    midifile f(global_filename);
    result = f.write(m_mainperf);

    if (!result) {
        Gtk::MessageDialog errdialog(*this,
                "Error writing file.", false,
                Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
        errdialog.run();
    } else {
        /* add to recent files list */
        m_options->add_recent_file(global_filename);
        /* update recent menu */
        redraw_menu();
    }
    m_modified = !result;
    return result;
}


int mainwnd::query_save_changes()
{
    Glib::ustring query_str;

    if (global_filename == "")
        query_str = "Unnamed file was changed.\nSave changes?";
    else
        query_str = "File '" + global_filename + "' was changed.\n"
                "Save changes?";

    Gtk::MessageDialog dialog(*this, query_str, false,
            Gtk::MESSAGE_QUESTION,
            Gtk::BUTTONS_NONE, true);

    dialog.add_button(Gtk::Stock::YES, Gtk::RESPONSE_YES);
    dialog.add_button(Gtk::Stock::NO, Gtk::RESPONSE_NO);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

    return dialog.run();
}


bool mainwnd::is_save()
{
    bool result = false;

    if (is_modified()) {
        int choice = query_save_changes();
        switch (choice) {
            case Gtk::RESPONSE_YES:
                if (save_file())
                    result = true;
                break;
            case Gtk::RESPONSE_NO:
                result = true;
                break;
            case Gtk::RESPONSE_CANCEL:
            default:
                break;
        }
    }
    else
        result = true;

    return result;
}


/* convert string to lower case letters */
void
mainwnd::toLower(basic_string<char>& s)
{
    for (basic_string<char>::iterator p = s.begin();
            p != s.end(); p++) {
        *p = tolower(*p);
    }
}


void
mainwnd::file_import_dialog()
{
    Gtk::FileChooserDialog dialog("Import MIDI file",
            Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);

    Gtk::FileFilter filter_midi;
    filter_midi.set_name("MIDI files");
    filter_midi.add_pattern("*.midi");
    filter_midi.add_pattern("*.mid");
    dialog.add_filter(filter_midi);

    Gtk::FileFilter filter_any;
    filter_any.set_name("Any files");
    filter_any.add_pattern("*");
    dialog.add_filter(filter_any);

    dialog.set_current_folder(last_used_dir);

    ButtonBox *btnbox = dialog.get_action_area();
    HBox hbox( false, 2 );

    m_adjust_load_offset = manage( new Adjustment( 0, -(c_max_sets - 1),
                c_max_sets - 1, 1 ));
    m_spinbutton_load_offset = manage( new SpinButton( *m_adjust_load_offset ));
    m_spinbutton_load_offset->set_editable( false );
    m_spinbutton_load_offset->set_wrap( true );
    hbox.pack_end(*m_spinbutton_load_offset, false, false );
    hbox.pack_end(*(manage( new Label("Screen Set Offset"))), false, false, 4);

    btnbox->pack_start(hbox, false, false );

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    dialog.show_all_children();

    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
       case(Gtk::RESPONSE_OK):
       {
           try{
               midifile f( dialog.get_filename() );
               f.parse( m_mainperf, (int) m_adjust_load_offset->get_value() );
           }
           catch(...){
               Gtk::MessageDialog errdialog(*this,
                       "Error reading file.", false,
                       Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
                errdialog.run();
           }

           global_filename = std::string(dialog.get_filename());
           update_window_title();
           m_modified = true;

           m_main_wid->reset();
           m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                       m_mainperf->get_screenset() ));
           m_adjust_bpm->set_value( m_mainperf->get_bpm() );

           break;
       }

       case(Gtk::RESPONSE_CANCEL):
           break;

       default:
           break;

   }
}

/*callback function*/
void mainwnd::file_exit()
{
    if (is_save()) {
        if (is_pattern_playing)
            stop_playing();
        hide();
    }
}


bool
mainwnd::on_delete_event(GdkEventAny *a_e)
{
    bool result = is_save();
    if (result && is_pattern_playing)
            stop_playing();

    return !result;
}


void
mainwnd::about_dialog()
{
    Gtk::AboutDialog dialog;
    dialog.set_transient_for(*this);
    dialog.set_name(PACKAGE_NAME);
    dialog.set_version(VERSION);
    dialog.set_comments("Interactive MIDI Sequencer\n");

    dialog.set_copyright(
            "(C) 2002 - 2006 Rob C. Buse\n"
            "(C) 2008 - 2010 Seq24team");
    dialog.set_website(
            "http://www.filter24.org/seq24\n"
            "http://edge.launchpad.net/seq24");

    std::list<Glib::ustring> list_authors;
    list_authors.push_back("Rob C. Buse <rcb@filter24.org>");
    list_authors.push_back("Ivan Hernandez <ihernandez@kiusys.com>");
    list_authors.push_back("Guido Scholz <guido.scholz@bayernline.de>");
    list_authors.push_back("Jaakko Sipari <jaakko.sipari@gmail.com>");
    list_authors.push_back("Peter Leigh <pete.leigh@gmail.com>");
    list_authors.push_back("Anthony Green <green@redhat.com>");
    list_authors.push_back("Daniel Ellis <mail@danellis.co.uk>");
    list_authors.push_back("Sebastien Alaiwan <sebastien.alaiwan@gmail.com>");
    list_authors.push_back("Kevin Meinert <kevin@subatomicglue.com>");
    list_authors.push_back("Andrea delle Canne <andreadellecanne@gmail.com>");
    dialog.set_authors(list_authors);

    std::list<Glib::ustring> list_documenters;
    list_documenters.push_back("Dana Olson <seq24@ubuntustudio.com>");
    dialog.set_documenters(list_documenters);

    dialog.show_all_children();
    dialog.run();
}


void
mainwnd::adj_callback_ss( )
{
    m_mainperf->set_screenset( (int) m_adjust_ss->get_value());
    m_main_wid->set_screenset( m_mainperf->get_screenset());
    m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                m_mainperf->get_screenset()));
    m_modified = true;
}


void
mainwnd::adj_callback_bpm( )
{
    m_mainperf->set_bpm( (int) m_adjust_bpm->get_value());
    m_modified = true;
}


bool
mainwnd::on_key_release_event(GdkEventKey* a_ev)
{
    if ( a_ev->keyval == m_mainperf->m_key_replace )
        m_mainperf->unset_sequence_control_status( c_status_replace );

    if (a_ev->keyval == m_mainperf->m_key_queue )
        m_mainperf->unset_sequence_control_status( c_status_queue );

    if ( a_ev->keyval == m_mainperf->m_key_snapshot_1 ||
            a_ev->keyval == m_mainperf->m_key_snapshot_2 )
        m_mainperf->unset_sequence_control_status( c_status_snapshot );

    if ( a_ev->keyval == m_mainperf->m_key_group_learn ){
        m_mainperf->unset_mode_group_learn();
    }

    return false;
}


void
mainwnd::edit_callback_notepad( )
{
    string text = m_entry_notes->get_text();
    m_mainperf->set_screen_set_notepad( m_mainperf->get_screenset(),
				        &text );
    m_modified = true;
}


bool
mainwnd::on_key_press_event(GdkEventKey* a_ev)
{
    Gtk::Window::on_key_press_event(a_ev);

    /* Ignore key presses if we're renaming the scene */
    if ( m_entry_notes->has_focus()) {
        return false;
    }

    // control and modifier key combinations matching
    else if ( a_ev->type == GDK_KEY_PRESS )
    {
        if ( global_print_keys ){
            printf( "key_press[%d]\n", a_ev->keyval );
            fflush( stdout );
        }

        /* check for bpm down key */
        if ( a_ev->keyval == m_mainperf->m_key_bpm_dn ){
            m_mainperf->set_bpm( m_mainperf->get_bpm() - 1 );
            m_adjust_bpm->set_value(  m_mainperf->get_bpm() );
        }

        /* check for bpm up key */
        if ( a_ev->keyval ==  m_mainperf->m_key_bpm_up ){
            m_mainperf->set_bpm( m_mainperf->get_bpm() + 1 );
            m_adjust_bpm->set_value(  m_mainperf->get_bpm() );
        }

        /* check for sequence replace key */
        if ( a_ev->keyval == m_mainperf->m_key_replace )
        {
            m_mainperf->set_sequence_control_status( c_status_replace );
        }

        /* check for sequence queue key */
        if ((a_ev->keyval ==  m_mainperf->m_key_queue )
                || (a_ev->keyval == m_mainperf->m_key_keep_queue ))
        {
            m_mainperf->set_sequence_control_status( c_status_queue );
        }

        /* check for sequence snapshot key */
        if ( a_ev->keyval == m_mainperf->m_key_snapshot_1 ||
                a_ev->keyval == m_mainperf->m_key_snapshot_2 )
        {
            m_mainperf->set_sequence_control_status( c_status_snapshot );
        }

        /* check for screenset down key */
        if ( a_ev->keyval == m_mainperf->m_key_screenset_dn ){

            m_mainperf->set_screenset(  m_mainperf->get_screenset() - 1 );
            m_main_wid->set_screenset(  m_mainperf->get_screenset() );
            m_adjust_ss->set_value( m_mainperf->get_screenset()  );
            m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                        m_mainperf->get_screenset()));
        }

        /* check for screenset up key */
        if ( a_ev->keyval == m_mainperf->m_key_screenset_up ){

            m_mainperf->set_screenset(  m_mainperf->get_screenset() + 1 );
            m_main_wid->set_screenset(  m_mainperf->get_screenset() );
            m_adjust_ss->set_value( m_mainperf->get_screenset()  );
            m_entry_notes->set_text(*m_mainperf->get_screen_set_notepad(
                        m_mainperf->get_screenset()));
        }

        /* not sure what this does yet */
        if ( a_ev->keyval == m_mainperf->m_key_set_playing_screenset ){
            m_mainperf->set_playing_screenset();
        }

        /* check for mute group on key */
        if ( a_ev->keyval == m_mainperf->m_key_group_on ){
            m_mainperf->set_mode_group_mute();
        }

        /* check for mute group off key */
        if ( a_ev->keyval == m_mainperf->m_key_group_off ){
            m_mainperf->unset_mode_group_mute();
        }

        /* check for mute group learn key */
        if ( a_ev->keyval == m_mainperf->m_key_group_learn ){
            m_mainperf->set_mode_group_learn();
        }

        /* activate mute group key */
        if (m_mainperf->get_key_groups()->count( a_ev->keyval ) != 0 )
        {
            m_mainperf->select_and_mute_group(
                    m_mainperf->lookup_keygroup_group(a_ev->keyval));
        }

        /* if mute group learn is on */
        if (m_mainperf->is_learn_mode() &&
                a_ev->keyval != m_mainperf->m_key_group_learn)
        {
            if( m_mainperf->get_key_groups()->count( a_ev->keyval ) != 0 )
            {
                std::ostringstream os;
                os << "Key \""
                    << gdk_keyval_name(a_ev->keyval)
                    << "\" (code = "
                    << a_ev->keyval
                    << ") successfully mapped.";

                Gtk::MessageDialog dialog(*this,
                        "MIDI mute group learn success", false,
                        Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
                dialog.set_secondary_text(os.str(), false);
                dialog.run();

                // we miss the keyup msg for learn, force set it off
                m_mainperf->unset_mode_group_learn();
            }
            else
            {
                std::ostringstream os;
                os << "Key \""
                    << gdk_keyval_name(a_ev->keyval)
                    << "\" (code = "
                    << a_ev->keyval
                    << ") is not one of the configured mute-group keys.\n"
                    << "To change this see File/Options menu or .seq24rc";

                Gtk::MessageDialog dialog(*this,
                        "MIDI mute group learn failed", false,
                        Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);

                dialog.set_secondary_text(os.str(), false);
                dialog.run();
                // we miss the keyup msg for learn, force set it
                m_mainperf->unset_mode_group_learn();
            }
        }

        // the start/end key may be the same key (i.e. SPACE)
        // allow toggling when the same key is mapped to both
        // triggers (i.e. SPACEBAR)
        bool dont_toggle = m_mainperf->m_key_start
            != m_mainperf->m_key_stop;

        if ( a_ev->keyval == m_mainperf->m_key_start
                && (dont_toggle || !is_pattern_playing))
        {
            start_playing();
        }
        else if ( a_ev->keyval == m_mainperf->m_key_stop
                && (dont_toggle || is_pattern_playing))
        {
            stop_playing();
        }

        /* toggle sequence mute/unmute using keyboard keys */
        if (m_mainperf->get_key_events()->count( a_ev->keyval) != 0)
        {
            sequence_key(m_mainperf->lookup_keyevent_seq( a_ev->keyval));
        }
    }
    return false;
}


void
mainwnd::sequence_key( int a_seq )
{
    /* add screen set offset */
    a_seq += m_mainperf->get_screenset() * c_mainwnd_rows * c_mainwnd_cols;

    if ( m_mainperf->is_active( a_seq ) ){

//        m_mainperf->sequence_playing_toggle( a_seq );

        /* if we're recording,
         * add seq playback changes to the song data */
        if ( m_mainperf->get_song_recording() ) {

            long tick = m_mainperf->get_tick();

            bool trigger_state = m_mainperf->get_sequence( a_seq )->get_trigger_state( tick );

            sequence* seq = m_mainperf->get_sequence( a_seq );

            /* if sequence already playing */
            if ( trigger_state )
            {
                /* if this play is us recording live, end the new trigger block here */
                if (seq->get_song_recording())
                    seq->song_recording_stop();

                /* ...else we need to trim the block already in place */
                else {
                    seq->exact_split_trigger( tick );
                    seq->del_trigger( tick );
                }

            }

            /* if not playing, start recording a new strip */
            else
            {
//                 snap to length of sequence
//                tick = tick - (tick % seq_length);

                m_mainperf->push_trigger_undo();
                seq->song_recording_start( tick );
            }
        }

        m_mainperf->sequence_playing_toggle( a_seq );
    }
}


void
mainwnd::update_window_title()
{
    std::string title;

    if (global_filename == "")
        title = ( PACKAGE ) + string( " - [unnamed]" );
    else
        title =
            ( PACKAGE )
            + string( " - [" )
            + Glib::filename_to_utf8(global_filename)
            + string( "]" );

    set_title ( title.c_str());
}


bool
mainwnd::is_modified()
{
    return m_modified;
}


int mainwnd::m_sigpipe[2];


/* Handler for system signals (SIGUSR1, SIGINT...)
 * Write a message to the pipe and leave as soon as possible
 */
void
mainwnd::handle_signal(int sig)
{
    if (write(m_sigpipe[1], &sig, sizeof(sig)) == -1)
    {
        printf("write() failed: %s\n", std::strerror(errno));
    }
}

bool
mainwnd::install_signal_handlers()
{
    /*install pipe to forward received system signals*/
    if (pipe(m_sigpipe) < 0)
    {
        printf("pipe() failed: %s\n", std::strerror(errno));
        return false;
    }

    /*install notifier to handle pipe messages*/
    Glib::signal_io().connect(sigc::mem_fun(*this, &mainwnd::signal_action),
            m_sigpipe[0], Glib::IO_IN);

    /*install signal handlers*/
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;

    if (sigaction(SIGUSR1, &action, NULL) == -1)
    {
        printf("sigaction() failed: %s\n", std::strerror(errno));
        return false;
    }

    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        printf("sigaction() failed: %s\n", std::strerror(errno));
        return false;
    }

    return true;
}


bool
mainwnd::signal_action(Glib::IOCondition condition)
{
    int message;

    if ((condition & Glib::IO_IN) == 0)
    {
        printf("Error: unexpected IO condition\n");
        return false;
    }


    if (read(m_sigpipe[0], &message, sizeof(message)) == -1)
    {
        printf("read() failed: %s\n", std::strerror(errno));
        return false;
    }

    switch (message)
    {
        case SIGUSR1:
            save_file();
            break;

        case SIGINT:
            file_exit();
            break;

        default:
            printf("Unexpected signal received: %d\n", message);
            break;
    }
    return true;
}

void mainwnd::redraw_menu(){

    /* get rid of old menubar and main content */
    vbox_main->remove(*m_menubar);
    vbox_main->remove(*vbox_live_tab);

    /* unpack mainvbox from window */
    this->remove();

    /* regenerate menu content */
    m_menubar = new MenuBar();

    m_menu_file = manage(new Menu());
    m_menubar->items().push_front(MenuElem("_File", *m_menu_file));

    m_menu_view = manage( new Menu());
    m_menubar->items().push_back(MenuElem("_View", *m_menu_view));

    m_menu_help = manage( new Menu());
    m_menubar->items().push_back(MenuElem("_Help", *m_menu_help));

    /* file menu items */
    m_menu_file->items().push_back(MenuElem("_New",
                Gtk::AccelKey("<control>N"),
                mem_fun(*this, &mainwnd::file_new)));

    m_menu_file->items().push_back(MenuElem("_Open...",
                Gtk::AccelKey("<control>O"),
                mem_fun(*this, &mainwnd::file_open)));

    /* recent files sub-menu */
    m_menu_file_recent = manage (new Menu());

    /* only add if a path is actually contained in each slot */
    if (recent_files[0]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[0],
                Gtk::AccelKey("<control>R"),
                mem_fun(*this, &mainwnd::load_recent_1)));
    else
    m_menu_file_recent->items().push_back(MenuElem("(no recent files)"));

    if (recent_files[1]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[1],
                mem_fun(*this, &mainwnd::load_recent_2)));

    if (recent_files[2]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[2],
                mem_fun(*this, &mainwnd::load_recent_3)));

    if (recent_files[3]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[3],
                mem_fun(*this, &mainwnd::load_recent_4)));

    if (recent_files[4]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[4],
                mem_fun(*this, &mainwnd::load_recent_5)));

    if (recent_files[5]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[5],
                mem_fun(*this, &mainwnd::load_recent_6)));

    if (recent_files[6]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[6],
                mem_fun(*this, &mainwnd::load_recent_7)));

    if (recent_files[7]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[7],
                mem_fun(*this, &mainwnd::load_recent_8)));

    if (recent_files[8]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[8],
                mem_fun(*this, &mainwnd::load_recent_9)));

    if (recent_files[9]!="")
    m_menu_file_recent->items().push_back(MenuElem(recent_files[9],
                mem_fun(*this, &mainwnd::load_recent_10)));

    m_menu_file->items().push_back(MenuElem("Open Recent...",
                *m_menu_file_recent));

    m_menu_file->items().push_back(MenuElem("_Save",
                Gtk::AccelKey("<control>S"),
                mem_fun(*this, &mainwnd::file_save)));

    m_menu_file->items().push_back(MenuElem("Save _As...",
                mem_fun(*this, &mainwnd::file_save_as)));

    m_menu_file->items().push_back(SeparatorElem());

    m_menu_file->items().push_back(MenuElem("_Import...",
                mem_fun(*this, &mainwnd::file_import_dialog)));

    m_menu_file->items().push_back(MenuElem("_Preferences...",
                Gtk::AccelKey("<control>P"),
                mem_fun(*this,&mainwnd::prefs_dialog)));

    m_menu_file->items().push_back(SeparatorElem());

    m_menu_file->items().push_back(MenuElem("E_xit",
                Gtk::AccelKey("<control>Q"),
                mem_fun(*this, &mainwnd::file_exit)));

    /* view menu items */
    m_menu_view->items().push_back(MenuElem("_Song Editor...",
                Gtk::AccelKey("<control>E"),
                mem_fun(*this, &mainwnd::open_performance_edit)));

    /* help menu items */
    m_menu_help->items().push_back(MenuElem("_About...",
                mem_fun(*this, &mainwnd::about_dialog)));

    /* pack content. main content widgets are the
     * same, so just add again */
    vbox_main->pack_start(*m_menubar, false, false);
    vbox_main->pack_start(*vbox_live_tab);

    /* add main layout box */
    this->add (*vbox_main);

    /* show everything */
    show_all();
}

/* recent file loading methods for recent menu signals */
void mainwnd::load_recent_1(){
    open_file(recent_files[0]);
}
void mainwnd::load_recent_2(){
    open_file(recent_files[1]);
}

void mainwnd::load_recent_3(){
    open_file(recent_files[2]);
}

void mainwnd::load_recent_4(){
    open_file(recent_files[3]);
}

void mainwnd::load_recent_5(){
    open_file(recent_files[4]);
}

void mainwnd::load_recent_6(){
    open_file(recent_files[5]);
}

void mainwnd::load_recent_7(){
    open_file(recent_files[6]);
}

void mainwnd::load_recent_8(){
    open_file(recent_files[7]);
}

void mainwnd::load_recent_9(){
    open_file(recent_files[8]);
}

void mainwnd::load_recent_10(){
    open_file(recent_files[9]);
}

