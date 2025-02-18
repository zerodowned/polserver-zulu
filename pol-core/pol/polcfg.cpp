/*
History
=======
2007/06/17 Shinigami: added config.world_data_path
2009/08/06 MuadDib:   Removed PasswordOnlyHash support
                      ClearTextPasswords now default is False.
2009/10/14 Turley:    added bool LogfileTimestampEveryLine
2009/12/02 Turley:    added MaxTileID -Tomi
2009/12/04 Turley:    cleanup "MasterKey1","MasterKey2","ClientVersion","KeyFile" - Tomi
2010/02/04 Turley:    polcfg.discard_old_events discards oldest event if queue is full

Notes
=======

*/


#include "../clib/stl_inc.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../bscript/config.h"

#include "../clib/cfgelem.h"
#include "../clib/cfgfile.h"
#include "../clib/dirlist.h"
#include "../clib/logfile.h"
#include "../clib/mdump.h"
#include "../clib/mlog.h"
#include "../clib/passert.h"
#include "../clib/strutil.h"

#include "core.h"
#include "polcfg.h"
#include "polsig.h"
#include "schedule.h"

#include "crypt/cryptkey.h"

PolConfig config;
extern unsigned long cfg_masterkey1;
extern unsigned long cfg_masterkey2;

static struct stat pol_cfg_stat;

void read_pol_config( bool initial_load )
{
    ConfigFile cf( "pol.cfg" );
    ConfigElem elem;

    cf.readraw( elem );

    if (initial_load)
    {
        stat( "pol.cfg", &pol_cfg_stat );

        // these config options can't change after startup.
        config.uo_datafile_root = elem.remove_string( "UoDataFileRoot" );
        config.uo_datafile_root = normalized_dir_form( config.uo_datafile_root );

        config.world_data_path = elem.remove_string( "WorldDataPath", "data/" );
        config.world_data_path = normalized_dir_form( config.world_data_path );

		config.realm_data_path = elem.remove_string( "RealmDataPath", "realm/" );
		config.realm_data_path = normalized_dir_form( config.realm_data_path );

		config.pidfile_path = elem.remove_string( "PidFilePath", "./" );
		config.pidfile_path = normalized_dir_form( config.pidfile_path );

        config.listen_port = elem.remove_ushort( "ListenPort", 0 );
        config.check_integrity = true; // elem.remove_bool( "CheckIntegrity", true );
        config.count_resource_tiles = elem.remove_bool( "CountResourceTiles", false );
        config.multithread = elem.remove_ushort( "Multithread", 0 );
        config.web_server = elem.remove_bool( "WebServer", false );
        config.web_server_port = elem.remove_ushort( "WebServerPort", 8080 );

		unsigned short max_tile = elem.remove_ushort( "MaxTileID", 0x3FFF );

		if (max_tile != 0x3FFF && max_tile != 0x7FFF)
			config.max_tile_id = 0x3FFF;
		else
			config.max_tile_id = max_tile;

        config.ignore_load_errors = elem.remove_bool( "IgnoreLoadErrors", false );

        config.debug_port = elem.remove_ushort( "DebugPort", 0 );
    }
    config.verbose = elem.remove_bool( "Verbose", false );
    config.watch_mapcache = elem.remove_bool( "WatchMapCache", false );
    config.loglevel = elem.remove_ushort( "LogLevel", 0 );
    config.select_timeout_usecs = elem.remove_ushort( "SelectTimeout", 10 );
    config.watch_rpm = elem.remove_bool( "WatchRpm", false );
    config.watch_sysload = elem.remove_bool( "WatchSysLoad", false );
    config.log_sysload = elem.remove_bool( "LogSysLoad", false );
    config.inhibit_saves = elem.remove_bool( "InhibitSaves", false );
    config.log_script_cycles = elem.remove_bool( "LogScriptCycles", false );
    config.web_server_local_only = elem.remove_bool( "WebServerLocalOnly", true );
    config.web_server_debug = elem.remove_ushort( "WebServerDebug", 0 );
    config.web_server_password = elem.remove_string( "WebServerPassword", "" );

    config.cache_interactive_scripts = elem.remove_bool( "CacheInteractiveScripts", true );
    config.show_speech_colors = elem.remove_bool( "ShowSpeechColors", false );
    config.require_spellbooks = elem.remove_bool( "RequireSpellbooks", true );

    config.enable_secure_trading = elem.remove_bool( "EnableSecureTrading", false );
    config.runaway_script_threshold = elem.remove_ulong( "RunawayScriptThreshold", 5000 );

	config.min_cmdlvl_ignore_inactivity = elem.remove_ushort("MinCmdLvlToIgnoreInactivity", 1);
    config.inactivity_warning_timeout = elem.remove_ushort( "InactivityWarningTimeout", 4 );
    config.inactivity_disconnect_timeout = elem.remove_ushort( "InactivityDisconnectTimeout", 5 );

    config.min_cmdlevel_to_login = elem.remove_ushort( "MinCmdlevelToLogin", 0 );

    escript_config.max_call_depth = elem.remove_ulong( "MaxCallDepth", 100 );
    passert_disabled = !elem.remove_bool( "EnableAssertions", true );
    passert_dump_stack = elem.remove_bool( "DumpStackOnAssertionFailure", false );

    string tmp = elem.remove_string( "AssertionFailureAction", "abort" );
    if (strlower( tmp ) == "abort" )
    {
        passert_shutdown = false;
        passert_nosave = false;
        passert_abort = true;
        config.assertion_shutdown_save_type = SAVE_FULL; // should never come into play
    }
    else if (strlower( tmp ) == "continue")
    {
        passert_shutdown = false;
        passert_nosave = false;
        passert_abort = false;
        config.assertion_shutdown_save_type = SAVE_FULL; // should never come into play
    }
    else if (strlower( tmp ) == "shutdown")
    {
        passert_shutdown = true;
        passert_nosave = false;
        passert_abort = false;
        config.assertion_shutdown_save_type = SAVE_FULL;
    }
    else if (strlower( tmp ) == "shutdown-nosave" )
    {
        passert_shutdown = true;
        passert_nosave = true;
        passert_abort = false;
        config.assertion_shutdown_save_type = SAVE_FULL; // should never come into play
    }
    else if (strlower( tmp ) == "shutdown-save-full")
    {
        passert_shutdown = true;
        passert_nosave = false;
        passert_abort = false;
        config.assertion_shutdown_save_type = SAVE_FULL;
    }
    else if (strlower( tmp ) == "shutdown-save-incremental")
    {
        passert_shutdown = true;
        passert_nosave = false;
        passert_abort = false;
        config.assertion_shutdown_save_type = SAVE_INCREMENTAL;
    }
    else
    {
        passert_shutdown = false;
        passert_abort = true;
        config.assertion_shutdown_save_type = SAVE_FULL; // should never come into play
        Log2( "Unknown pol.cfg AssertionFailureAction value: %s (expected abort, continue, shutdown, or shutdown-nosave)\n", tmp.c_str() );
    }

    tmp = elem.remove_string( "ShutdownSaveType", "full" );
    if (strlower( tmp ) == "full")
    {
        config.shutdown_save_type = SAVE_FULL;
    }
    else if (strlower( tmp ) == "incremental")
    {
        config.shutdown_save_type = SAVE_INCREMENTAL;
    }
    else
    {
        config.shutdown_save_type = SAVE_FULL;
        Log2( "Unknown pol.cfg ShutdownSaveType value: %s (expected full or incremental)\n", tmp.c_str() );
    }

    CalculateCryptKeys(elem.remove_string( "ClientEncryptionVersion", "none" ), config.client_encryption_version);

    config.display_unknown_packets = elem.remove_bool( "DisplayUnknownPackets", false );
    config.exp_los_checks_map = elem.remove_bool( "ExpLosChecksMap", true );
    config.enable_debug_log = elem.remove_bool( "EnableDebugLog", false );
    config.debug_password = elem.remove_string( "DebugPassword", "" );
    config.debug_local_only = elem.remove_bool( "DebugLocalOnly", true );
	
    config.report_rtc_scripts = elem.remove_bool( "ReportRunToCompletionScripts", true);
	config.report_critical_scripts = elem.remove_bool( "ReportCriticalScripts", true);
	config.report_missing_configs = elem.remove_bool( "ReportMissingConfigs", true);
	config.max_clients = elem.remove_ushort( "MaximumClients", 400 );
	config.character_slots = elem.remove_ushort("CharacterSlots", 5);
	config.max_clients_bypass_cmdlevel = elem.remove_ushort( "MaximumClientsBypassCmdLevel", 1);
	config.minidump_type = elem.remove_string("MiniDumpType","variable");
	config.retain_cleartext_passwords = elem.remove_bool( "RetainCleartextPasswords", false);
	config.discard_old_events = elem.remove_bool("DiscardOldEvents",false);

	config.locale = elem.remove_string("locale", "C");


    LogfileTimestampEveryLine = elem.remove_bool("TimestampEveryLine",false); // clib/logfile.h bool
#ifdef _WIN32
    MiniDumper::SetMiniDumpType( config.minidump_type );
#endif

    if (config.enable_debug_log && !mlog.is_open())
    {
        mlog.open( "log/debug.log", ios::out|ios::app );
    }
    else if (!config.enable_debug_log && mlog.is_open())
    {
        mlog.close();
    }

    config.debug_level = elem.remove_ushort( "DebugLevel", 0 );
}

void reload_pol_cfg(void)
{
    // cout << "checking for pol.cfg changes" << endl;
    THREAD_CHECKPOINT( tasks, 600 );
    try
    {
        struct stat newst;
        stat( "pol.cfg", &newst );
        
        // cout << "os: " << pol_cfg_stat.st_mtime << endl;
        // cout << "ns: " << newst.st_mtime << endl;
        
        if ((newst.st_mtime != pol_cfg_stat.st_mtime) &&
            (newst.st_mtime < time(NULL)-10))
        {
            cout << "Reloading pol.cfg...";
            memcpy( &pol_cfg_stat, &newst, sizeof pol_cfg_stat );
            
            read_pol_config( false );
            cout << "Done!" << endl;
            
        }
    }
    catch( std::exception& ex )
    {
        cout << "Error rereading pol.cfg: " << ex.what() << endl;
    }
    THREAD_CHECKPOINT( tasks, 699 );
}

PeriodicTask reload_pol_cfg_task( reload_pol_cfg, 30, "LOADPOLCFG" );
