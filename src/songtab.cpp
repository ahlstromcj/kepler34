#include "songtab.h"
#include "sequence.h"

#include "pixmaps/snap.xpm"
#include "pixmaps/play3.xpm"
#include "pixmaps/rec2.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/expand.xpm"
#include "pixmaps/collapse.xpm"
#include "pixmaps/loop.xpm"
#include "pixmaps/copy.xpm"
#include "pixmaps/undo.xpm"
#include "pixmaps/down.xpm"
#include "pixmaps/perfedit.xpm"

using namespace sigc;

// tooltip helper, for old vs new gtk...
#if GTK_MINOR_VERSION >= 12
#   define add_tooltip( obj, text ) obj->set_tooltip_text( text);
#else
#   define add_tooltip( obj, text ) m_tooltips->set_tip( *obj, text );
#endif

songtab::songtab( perform *a_perf  )
{
    using namespace Menu_Helpers;

    /* set the performance */
    m_snap = c_ppqn / 4;

    m_mainperf = a_perf;

    /* window title & size */
    set_size_request(700, 400);

    /* tooltips */
    m_tooltips = manage( new Tooltips( ) );

    m_vadjust = manage( new Adjustment(0,0,1,1,1,1 ));
    m_hadjust = manage( new Adjustment(0,0,1,1,1,1 ));

    m_vscroll   =  manage(new VScrollbar( *m_vadjust ));
    m_hscroll   =  manage(new HScrollbar( *m_hadjust ));

    m_perfnames = manage( new perfnames( m_mainperf, m_vadjust ));

    m_perfroll = manage( new perfroll( m_mainperf,
                       m_hadjust,
                       m_vadjust ));

    m_perftime = manage( new perftime( m_mainperf, m_hadjust ));

    /* init table, viewports and scroll bars */
    m_table     = manage( new Table( 6, 3, false));
    m_table->set_border_width( 2 );

    m_hbox      = manage( new HBox( false, 2 ));
    m_hlbox     = manage( new HBox( false, 2 ));

    m_hlbox->set_border_width( 2 );

    m_button_grow = manage( new Button());
    m_button_grow->add( *manage( new Arrow( Gtk::ARROW_RIGHT, Gtk::SHADOW_OUT )));
    m_button_grow->signal_clicked().connect( mem_fun( *this, &songtab::grow));
    add_tooltip( m_button_grow, "Increase size of Grid." );

    /* fill table */
    m_table->attach( *m_hlbox,  0, 3, 0, 1,  Gtk::FILL, Gtk::SHRINK, 2, 0 ); // shrink was 0

    m_table->attach( *m_perfnames,    0, 1, 2, 3, Gtk::SHRINK, Gtk::FILL );

    m_table->attach( *m_perftime, 1, 2, 1, 2, Gtk::FILL, Gtk::SHRINK );
    m_table->attach( *m_perfroll, 1, 2, 2, 3,
             Gtk::FILL | Gtk::SHRINK,
             Gtk::FILL | Gtk::SHRINK );

    m_table->attach( *m_vscroll, 2, 3, 2, 3, Gtk::SHRINK, Gtk::FILL | Gtk::EXPAND  );

    m_table->attach( *m_hbox,  0, 1, 3, 4,  Gtk::FILL, Gtk::SHRINK, 0, 2 );
    m_table->attach( *m_hscroll, 1, 2, 3, 4, Gtk::FILL | Gtk::EXPAND, Gtk::SHRINK  );
    m_table->attach( *m_button_grow, 2, 3, 3, 4, Gtk::SHRINK, Gtk::SHRINK  );

    m_menu_snap =   manage( new Menu());
    m_menu_snap->items().push_back(MenuElem("1/1",     sigc::bind(mem_fun(*this,&songtab::set_snap), 1  )));
    m_menu_snap->items().push_back(MenuElem("1/2",   sigc::bind(mem_fun(*this,&songtab::set_snap), 2  )));
    m_menu_snap->items().push_back(MenuElem("1/4",   sigc::bind(mem_fun(*this,&songtab::set_snap), 4  )));
    m_menu_snap->items().push_back(MenuElem("1/8",   sigc::bind(mem_fun(*this,&songtab::set_snap), 8  )));
    m_menu_snap->items().push_back(MenuElem("1/16",   sigc::bind(mem_fun(*this,&songtab::set_snap), 16  )));
    m_menu_snap->items().push_back(MenuElem("1/32",   sigc::bind(mem_fun(*this,&songtab::set_snap), 32  )));

    /* snap */
    m_button_snap = manage( new Button());
    m_button_snap->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( snap_xpm ))));
    m_button_snap->signal_clicked().connect(  sigc::bind<Menu *>( mem_fun( *this, &songtab::popup_menu), m_menu_snap  ));
    add_tooltip( m_button_snap, "Grid snap. (Fraction of Measure Length)" );
    m_entry_snap = manage( new Entry());
    m_entry_snap->set_size_request( 40, -1 );
    m_entry_snap->set_editable( false );

    m_menu_bpm = manage( new Menu() );
    m_menu_bw = manage( new Menu() );

    /* bw */
    m_menu_bw->items().push_back(MenuElem("1", sigc::bind(mem_fun(*this,&songtab::set_bw), 1  )));
    m_menu_bw->items().push_back(MenuElem("2", sigc::bind(mem_fun(*this,&songtab::set_bw), 2  )));
    m_menu_bw->items().push_back(MenuElem("4", sigc::bind(mem_fun(*this,&songtab::set_bw), 4  )));
    m_menu_bw->items().push_back(MenuElem("8", sigc::bind(mem_fun(*this,&songtab::set_bw), 8  )));
    m_menu_bw->items().push_back(MenuElem("16", sigc::bind(mem_fun(*this,&songtab::set_bw), 16 )));

    char b[20];

    for( int i=0; i<16; i++ ){

        snprintf( b, sizeof(b), "%d", i+1 );

        /* length */
        m_menu_bpm->items().push_back(MenuElem(b,
                                               sigc::bind(mem_fun(*this,&songtab::set_bpm),
                                                    i+1 )));
    }

    /* beats per measure */
    m_button_bpm = manage( new Button());
    m_button_bpm->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( down_xpm  ))));
    m_button_bpm->signal_clicked().connect(  sigc::bind<Menu *>( mem_fun( *this, &songtab::popup_menu), m_menu_bpm  ));
    add_tooltip( m_button_bpm, "Time Signature. Beats per Measure" );
    m_entry_bpm = manage( new Entry());
    m_entry_bpm->set_width_chars(2);
    m_entry_bpm->set_editable( false );

    /* beat width */
    m_button_bw = manage( new Button());
    m_button_bw->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( down_xpm  ))));
    m_button_bw->signal_clicked().connect(  sigc::bind<Menu *>( mem_fun( *this, &songtab::popup_menu), m_menu_bw  ));
    add_tooltip( m_button_bw, "Time Signature.  Length of Beat" );
    m_entry_bw = manage( new Entry());
    m_entry_bw->set_width_chars(2);
    m_entry_bw->set_editable( false );

    /* undo */
    m_button_undo = manage( new Button());
    m_button_undo->add( *manage( new Image(Gdk::Pixbuf::create_from_xpm_data( undo_xpm  ))));
    m_button_undo->signal_clicked().connect(  mem_fun( *this, &songtab::undo));
    add_tooltip( m_button_undo, "Undo." );

    /* expand */
    m_button_expand = manage( new Button());
    m_button_expand->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( expand_xpm ))));
    m_button_expand->signal_clicked().connect(  mem_fun( *this, &songtab::expand));
    add_tooltip( m_button_expand, "Expand between L and R markers." );

    /* collapse */
    m_button_collapse = manage( new Button());
    m_button_collapse->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( collapse_xpm ))));
    m_button_collapse->signal_clicked().connect(  mem_fun( *this, &songtab::collapse));
    add_tooltip( m_button_collapse, "Collapse between L and R markers." );

    /* copy */
    m_button_copy = manage( new Button());
    m_button_copy->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( copy_xpm ))));
    m_button_copy->signal_clicked().connect(  mem_fun( *this, &songtab::copy ));
    add_tooltip( m_button_copy, "Expand and copy between L and R markers." );

    /* loop */
    m_button_loop = manage( new ToggleButton() );
    m_button_loop->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( loop_xpm ))));
    m_button_loop->signal_toggled().connect(  mem_fun( *this, &songtab::set_looped ));
    add_tooltip( m_button_loop, "Play looped between L and R." );

    /* stop */
    m_button_stop = manage( new Button() );
    m_button_stop->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( stop_xpm ))));
    m_button_stop->signal_clicked().connect( mem_fun( *this, &songtab::stop_playing));
    add_tooltip( m_button_stop, "Stop playing." );

    /* record */
    m_button_record = manage( new ToggleButton() );
    m_button_record->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( rec2_xpm ))));
    m_button_record->signal_toggled().connect( mem_fun( *this, &songtab::record_toggle));
    add_tooltip( m_button_stop, "Record sequence changes to the song editor." );

    /* play */
    m_button_play = manage( new Button() );
    m_button_play->add(*manage( new Image(Gdk::Pixbuf::create_from_xpm_data( play3_xpm ))));
    m_button_play->signal_clicked().connect(  mem_fun( *this, &songtab::start_playing));
    add_tooltip( m_button_play, "Begin playing the song at L marker." );

    m_hlbox->pack_end( *m_button_copy , false, false );
    m_hlbox->pack_end( *m_button_expand , false, false );
    m_hlbox->pack_end( *m_button_collapse , false, false );
    m_hlbox->pack_end( *m_button_undo , false, false );

    m_hlbox->pack_start( *m_button_stop , false, false );
    m_hlbox->pack_start( *m_button_play , false, false );
    m_hlbox->pack_start( *m_button_record , false, false );
    m_hlbox->pack_start( *m_button_loop , false, false );

    m_hlbox->pack_start( *(manage(new VSeparator( ))), false, false, 4);

    m_hlbox->pack_start( *m_button_bpm , false, false );
    m_hlbox->pack_start( *m_entry_bpm , false, false );

    m_hlbox->pack_start( *(manage(new Label( "/" ))), false, false, 4);

    m_hlbox->pack_start( *m_button_bw , false, false );
    m_hlbox->pack_start( *m_entry_bw , false, false );

    m_hlbox->pack_start( *(manage(new Label( "x" ))), false, false, 4);

    m_hlbox->pack_start( *m_button_snap , false, false );
    m_hlbox->pack_start( *m_entry_snap , false, false );

    /* add table */
    add( *m_table );

    m_snap = 8;
    m_bpm = 4;
    m_bw = 4;

    set_snap( 8 );
    set_bpm( 4 );
    set_bw( 4 );

    add_events( Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK );
}

bool
songtab::on_key_press_event(GdkEventKey* a_ev)
{
    bool event_was_handled = false;
    if ( a_ev->type == GDK_KEY_PRESS ){

        if ( global_print_keys ){
            printf( "key_press[%d] == %s\n", a_ev->keyval, gdk_keyval_name( a_ev->keyval ) );
        }
        // the start/end key may be the same key (i.e. SPACE)
        // allow toggling when the same key is mapped to both triggers (i.e. SPACEBAR)
        bool dont_toggle = m_mainperf->m_key_start != m_mainperf->m_key_stop;
        if ( a_ev->keyval == m_mainperf->m_key_start && (dont_toggle || !m_mainperf->is_running()) )
        {
            start_playing();
            return true;
        }
        else if ( a_ev->keyval == m_mainperf->m_key_stop && (dont_toggle || m_mainperf->is_running()) )
        {
            stop_playing();
            return true;
        }

        //TODO add "R" record hotkey to documentation
        if (a_ev->keyval == m_mainperf->m_key_record_upper
                || a_ev->keyval == m_mainperf->m_key_record_lower){
            m_button_record->set_active(!m_button_record->get_active());
            return true;
        }

        if (a_ev->keyval == m_mainperf->m_key_start || a_ev->keyval == m_mainperf->m_key_stop)
          event_was_handled = true;
    }

    if(!event_was_handled)
    {
      return on_key_press_event(a_ev);
    }

    return false;
}

void
songtab::undo()
{
    m_mainperf->pop_trigger_undo();
    m_perfroll->queue_draw();
}

void
songtab::start_playing()
{
    /* clicking play in the song editor causes
     * the song data to be played */
    m_mainperf->set_playback_mode( true );
    m_mainperf->position_jack( true );
    m_mainperf->start_jack( );
    m_mainperf->start( true );
}

void
songtab::stop_playing()
{

    m_mainperf->stop_jack();
    m_mainperf->stop();
}

void
songtab::collapse()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->move_triggers( false );
    m_perfroll->queue_draw();
}

void
songtab::copy()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->copy_triggers(  );
    m_perfroll->queue_draw();
}

void
songtab::expand()
{
    m_mainperf->push_trigger_undo();
    m_mainperf->move_triggers( true );
    m_perfroll->queue_draw();
}

void
songtab::set_looped()
{
    m_mainperf->set_looping( m_button_loop->get_active());
}

/* toggle recording */
void
songtab::record_toggle()
{
    m_mainperf->set_song_recording(m_button_record->get_active());
}

void
songtab::popup_menu(Menu *a_menu)
{
    a_menu->popup(0,0);
}

void
songtab::set_guides()
{
    long measure_ticks = (c_ppqn * 4) * m_bpm / m_bw;
    long snap_ticks =  measure_ticks / m_snap;
    long beat_ticks = (c_ppqn * 4) / m_bw;
    m_perfroll->set_guides( snap_ticks, measure_ticks, beat_ticks );
    m_perftime->set_guides( snap_ticks, measure_ticks );
}


void
songtab::set_snap( int a_snap  )
{
    char b[10];
    snprintf( b, sizeof(b), "1/%d", a_snap );
    m_entry_snap->set_text(b);

    m_snap = a_snap;
    set_guides();
}

void songtab::set_bpm( int a_beats_per_measure )
{
    char b[10];
    snprintf(b, sizeof(b), "%d", a_beats_per_measure );
    m_entry_bpm->set_text(b);

    m_bpm = a_beats_per_measure;
    set_guides();
}


void songtab::set_bw( int a_beat_width )
{
    char b[10];
    snprintf(b, sizeof(b), "%d", a_beat_width );
    m_entry_bw->set_text(b);

    m_bw = a_beat_width;
    set_guides();
}


void
songtab::on_realize()
{
    // we need to do the default realize
    Gtk::Widget::on_realize();

    Glib::signal_timeout().connect(mem_fun(*this,&songtab::timeout ), c_redraw_ms);
}


void
songtab::grow()
{
    m_perfroll->increment_size();
    m_perftime->increment_size();
}

void
songtab::init_before_show()
{
    m_perfroll->init_before_show();
    //m_perftime->init_before_show();
}

bool
songtab::timeout()
{

    m_perfroll->redraw_dirty_sequences();
    m_perfroll->draw_progress();
    m_perfnames->redraw_dirty_sequences();

    return true;
}

songtab::~songtab()
{

}


bool
songtab::on_delete_event(GdkEventAny *a_event)
{
    return false;
}
