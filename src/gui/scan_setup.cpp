/*
	$Id: scan_setup.cpp 20.03.2022 mohousch Exp $

	Neutrino-GUI  -   DBoxII-Project

	scan setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

	License: GPL

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui/scan_setup.h"

#include <global.h>
#include <neutrino2.h>

#include "gui/scan.h"
#include "gui/motorcontrol.h"

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/hintbox.h>

#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"

#include <system/debug.h>

#include <global.h>

//zapit includes
#include <zapit/frontend_c.h>
#include <zapit/getservices.h>
#include <zapit/satconfig.h>


extern satellite_map_t satellitePositions;					// defined in getServices.cpp
extern std::map<transponder_id_t, transponder> select_transponders;		// defined in getservices.cpp

// global
CScanSettings * scanSettings;
static int dmode = NO_DISEQC;

char zapit_lat[20];				//defined neutrino.cpp
char zapit_long[20];				//defined neutrino.cpp

// frontend
extern int FrontendCount;			// defined in zapit.cpp

// option off0_on1
#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
        { 0, _("off") },
        { 1, _("on") }
};

// option off1 on0
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
        { 1, _("off") },
        { 0, _("on") }
};

#define SCANTS_BOUQUET_OPTION_COUNT 4
const keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	{ CZapit::BM_DELETEBOUQUETS, _("erase all") },
	{ CZapit::BM_DONTTOUCHBOUQUETS, _("leave current") },
	{ CZapit::BM_UPDATEBOUQUETS, _("update") },
	{ CZapit::BM_CREATESATELLITEBOUQUET, _("create satellite bouquets") }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapit::ST_TVRADIO, _("TV & Radio") },
	{  CZapit::ST_TV, _("only tv") },
	{  CZapit::ST_RADIO, _("only radio") },
	{  CZapit::ST_ALL, _("all services") }
};

#define SATSETUP_DISEQC_OPTION_COUNT 8
const keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ NO_DISEQC          , _("no DiSEqC") },
	{ MINI_DISEQC        , _("Mini-DiSEqC") },
	{ DISEQC_1_0         , _("DiSEqC 1.0") },
	{ DISEQC_1_1         , _("DiSEqC 1.1") },
	{ DISEQC_ADVANCED    , _("Advanced") },
	{ DISEQC_UNICABLE    , _("Unicable") },
	{ DISEQC_UNICABLE2   , _("Unicable2 (JESS)") },
	{ SMATV_REMOTE_TUNING, _("SMATV Remote Tuning") }
};

#define SATSETUP_SCANTP_FEC_COUNT 24
#define CABLESETUP_SCANTP_FEC_COUNT 6
const keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
	{ FEC_NONE, _("FEC_NONE") },
	
        { FEC_1_2, "1/2" },
        { FEC_2_3, "2/3" },
        { FEC_3_4, "3/4" },
        { FEC_5_6, "5/6" },
        { FEC_7_8, "7/8" },

        { FEC_S2_QPSK_1_2, "1/2 s2 qpsk" },
        { FEC_S2_QPSK_2_3, "3/2 s2 qpsk" },
        { FEC_S2_QPSK_3_4, "3/4 s2 qpsk" },
        { FEC_S2_QPSK_5_6, "5/6 s2 qpsk" },
        { FEC_S2_QPSK_7_8, "7/8 s2 qpsk" },
        { FEC_S2_QPSK_8_9, "8/9 s2 qpsk" },
        { FEC_S2_QPSK_3_5, "3/5 s2 qpsk" },
        { FEC_S2_QPSK_4_5, "4/5 s2 qpsk" },
        { FEC_S2_QPSK_9_10, "9/10 s2 qpsk" },

        { FEC_S2_8PSK_1_2, "1/2 s2 8psk" },
        { FEC_S2_8PSK_2_3, "3/2 s2 8psk" },
        { FEC_S2_8PSK_3_4, "3/4 s2 8psk" },
        { FEC_S2_8PSK_5_6, "5/6 s2 8psk" },
        { FEC_S2_8PSK_7_8, "7/8 s2 8psk" },
        { FEC_S2_8PSK_8_9, "8/9 s2 8psk" },
        { FEC_S2_8PSK_3_5, "3/5 s2 8psk" },
        { FEC_S2_8PSK_4_5, "4/5 s2 8psk" },
        { FEC_S2_8PSK_9_10, "9/10 s2 8psk" }
};

#define CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT 6
const keyval CABLETERRESTRIALSETUP_SCANTP_MOD[CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT] =
{
	// cable
	{ QAM_16, "QAM/16" },
	{ QAM_32, "QAM/32" },
	{ QAM_64, "QAM/64" },
	{ QAM_128, "QAM/128" },
	{ QAM_256, "QAM/256" },
	{ QAM_AUTO, "QAM_AUTO" }
};

#define SATSETUP_SCANTP_MOD_COUNT 2
const keyval SATSETUP_SCANTP_MOD[SATSETUP_SCANTP_MOD_COUNT] =
{
	// sat
	{ QPSK, "QPSK" },
#if HAVE_DVB_API_VERSION >= 5
	{ PSK_8, "PSK_8" }
#else
	{ PSK8, "PSK8" }
#endif
};

#define SATSETUP_SCANTP_BAND_COUNT 4
const keyval SATSETUP_SCANTP_BAND[SATSETUP_SCANTP_BAND_COUNT] =
{
	{ BANDWIDTH_8_MHZ, "BAND_8" },
	{ BANDWIDTH_7_MHZ, "BAND_7" },
	{ BANDWIDTH_6_MHZ, "BAND_6" },
	{ BANDWIDTH_AUTO, "BAND_AUTO"}
};

// transmition mode
#define TERRESTRIALSETUP_TRANSMIT_MODE_COUNT 3
const keyval TERRESTRIALSETUP_TRANSMIT_MODE[TERRESTRIALSETUP_TRANSMIT_MODE_COUNT] =
{
	{ TRANSMISSION_MODE_2K, "2K" },
	{ TRANSMISSION_MODE_8K, "8K" },
	{ TRANSMISSION_MODE_AUTO, "AUTO" },
};

// guard interval
#define TERRESTRIALSETUP_GUARD_INTERVAL_COUNT 5
const keyval TERRESTRIALSETUP_GUARD_INTERVAL[TERRESTRIALSETUP_GUARD_INTERVAL_COUNT] =
{
	{ GUARD_INTERVAL_1_32, "1_32" },
	{ GUARD_INTERVAL_1_16, "1_16" },
	{ GUARD_INTERVAL_1_8, "1_8" },
	{ GUARD_INTERVAL_1_4, "1_4"},
	{ GUARD_INTERVAL_AUTO, "AUTO"},
};

// hierarchy
#define TERRESTRIALSETUP_HIERARCHY_COUNT 5
const keyval TERRESTRIALSETUP_HIERARCHY[TERRESTRIALSETUP_HIERARCHY_COUNT] =
{
	{ HIERARCHY_NONE, "NONE" },
	{ HIERARCHY_1, "1" },
	{ HIERARCHY_2, "2" },
	{ HIERARCHY_4, "4"},
	{ HIERARCHY_AUTO, "AUTO"},
};

#define SATSETUP_SCANTP_POL_COUNT 2
const keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, "H" },
	{ 1, "V" }
};

#define DISEQC_ORDER_OPTION_COUNT 2
const keyval DISEQC_ORDER_OPTIONS[DISEQC_ORDER_OPTION_COUNT] =
{
	{ COMMITED_FIRST, _("Commited/Uncommited") },
	{ UNCOMMITED_FIRST, _("Uncommited/Commited") }
};

#define OPTIONS_SOUTH0_NORTH1_OPTION_COUNT 2
const keyval OPTIONS_SOUTH0_NORTH1_OPTIONS[OPTIONS_SOUTH0_NORTH1_OPTION_COUNT] =
{
	{0, _("South") },
	{1, _("North") }
};

#define OPTIONS_EAST0_WEST1_OPTION_COUNT 2
const keyval OPTIONS_EAST0_WEST1_OPTIONS[OPTIONS_EAST0_WEST1_OPTION_COUNT] =
{
	{0, _("East") },
	{1, _("West") }
};

// 
#define FRONTEND_MODE_SINGLE_OPTION_COUNT 2
#define FRONTEND_MODE_TWIN_OPTION_COUNT 3
const keyval FRONTEND_MODE_OPTIONS[FRONTEND_MODE_TWIN_OPTION_COUNT] =
{
	{ (fe_mode_t)FE_SINGLE, _("direct connected") },
	{ (fe_mode_t)FE_NOTCONNECTED, _("not connected") },
	{ (fe_mode_t)FE_LOOP, _("loop") },
};

//
#define FRONTEND_DELSYS_OPTION_COUNT 3
const keyval FRONTEND_DELSYS_OPTIONS[FRONTEND_DELSYS_OPTION_COUNT]
{
//	{ UNDEFINED, "UNDEFINED" },
//	{ DVB_S, "DVBS" },
//	{ DVB_S2, "DVBS2" },
//	{ DVB_S2X, "DVBS2X" },
	{ DVB_C, "DVBC" },
	{ DVB_T, "DVBT" },
	{ DVB_T2, "DVBT2" },
//	{ DVB_DTMB, "DVBDTMB" },
//	{ DVB_A, "DVBA" }
};

CScanSetup::CScanSetup(int num)
{
	feindex = num;
	
	scanSettings = new CScanSettings(feindex);
}

CScanSetup::~CScanSetup()
{	
}

void CScanSetup::hide()
{
	CFrameBuffer::getInstance()->paintBackground();
	CFrameBuffer::getInstance()->blit();
}

int CScanSetup::exec(CMenuTarget * parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "CScanSetup::exec: actionKey: %s\n", actionKey.c_str());
	
	if (parent)
		parent->hide();

	if(actionKey == "save_scansettings") 
	{
		// hint box
		CHintBox * hintBox = new CHintBox(_("Information"), _("Save settings now")); // UTF-8
		hintBox->paint();
		
		// save scan.conf
		if(!scanSettings->saveSettings(NEUTRINO_SCAN_SETTINGS_FILE, feindex)) 
			dprintf(DEBUG_NORMAL, "CNeutrinoApp::exec: error while saving scan-settings!\n");
		
		// send directly diseqc
#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
		{
			CServices::getInstance()->saveMotorPositions();
			
			//diseqc type
			CZapit::getInstance()->getFE(feindex)->setDiseqcType((diseqc_t)CZapit::getInstance()->getFE(feindex)->diseqcType);
			
			// diseqc repeat
			CZapit::getInstance()->getFE(feindex)->setDiseqcRepeats(CZapit::getInstance()->getFE(feindex)->diseqcRepeats);
		
			//gotoxx
			CZapit::getInstance()->getFE(feindex)->gotoXXLatitude = strtod(zapit_lat, NULL);
			CZapit::getInstance()->getFE(feindex)->gotoXXLongitude = strtod(zapit_long, NULL);
		}
		
		// set fe mode
		CZapit::getInstance()->setFEMode(CZapit::getInstance()->getFE(feindex)->mode, feindex);
		
		// set fe delsys
		//CZapit::getInstance()->getFE(feindex)->changeDelSys(CZapit::getInstance()->getFE(feindex)->forcedDelSys);
		
		// save frontend.conf
		CZapit::getInstance()->saveFrontendConfig(feindex);
		
		hintBox->hide();
		delete hintBox;
		hintBox = NULL;
		
		//return RETURN_REPAINT;
		hide();
		showScanService();
		
		return RETURN_EXIT_ALL;
	}
	else if(actionKey == "unisetup") 
	{
		return showUnicableSetup();
	}
	
	showScanService();
	
	return RETURN_REPAINT;
}

void CScanSetup::showScanService()
{
	dprintf(DEBUG_NORMAL, "CScanSetup::showScanService: Tuner: %d\n", feindex);
	
	if(!CZapit::getInstance()->getFE(feindex))
		return;
	
	//load scan settings 
	if( !scanSettings->loadSettings(NEUTRINO_SCAN_SETTINGS_FILE, feindex) ) 
		dprintf(DEBUG_NORMAL, "CScanSetup::CScanSetup: Loading of scan settings failed. Using defaults.\n");
	
	//
	CWidget* widget = NULL;
	ClistBox* scansetup = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("scansetup");
	
	if (widget)
	{
		scansetup = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		scansetup = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		scansetup->setWidgetMode(MODE_SETUP);
		scansetup->enableShrinkMenu();
		
		scansetup->enablePaintHead();
		scansetup->setTitle(_("Scan transponder"), NEUTRINO_ICON_SCAN);

		scansetup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		scansetup->setFootButtons(&btn);
		
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "scansetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		widget->addWidgetItem(scansetup);
	}

	scansetup->clearItems();
	
	// 
	dmode = CZapit::getInstance()->getFE(feindex)->diseqcType;
	
	// load frontend config
	CZapit::getInstance()->loadFrontendConfig();
	
	//
	CZapit::getInstance()->initTuner(CZapit::getInstance()->getFE(feindex));
	
	// load motor position
#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
		CServices::getInstance()->loadMotorPositions();
	
	// intros
	scansetup->addItem(new ClistBoxItem(_("back")));
	scansetup->addItem(new CMenuSeparator(LINE));
	
	//save settings
	scansetup->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	scansetup->addItem(new CMenuSeparator(LINE));
			
	// init satNotify
	CSatelliteSetupNotifier * satNotify = new CSatelliteSetupNotifier(feindex);
	CScanSetupNotifier * feModeNotifier = new CScanSetupNotifier(feindex);
	CScanSetupDelSysNotifier * feDelSysNotifier = new CScanSetupDelSysNotifier(feindex);
	
	// satsetup
	CWidget* satSetupWidget = NULL;
	ClistBox* satSetup = NULL;
	
	satSetupWidget = CNeutrinoApp::getInstance()->getWidget("satsetup");
	
	if (satSetupWidget)
	{
		satSetup = (ClistBox*)satSetupWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		satSetup = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		satSetup->setWidgetMode(MODE_SETUP);
		satSetup->enableShrinkMenu();
		
		satSetup->enablePaintHead();
		satSetup->setTitle(_("Sat settings"), NEUTRINO_ICON_SCAN);

		satSetup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		satSetup->setFootButtons(&btn);
		
		//
		satSetupWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		satSetupWidget->name = "satsetup";
		satSetupWidget->setMenuPosition(MENU_POSITION_CENTER);
		satSetupWidget->addWidgetItem(satSetup);
	}

	satSetup->clearItems();
	
	satSetup->addItem(new ClistBoxItem(_("back")));
	satSetup->addItem(new CMenuSeparator(LINE));

	// motor settings
	CWidget* satfindMenuWidget = NULL;
	ClistBox* satfindMenu = NULL;
	
	satfindMenuWidget = CNeutrinoApp::getInstance()->getWidget("satfindMenu");
	
	if (satfindMenuWidget)
	{
		satfindMenu = (ClistBox*)satfindMenuWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		satfindMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		satfindMenu->setWidgetMode(MODE_SETUP);
		satfindMenu->enableShrinkMenu();
		
		satfindMenu->enablePaintHead();
		satfindMenu->setTitle(_("Motor settings"), NEUTRINO_ICON_SCAN);

		satfindMenu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		satfindMenu->setFootButtons(&btn);
		
		//
		satfindMenuWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		satfindMenuWidget->name = "satfindMenu";
		satfindMenuWidget->setMenuPosition(MENU_POSITION_CENTER);
		satfindMenuWidget->addWidgetItem(satfindMenu);
	}
	
	satfindMenu->clearItems();

	satfindMenu->addItem(new ClistBoxItem(_("back")));
	satfindMenu->addItem(new CMenuSeparator(LINE));
		
	//// satselect (list)
	CMenuOptionStringChooser * satSelect = NULL;
	CWidget* satOnOffWidget = NULL;
	ClistBox* satOnOfflistBox = NULL;
	
#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif 
	{
		satSelect = new CMenuOptionStringChooser(_("Satellite"), scanSettings->satNameNoDiseqc, true, NULL, RC_nokey, "", true);
			
		//
		satOnOffWidget = CNeutrinoApp::getInstance()->getWidget("satOnOff");
		
		if (satOnOffWidget)
		{
			satOnOfflistBox = (ClistBox*)satOnOffWidget->getWidgetItem(WIDGETITEM_LISTBOX);
		}
		else
		{
			satOnOfflistBox = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

			satOnOfflistBox->setWidgetMode(MODE_SETUP);
			satOnOfflistBox->enableShrinkMenu();
			
			satOnOfflistBox->enablePaintHead();
			satOnOfflistBox->setTitle(_("Satellite"), NEUTRINO_ICON_SCAN);

			satOnOfflistBox->enablePaintFoot();
				
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
			satOnOfflistBox->setFootButtons(&btn);
			
			//
			satOnOffWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			satOnOffWidget->name = "satOnOff";
			satOnOffWidget->setMenuPosition(MENU_POSITION_CENTER);
			satOnOffWidget->addWidgetItem(satOnOfflistBox);
		}
		
		satOnOfflistBox->clearItems();
	
		// intros
		satOnOfflistBox->addItem(new ClistBoxItem(_("back")));
		satOnOfflistBox->addItem(new CMenuSeparator(LINE));
		
		//
		CWidget* tempsatWidget = NULL;
		ClistBox* tempsatlistBox = NULL;

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			// satname
			if(sit->second.system == DVB_S)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "[neutrino] fe(%d) Adding sat menu for %s position %d\n", feindex, sit->second.name.c_str(), sit->first);

				//
				tempsatWidget = CNeutrinoApp::getInstance()->getWidget("tempsat");
				
				if (tempsatWidget)
				{
					tempsatlistBox = (ClistBox*)tempsatWidget->getWidgetItem(WIDGETITEM_LISTBOX);
					
					if (tempsatlistBox->hasHead())
						tempsatlistBox->setTitle(sit->second.name.c_str(), NEUTRINO_ICON_SCAN);
				}
				else
				{
					tempsatlistBox = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

					tempsatlistBox->setWidgetMode(MODE_SETUP);
					tempsatlistBox->enableShrinkMenu();
					
					//
					tempsatlistBox->enablePaintHead();
					tempsatlistBox->setTitle(sit->second.name.c_str(), NEUTRINO_ICON_SCAN);
					
					//
					tempsatlistBox->enablePaintFoot();		
					const struct button_label btn = { NEUTRINO_ICON_INFO, " "};		
					tempsatlistBox->setFootButtons(&btn);
					
					//
					tempsatWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
					tempsatWidget->name = "tempsat";
					tempsatWidget->setMenuPosition(MENU_POSITION_CENTER);
					tempsatWidget->enableSaveScreen();
					tempsatWidget->addWidgetItem(tempsatlistBox);
				}
				
				tempsatlistBox->clear();
				
				//
				tempsatlistBox->addItem(new ClistBoxItem(_("back")));
				tempsatlistBox->addItem(new CMenuSeparator(LINE));
				
				// savesettings
				tempsatlistBox->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
				tempsatlistBox->addItem(new CMenuSeparator(LINE));

				// satname
				CMenuOptionChooser * inuse = new CMenuOptionChooser(sit->second.name.c_str(),  &sit->second.use_in_scan, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

				// diseqc
				CMenuOptionNumberChooser * diseqc = new CMenuOptionNumberChooser(_("Diseqc input"), &sit->second.diseqc, ((dmode != NO_DISEQC) && (dmode != DISEQC_ADVANCED)), -1, 15, NULL, 1, -1);

				// commited input
				CMenuOptionNumberChooser * comm = new CMenuOptionNumberChooser(_("Commited/Uncommited"), &sit->second.commited, dmode == DISEQC_ADVANCED, -1, 15, NULL, 1, -1);

				// uncommited input
				CMenuOptionNumberChooser * uncomm = new CMenuOptionNumberChooser(_("Uncommited/Commited"), &sit->second.uncommited, dmode == DISEQC_ADVANCED, -1, 15, NULL, 1, -1);

				// motor position
				CMenuOptionNumberChooser * motor = new CMenuOptionNumberChooser(_("Rotor position"), &sit->second.motor_position, true, 0, 64);

				// usals
				CMenuOptionChooser * usals = new CMenuOptionChooser(_("Use gotoXX"),  &sit->second.use_usals, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

				satNotify->addItem(1, diseqc);
				satNotify->addItem(0, comm);
				satNotify->addItem(0, uncomm);

				CIntInput* lofL = new CIntInput(_("LNB Low Offset"), (int&) sit->second.lnbOffsetLow, 5);
				CIntInput* lofH = new CIntInput(_("LNB High Offset"), (int&) sit->second.lnbOffsetHigh, 5);
				CIntInput* lofS = new CIntInput(_("LNB switch Offset"), (int&) sit->second.lnbSwitch, 5);

				satOnOfflistBox->addItem(inuse);
					
				tempsatlistBox->addItem(diseqc);
				tempsatlistBox->addItem(comm);
				tempsatlistBox->addItem(uncomm);
				tempsatlistBox->addItem(motor);
				tempsatlistBox->addItem(usals);
				tempsatlistBox->addItem(new ClistBoxItem(_("LNB Low Offset"), true, lofL->getValue(), lofL ));
				tempsatlistBox->addItem(new ClistBoxItem(_("LNB High Offset"), true, lofH->getValue(), lofH ));
				tempsatlistBox->addItem(new ClistBoxItem(_("LNB switch Offset"), true, lofS->getValue(), lofS));
					
				// sat setup
				satSetup->addItem(new ClistBoxItem(sit->second.name.c_str(), true, NULL, tempsatWidget));
			}
		}
		
		if (tempsatlistBox)
		{
			delete tempsatlistBox;
			tempsatlistBox = NULL;
		}
		
		if (tempsatWidget)
		{
			delete tempsatWidget;
			tempsatWidget = NULL;
		}
	} 
#if HAVE_DVB_API_VERSION >= 5 
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM )
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Cable"), (char*)scanSettings->satNameNoDiseqc, true, NULL, RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
		{
			if(sit->second.system == DVB_C)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "[neutrino] fe(%d) Adding cable menu for %s position %d\n", feindex, sit->second.name.c_str(), sit->first);
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) 
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == DVB_T)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "CNeutrinoApp::InitScanSettings fe(%d) Adding terrestrial menu for %s position %d\n", feindex, sit->second.name.c_str(), sit->first);
			}
		}
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_A)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
#endif
	{
		satSelect = new CMenuOptionStringChooser(_("Provider"), (char*)scanSettings->satNameNoDiseqc, true, NULL, RC_nokey, "", true);

		for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++)
		{
			if(sit->second.system == DVB_A)
			{
				satSelect->addOption(sit->second.name.c_str());
				dprintf(DEBUG_DEBUG, "CNeutrinoApp::InitScanSettings fe(%d) Adding atsc menu for %s position %d\n", feindex, sit->second.name.c_str(), sit->first);
			}
		}
	}
	////

	//
	satfindMenu->addItem(satSelect);

	// motor menu/diseqc
	CWidget* motorMenuWidget = NULL;
	ClistBox* motorMenu = NULL;

	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK) 
	{
		satfindMenu->addItem(new ClistBoxItem(_("Motor settings"), true, NULL, new CMotorControl(feindex)));
		
		//
		motorMenuWidget = CNeutrinoApp::getInstance()->getWidget("motorsetup");
		
		if (motorMenuWidget)
		{
			motorMenu = (ClistBox*)motorMenuWidget->getWidgetItem(WIDGETITEM_LISTBOX);
		}
		else
		{
			motorMenu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

			motorMenu->setWidgetMode(MODE_SETUP);
			motorMenu->enableShrinkMenu();
			
			motorMenu->enablePaintHead();
			motorMenu->setTitle(_("Motor settings"), NEUTRINO_ICON_SCAN);

			motorMenu->enablePaintFoot();
				
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
			motorMenu->setFootButtons(&btn);
			
			//
			motorMenuWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			motorMenuWidget->name = "motorsetup";
			motorMenuWidget->setMenuPosition(MENU_POSITION_CENTER);
			motorMenuWidget->addWidgetItem(motorMenu);
		}

		motorMenu->clearItems();
		
		// intros
		motorMenu->addItem(new CMenuSeparator(LINE));
		motorMenu->addItem(new ClistBoxItem(_("back")));

		// save settings
		motorMenu->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));

		motorMenu->addItem(new ClistBoxItem(_("Motor settings"), true, NULL, satfindMenuWidget, ""));

		motorMenu->addItem(new CMenuSeparator(LINE));

		motorMenu->addItem(new CMenuOptionNumberChooser(_("moving speed (10 = 1deg/sec)"), (int *)&CZapit::getInstance()->getFE(feindex)->motorRotationSpeed, true, 0, 64, NULL) );

		motorMenu->addItem(new CMenuOptionChooser(_("Use gotoXX"),  (int *)&CZapit::getInstance()->getFE(feindex)->useGotoXX, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

		CStringInput * toff;
		CStringInput * taff;
		sprintf(zapit_lat, "%3.6f", CZapit::getInstance()->getFE(feindex)->gotoXXLatitude);
		sprintf(zapit_long, "%3.6f", CZapit::getInstance()->getFE(feindex)->gotoXXLongitude);

		// gotoxxladirection
		motorMenu->addItem(new CMenuOptionChooser(_("LaDirection"),  (int *)&CZapit::getInstance()->getFE(feindex)->gotoXXLaDirection, OPTIONS_SOUTH0_NORTH1_OPTIONS, OPTIONS_SOUTH0_NORTH1_OPTION_COUNT, true));

		// latitude
		toff = new CStringInput(_("Latitude"), (char *) zapit_lat, 10, NULL, NULL, "0123456789.");
		motorMenu->addItem(new ClistBoxItem(_("Latitude"), true, zapit_lat, toff));

		// gotoxx lodirection
		motorMenu->addItem(new CMenuOptionChooser(_("LoDirection"),  (int *)&CZapit::getInstance()->getFE(feindex)->gotoXXLoDirection, OPTIONS_EAST0_WEST1_OPTIONS, OPTIONS_EAST0_WEST1_OPTION_COUNT, true));

		// longitude
		taff = new CStringInput(_("Longitude"), (char *) zapit_long, 10, NULL, NULL, "0123456789.");
		motorMenu->addItem(new ClistBoxItem(_("Longitude"), true, zapit_long, taff));
		
		// usals repeat
		motorMenu->addItem(new CMenuOptionNumberChooser(_("USALS command repeat"), (int *)&CZapit::getInstance()->getFE(feindex)->repeatUsals, true, 0, 10) );
		
		// rotor swap east/west
		motorMenu->addItem( new CMenuOptionChooser(_("Swap rotor east/west"), &g_settings.rotor_swap, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true ));
	}
	
	// frontend mode
	// check for twin
	// mode loop can be used if we hat twice sat tuner, otherwise direct connected or not connected
	// FIXME:
	bool have_twin = false;
	
	//if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK || CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM)
	{
		for(int i = 0; i < FrontendCount; i++) 
		{
			if( i != feindex && ( CZapit::getInstance()->getFE(i)->getInfo()->type == CZapit::getInstance()->getFE(feindex)->getInfo()->type) )
			{
				have_twin = true;
				break;
			}
		}
	}
	
	scansetup->addItem(new CMenuOptionChooser(_("Tuner mode"),  (int *)&CZapit::getInstance()->getFE(feindex)->mode, FRONTEND_MODE_OPTIONS, have_twin? FRONTEND_MODE_TWIN_OPTION_COUNT:FRONTEND_MODE_SINGLE_OPTION_COUNT, true, feModeNotifier));
	
	// frontend delsys
	if (CZapit::getInstance()->getFE(feindex)->isHybrid())
		scansetup->addItem(new CMenuOptionChooser(_("Tuner type"),  (int *)&CZapit::getInstance()->getFE(feindex)->forcedDelSys, FRONTEND_DELSYS_OPTIONS, FRONTEND_DELSYS_OPTION_COUNT, true, feDelSysNotifier));
	
	scansetup->addItem( new CMenuSeparator(LINE) );

	// scan type
	CMenuOptionChooser * ojScantype = new CMenuOptionChooser(_("Scan for services"), (int *)&scanSettings->scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, ((CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP)));
	feModeNotifier->addItem(0, ojScantype);
	scansetup->addItem(ojScantype);
		
	// bqts
	CMenuOptionChooser * ojBouquets = new CMenuOptionChooser(_("Bouquet"), (int *)&scanSettings->bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, ((CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP)));
	feModeNotifier->addItem(0, ojBouquets);
	scansetup->addItem(ojBouquets);
	
	// NIT
	CMenuOptionChooser * useNit = new CMenuOptionChooser(_("Use NIT"), (int *)&scanSettings->scan_mode, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, ( (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP) ));
	feModeNotifier->addItem(0, useNit);
	scansetup->addItem(useNit);
		
	scansetup->addItem(new CMenuSeparator(LINE));
		
	// diseqc/diseqcrepeat/unisetup/lnb/motor
	CMenuOptionChooser * ojDiseqc = NULL;
	CMenuOptionNumberChooser * ojDiseqcRepeats = NULL;
	ClistBoxItem * fsatSetup = NULL;
	ClistBoxItem * fmotorMenu = NULL;
	ClistBoxItem * uniSetup = NULL;

#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
	{
		// diseqc
		ojDiseqc = new CMenuOptionChooser(_("DiSEqC"), (int *)&CZapit::getInstance()->getFE(feindex)->diseqcType, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, ( (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != FE_LOOP) ), satNotify, RC_nokey, "", true);
		feModeNotifier->addItem(1, ojDiseqc);
		
		// diseqc repeat
		ojDiseqcRepeats = new CMenuOptionNumberChooser(_("DiSEqC-repeats"), &CZapit::getInstance()->getFE(feindex)->diseqcRepeats, (dmode != NO_DISEQC) && \
			(dmode < DISEQC_ADVANCED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != FE_LOOP), 0, 2, NULL);
		satNotify->addItem(4, ojDiseqcRepeats);
		feModeNotifier->addItem(4, ojDiseqcRepeats);

		// unicable setup
		uniSetup = new ClistBoxItem(_("Unicable Setup"), (dmode > DISEQC_ADVANCED ? true : false), NULL, this, "unisetup", RC_nokey);
		satNotify->addItem(3, uniSetup);
		feModeNotifier->addItem(3, uniSetup);

		// lnb setup
		fsatSetup = new ClistBoxItem(_("Setup satellites input / LNB"), (CZapit::getInstance()->getFE(feindex)->mode != FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP), NULL, satSetupWidget, "", RC_nokey);
		feModeNotifier->addItem(1, fsatSetup);
		
		// motor settings
		fmotorMenu = new ClistBoxItem(_("Motor settings"), (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP), NULL, motorMenuWidget, "", RC_nokey);
		feModeNotifier->addItem(1, fmotorMenu);
		
		scansetup->addItem(ojDiseqc);
		scansetup->addItem(ojDiseqcRepeats);
		scansetup->addItem(uniSetup);
		scansetup->addItem(fsatSetup);
		scansetup->addItem(fmotorMenu);
	}
	
	//// manual scan
	CWidget* manualScanWidget = NULL;
	ClistBox* manualScanlistBox = NULL;
	
	manualScanWidget = CNeutrinoApp::getInstance()->getWidget("manualscan");
	
	if (manualScanWidget)
	{
		manualScanlistBox = (ClistBox*)manualScanWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		manualScanlistBox = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		manualScanlistBox->setWidgetMode(MODE_SETUP);
		manualScanlistBox->enableShrinkMenu();
		
		manualScanlistBox->enablePaintHead();
		manualScanlistBox->setTitle(_("Manual frequency scan / Test signal"), NEUTRINO_ICON_SCAN);

		manualScanlistBox->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		manualScanlistBox->setFootButtons(&btn);
		
		//
		manualScanWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		manualScanWidget->name = "manualscan";
		manualScanWidget->setMenuPosition(MENU_POSITION_CENTER);
		manualScanWidget->addWidgetItem(manualScanlistBox);
	}
	
	manualScanlistBox->clearItems();

	//
	CScanTs * scanTs = new CScanTs(feindex);

	// intros
	manualScanlistBox->addItem(new ClistBoxItem(_("back")));
	manualScanlistBox->addItem(new CMenuSeparator(LINE));
	
	// save settings
	manualScanlistBox->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	manualScanlistBox->addItem(new CMenuSeparator(LINE));

	// sat select
	manualScanlistBox->addItem(satSelect);
		
	// TP select
	CTPSelectHandler * tpSelect = new CTPSelectHandler(feindex);	
	manualScanlistBox->addItem(new ClistBoxItem(_("Select transponder"), true, NULL, tpSelect));
		
	// frequency
	int freq_length = 8;

	switch (CZapit::getInstance()->getFE(feindex)->getInfo()->type)
	{
		case FE_QPSK:
		freq_length = 8;
		break;
		
		case FE_QAM:
		freq_length = 6;
		break;
		
		case FE_OFDM:
        case FE_ATSC:
		freq_length = 9;
		break;
		
		default:
		freq_length = 8;
		break;
	}
	
	CStringInput * freq = new CStringInput(_("Frequency"), (char *) scanSettings->TP_freq, freq_length, NULL, NULL, "0123456789");
	ClistBoxItem * Freq = new ClistBoxItem(_("Frequency"), true, scanSettings->TP_freq, freq);
		
	manualScanlistBox->addItem(Freq);
		
	// modulation(t/c)/polarisation(sat)
	CMenuOptionChooser * mod_pol = NULL;

#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Polarization"), (int *)&scanSettings->TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5 
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_C)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QAM )
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_mod, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5
	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) 
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_const, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}
#if HAVE_DVB_API_VERSION >= 5
    	else if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_A)
#else
	else if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_ATSC)
#endif
	{
		mod_pol = new CMenuOptionChooser(_("Modulation"), (int *)&scanSettings->TP_const, CABLETERRESTRIALSETUP_SCANTP_MOD, CABLETERRESTRIALSETUP_SCANTP_MOD_COUNT, true);
	}

	manualScanlistBox->addItem(mod_pol);

	// symbol rate
	CStringInput * rate = new CStringInput(_("Symbol rate"), (char *) scanSettings->TP_rate, 8, NULL, NULL, "0123456789");
	ClistBoxItem * Rate = new ClistBoxItem(_("Symbol rate"), true, scanSettings->TP_rate, rate);

	// fec
	int fec_count = ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK) ? SATSETUP_SCANTP_FEC_COUNT : CABLESETUP_SCANTP_FEC_COUNT;
	CMenuOptionChooser * fec = new CMenuOptionChooser(_("FEC"), (int *)&scanSettings->TP_fec, SATSETUP_SCANTP_FEC, fec_count, true);
		
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type != FE_OFDM && CZapit::getInstance()->getFE(feindex)->getInfo()->type != FE_ATSC)
	{
		// Rate
		manualScanlistBox->addItem(Rate);
			
		// fec
		manualScanlistBox->addItem(fec);
	}

	// band/hp/lp/
#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
#else
	if ( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_OFDM) 
#endif
	{
		// Band
		CMenuOptionChooser * Band = new CMenuOptionChooser(_("Bandwidth"), (int *)&scanSettings->TP_band, SATSETUP_SCANTP_BAND, SATSETUP_SCANTP_BAND_COUNT, true);
		manualScanlistBox->addItem(Band);

		// HP
		CMenuOptionChooser * HP = new CMenuOptionChooser(_("Code Rate HP"), (int *)&scanSettings->TP_HP, SATSETUP_SCANTP_FEC, fec_count, true);
		manualScanlistBox->addItem(HP);

		// LP
		CMenuOptionChooser * LP = new CMenuOptionChooser(_("Code Rate LP"), (int *)&scanSettings->TP_LP, SATSETUP_SCANTP_FEC, fec_count, true);
		manualScanlistBox->addItem(LP);
		
		// transmition mode
		CMenuOptionChooser * TM = new CMenuOptionChooser(_("Transmission mode"), (int *)&scanSettings->TP_trans, TERRESTRIALSETUP_TRANSMIT_MODE, TERRESTRIALSETUP_TRANSMIT_MODE_COUNT, true);
		manualScanlistBox->addItem(TM);
		
		// guard intervall
		CMenuOptionChooser * GI = new CMenuOptionChooser(_("Guard Interval"), (int *)&scanSettings->TP_guard, TERRESTRIALSETUP_GUARD_INTERVAL, TERRESTRIALSETUP_GUARD_INTERVAL_COUNT, true);
		manualScanlistBox->addItem(GI);
		
		// hierarchy
		CMenuOptionChooser * HR = new CMenuOptionChooser(_("Hierarchy"), (int *)&scanSettings->TP_hierarchy, TERRESTRIALSETUP_HIERARCHY, TERRESTRIALSETUP_HIERARCHY_COUNT, true);
		manualScanlistBox->addItem(HR);
		
		// plp_id
#if HAVE_DVB_API_VERSION >= 5
		if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
		{
			CStringInput * plp = new CStringInput(_("PLP ID"), (char *) scanSettings->TP_plp_id, 3);
			ClistBoxItem * plp_id = new ClistBoxItem(_("PLP ID"), true, scanSettings->TP_plp_id, plp);
			manualScanlistBox->addItem(plp_id);
		}
#endif
	}	

	manualScanlistBox->addItem(new CMenuSeparator(LINE));
		
	// testsignal
	manualScanlistBox->addItem(new ClistBoxItem(_("Test signal"), true, NULL, scanTs, "test") );
		
	// scan
	manualScanlistBox->addItem(new ClistBoxItem(_("Start scan"), true, NULL, scanTs, "manual") );
	////
		
	ClistBoxItem * manScan = new ClistBoxItem(_("Manual frequency scan / Test signal"), (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP), NULL, manualScanWidget, "");
	feModeNotifier->addItem(0, manScan);
	scansetup->addItem(manScan);
		
	//// autoscan
	CWidget* autoScanWidget = NULL;
	ClistBox* autoScanlistBox = NULL;
	
	autoScanWidget = CNeutrinoApp::getInstance()->getWidget("autoscan");
	
	if (autoScanWidget)
	{
		autoScanlistBox = (ClistBox*)autoScanWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		autoScanlistBox = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		autoScanlistBox->setWidgetMode(MODE_SETUP);
		autoScanlistBox->enableShrinkMenu();
		
		autoScanlistBox->enablePaintHead();
		autoScanlistBox->setTitle(_("Auto-Scan"), NEUTRINO_ICON_SCAN);

		autoScanlistBox->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		autoScanlistBox->setFootButtons(&btn);
		
		//
		autoScanWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		autoScanWidget->name = "autoscan";
		autoScanWidget->setMenuPosition(MENU_POSITION_CENTER);
		autoScanWidget->addWidgetItem(autoScanlistBox);
	}
	
	autoScanlistBox->clearItems();
	
	// intros
	autoScanlistBox->addItem(new ClistBoxItem(_("back")));
	autoScanlistBox->addItem(new CMenuSeparator(LINE));
	
	// savesettings
	autoScanlistBox->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
	autoScanlistBox->addItem(new CMenuSeparator(LINE));
		
	// satselect
	autoScanlistBox->addItem(satSelect);
		
	// scan
	autoScanlistBox->addItem(new ClistBoxItem(_("Start scan"), true, NULL, scanTs, "auto") );
	////
		
	//
	ClistBoxItem * auScan = new ClistBoxItem(_("Auto-Scan"), (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP), NULL, autoScanWidget, "");
	feModeNotifier->addItem(0, auScan);
	
	scansetup->addItem(auScan);

	//// scan all sats
	ClistBoxItem * fautoScanAll = NULL;
		
#if HAVE_DVB_API_VERSION >= 5
	if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S ||CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_S2)
#else
	if( CZapit::getInstance()->getFE(feindex)->getInfo()->type == FE_QPSK)
#endif
	{
		//
		CWidget* autoScanAllWidget = NULL;
		ClistBox* autoScanAlllistBox = NULL;
		
		autoScanAllWidget = CNeutrinoApp::getInstance()->getWidget("autoscanall");
		
		if (autoScanAllWidget)
		{
			autoScanAlllistBox = (ClistBox*)autoScanAllWidget->getWidgetItem(WIDGETITEM_LISTBOX);
		}
		else
		{
			autoScanAlllistBox = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

			autoScanAlllistBox->setWidgetMode(MODE_SETUP);
			autoScanAlllistBox->enableShrinkMenu();
			
			autoScanAlllistBox->enablePaintHead();
			autoScanAlllistBox->setTitle(_("Auto-Scan multiple Satellites"), NEUTRINO_ICON_SCAN);

			autoScanAlllistBox->enablePaintFoot();
				
			const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
				
			autoScanAlllistBox->setFootButtons(&btn);
			
			//
			autoScanAllWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
			autoScanAllWidget->name = "autoscanall";
			autoScanAllWidget->setMenuPosition(MENU_POSITION_CENTER);
			autoScanAllWidget->addWidgetItem(autoScanAlllistBox);
		}
		
		autoScanAlllistBox->clearItems();
			
		//fautoScanAll = new ClistBoxItem(_("Auto-Scan multiple Satellites"), ( (dmode != NO_DISEQC) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP)), NULL, autoScanAllWidget );
		//satNotify->addItem(2, fautoScanAll);
		//feModeNotifier->addItem(2, fautoScanAll);

		// intros
		autoScanAlllistBox->addItem(new ClistBoxItem(_("back")));
		autoScanAlllistBox->addItem(new CMenuSeparator(LINE));
		
		// save settings
		autoScanAlllistBox->addItem(new ClistBoxItem(_("Save settings now"), true, NULL, this, "save_scansettings", RC_red, NEUTRINO_ICON_BUTTON_RED));
		autoScanAlllistBox->addItem(new CMenuSeparator(LINE));
		
		// sat
		autoScanAlllistBox->addItem(new ClistBoxItem(_("Satellite"), true, NULL, satOnOffWidget));
			
		// scan
		autoScanAlllistBox->addItem(new ClistBoxItem(_("Start scan"), true, NULL, scanTs, "all") );
		////

		// add item
		fautoScanAll = new ClistBoxItem(_("Auto-Scan multiple Satellites"), ( (dmode != NO_DISEQC) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_NOTCONNECTED) && (CZapit::getInstance()->getFE(feindex)->mode != (fe_mode_t)FE_LOOP)), NULL, autoScanAllWidget );
		satNotify->addItem(2, fautoScanAll);
		feModeNotifier->addItem(2, fautoScanAll);
		scansetup->addItem(fautoScanAll);
	}
	////

	//
	widget->exec(NULL, "");
	
	//
	if (scansetup)
	{
		delete scansetup;
		scansetup = NULL;
	}
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

int CScanSetup::showUnicableSetup()
{
	dprintf(DEBUG_INFO, "CScanSetup::showUnicableSetup\n");
	
	int ret = RETURN_REPAINT;
	
	//
	CWidget* uniWidget = NULL;
	ClistBox* uni_setup = NULL;
	
	uniWidget = CNeutrinoApp::getInstance()->getWidget("unicablesetup");
	
	if (uniWidget)
	{
		uni_setup = (ClistBox*)uniWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		uni_setup = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		uni_setup->setWidgetMode(MODE_SETUP);
		uni_setup->enableShrinkMenu();
		
		uni_setup->enablePaintHead();
		uni_setup->setTitle(_("Unicable settings"), NEUTRINO_ICON_SCAN);

		uni_setup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		uni_setup->setFootButtons(&btn);
		
		//
		uniWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		uniWidget->name = "unicablesetup";
		uniWidget->setMenuPosition(MENU_POSITION_CENTER);
		uniWidget->addWidgetItem(uni_setup);
	}
	
	//uni_setup->clearItems();

	uni_setup->addItem(new ClistBoxItem(_("back")));
	uni_setup->addItem(new CMenuSeparator(LINE));

	// uni_scr
	CMenuOptionNumberChooser * uniscr = new CMenuOptionNumberChooser(_("Unicable SCR address"), &CZapit::getInstance()->getFE(feindex)->uni_scr, true, \
		-1, dmode == DISEQC_UNICABLE ? 7 : 31);
	uni_setup->addItem(uniscr);

	// uni_qrg
	CIntInput *uni_qrg = new CIntInput(_("Unicable frequency"), (int&) CZapit::getInstance()->getFE(feindex)->uni_qrg, 4);
	ClistBoxItem * uniqrg = new ClistBoxItem(_("Unicable frequency"), true, uni_qrg->getValue(), uni_qrg);
	uni_setup->addItem(uniqrg);

	//
	ret = uniWidget->exec(NULL, "");
	
	if (uni_setup)
	{
		delete uni_setup;
		uni_setup = NULL;
	}
	
	if (uniWidget)
	{
		delete uniWidget;
		uniWidget = NULL;
	}

	return ret;
}

// TPSelectHandler
CTPSelectHandler::CTPSelectHandler(int num)
{
	feindex = num;
}

//
int CTPSelectHandler::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	transponder_list_t::iterator tI;
	sat_iterator_t sit;
	t_satellite_position position = 0;
	std::map<int, transponder> tmplist;
	std::map<int, transponder>::iterator tmpI;
	int i;
	int select = -1;
	static int old_selected = 0;
	static t_satellite_position old_position = 0;

	if (parent)
		parent->hide();

	//loop throught satpos
	for(sit = satellitePositions.begin(); sit != satellitePositions.end(); sit++) 
	{
		if(!strcmp(sit->second.name.c_str(), scanSettings->satNameNoDiseqc)) 
		{
			position = sit->first;
			break;
		}
	}

	if(old_position != position) 
	{
		old_selected = 0;
		old_position = position;
	}
	
	//
	CWidget* tpWidget = NULL;
	ClistBox* menu = NULL;
	
	tpWidget = CNeutrinoApp::getInstance()->getWidget("transponder");
	
	if (tpWidget)
	{
		menu = (ClistBox*)tpWidget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		menu = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);

		menu->setWidgetMode(MODE_SETUP);
		
		//
		menu->enablePaintHead();
		menu->setTitle(_("Select transponder"), NEUTRINO_ICON_SCAN);

		//
		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		menu->setFootButtons(&btn);
		
		//
		tpWidget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		tpWidget->name = "transponder";
		tpWidget->setMenuPosition(MENU_POSITION_CENTER);
		tpWidget->addWidgetItem(menu);
	}
	
	//menu->clearItems();
	
	//
	i = 0;

	for(tI = select_transponders.begin(); tI != select_transponders.end(); tI++) 
	{
		t_satellite_position satpos = GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xFFF;
		if(GET_SATELLITEPOSITION_FROM_TRANSPONDER_ID(tI->first) & 0xF000)
			satpos = -satpos;
		
		if(satpos != position)
			continue;

		char buf[128];
		char * f, *s, *m;
		
#if HAVE_DVB_API_VERSION >= 5
		switch( CZapit::getInstance()->getFE(feindex)->getForcedDelSys()) 
		{
			case DVB_S:
			case DVB_S2:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.fec_inner, dvbs_get_modulation(tI->second.feparams.fec_inner),  f, s, m);

				snprintf(buf, sizeof(buf), "%d %c %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.polarization ? 'V' : 'H', tI->second.feparams.symbol_rate/1000, f, s, m);
			}
			break;

			case DVB_C:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.fec_inner, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.feparams.symbol_rate/1000, f, s, m);
			}
			break;

			case DVB_T:
			case DVB_T2:
			case DVB_DTMB:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.code_rate_HP, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency/100000, f, s, m);
			}
			break;
				
			case DVB_A:
            		{
				CZapit::getInstance()->getFE(feindex)->getDelSys(FEC_NONE, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency/100000, f, s, m);
			}
			break;
		}
#else
		switch( CZapit::getInstance()->getFE(feindex)->getInfo()->type) 
		{
			case FE_QPSK:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.fec_inner, dvbs_get_modulation(tI->second.feparams.fec_inner),  f, s, m);

				snprintf(buf, sizeof(buf), "%d %c %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.polarization ? 'V' : 'H', tI->second.feparams.symbol_rate/1000, f, s, m);
			}
			break;

			case FE_QAM:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.fec_inner, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %d %s %s %s ", tI->second.feparams.frequency/1000, tI->second.feparams.symbol_rate/1000, f, s, m);
			}
			break;

			case FE_OFDM:
			{
				CZapit::getInstance()->getFE(feindex)->getDelSys(tI->second.feparams.code_rate_HP, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency/100000, f, s, m);
			}
			break;
				
			case FE_ATSC:
            		{
				CZapit::getInstance()->getFE(feindex)->getDelSys(FEC_NONE, tI->second.feparams.modulation, f, s, m);

				snprintf(buf, sizeof(buf), "%d %s %s %s ", tI->second.feparams.frequency/100000, f, s, m);
			}
			break;
		}
#endif
		
		menu->addItem(new ClistBoxItem(buf), old_selected == i);
		tmplist.insert(std::pair <int, transponder>(i, tI->second));
		i++;
	}
	
	select = menu->getSelected();

	int retval = tpWidget->exec(NULL, "");
	
	if (menu)
	{
		delete menu;
		menu = NULL;
	}
	
	if (tpWidget)
	{
		delete tpWidget;
		tpWidget = NULL;
	}

	if(select >= 0) 
	{
		old_selected = select;

		tmpI = tmplist.find(select);

		sprintf( scanSettings->TP_freq, "%d", tmpI->second.feparams.frequency);
		
#if HAVE_DVB_API_VERSION >= 5
		switch( CZapit::getInstance()->getFE(feindex)->getForcedDelSys()) 
		{
			case DVB_S:
			case DVB_S2:	
				sprintf(scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_pol = tmpI->second.polarization;
				break;

			case DVB_C:	
				sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
				break;

			case DVB_T:
			case DVB_T2:
			case DVB_DTMB:
			{	
				scanSettings->TP_band = tmpI->second.feparams.bandwidth;
				scanSettings->TP_HP = tmpI->second.feparams.code_rate_HP;
				scanSettings->TP_LP = tmpI->second.feparams.code_rate_LP;
				scanSettings->TP_const = tmpI->second.feparams.modulation;
				scanSettings->TP_trans = tmpI->second.feparams.transmission_mode;
				scanSettings->TP_guard = tmpI->second.feparams.guard_interval;
				scanSettings->TP_hierarchy = tmpI->second.feparams.hierarchy_information;
				
				if (CZapit::getInstance()->getFE(feindex)->getForcedDelSys() == DVB_T2)
					sprintf( scanSettings->TP_plp_id, "%d", tmpI->second.feparams.plp_id);
			}
			break;

			case DVB_A:
            		{	
				//sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				//scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
			}
			break;
		}
#else
		switch( CZapit::getInstance()->getFE(feindex)->getInfo()->type) 
		{
			case FE_QPSK:	
				sprintf(scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_pol = tmpI->second.polarization;
				break;

			case FE_QAM:	
				sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
				break;

			case FE_OFDM:
			{	
				scanSettings->TP_band = tmpI->second.feparams.bandwidth;
				scanSettings->TP_HP = tmpI->second.feparams.code_rate_HP;
				scanSettings->TP_LP = tmpI->second.feparams.code_rate_LP;
				scanSettings->TP_const = tmpI->second.feparams.modulation;
				scanSettings->TP_trans = tmpI->second.feparams.transmission_mode;
				scanSettings->TP_guard = tmpI->second.feparams.guard_interval;
				scanSettings->TP_hierarchy = tmpI->second.feparams.hierarchy_information;
			}
			break;

			case FE_ATSC:
            		{	
				//sprintf( scanSettings->TP_rate, "%d", tmpI->second.feparams.symbol_rate);
				//scanSettings->TP_fec = tmpI->second.feparams.fec_inner;
				scanSettings->TP_mod = tmpI->second.feparams.modulation;
			}
			break;
		}
#endif	
	}
	
	if(retval == RETURN_EXIT_ALL)
		return RETURN_EXIT_ALL;

	return RETURN_REPAINT;
}

// scan settings
CScanSettings::CScanSettings( int num)
	: configfile('\t')
{
	feindex = num;
	
	//
	//satNameNoDiseqc[0] = 0;
	strcpy(satNameNoDiseqc, "none");
	bouquetMode = CZapit::BM_UPDATEBOUQUETS;
	scanType = CZapit::ST_TVRADIO;
	
	//
	scan_mode = 1;
}

// borrowed from cst neutrino-hd (femanager.cpp)
uint32_t CScanSettings::getConfigValue(int num, const char * name, uint32_t defval)
{
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_%s", num, name);
	
	return configfile.getInt32(cfg_key, defval);
}

//
void CScanSettings::setConfigValue(int num, const char * name, uint32_t val)
{
	char cfg_key[81];
	
	sprintf(cfg_key, "fe%d_%s", num, name);
	configfile.setInt32(cfg_key, val);
}

bool CScanSettings::loadSettings(const char * const fileName, int index)
{
	printf("CScanSettings::loadSettings: fe%d\n", index);
	
	// if scan.conf not exists load default
	if(!configfile.loadConfig(fileName))
		printf("%s not found\n", fileName);
	
	if( !CZapit::getInstance()->getFE(index) )
		return false;
	
	// common
	scanType = (CZapit::scanType) getConfigValue(index, "scanType", CZapit::ST_ALL);
	bouquetMode = (CZapit::bouquetMode) getConfigValue(index, "bouquetMode", CZapit::BM_UPDATEBOUQUETS);
	scan_mode = getConfigValue(index, "scan_mode", 1); // NIT (0) or fast (1)
	
	//
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_satNameNoDiseqc", index);
	strcpy(satNameNoDiseqc, configfile.getString(cfg_key, "none").c_str());
	
	// freq
	sprintf(cfg_key, "fe%d_TP_freq", index);
	strcpy(TP_freq, configfile.getString(cfg_key, "10100000").c_str());
	
	// rate
	sprintf(cfg_key, "fe%d_TP_rate", index);
	strcpy(TP_rate, configfile.getString(cfg_key, "27500000").c_str());
	
#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_S || CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_S2)
#else
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_QPSK)
#endif
	{
		TP_fec = getConfigValue(index, "TP_fec", 1);
		TP_pol = getConfigValue(index, "TP_pol", 0);
	}
	
#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_C)
#else	
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_QAM)
#endif
	{
		TP_mod = getConfigValue(index, "TP_mod", 3);
		TP_fec = getConfigValue(index, "TP_fec", 1);
	}
	
#if HAVE_DVB_API_VERSION >= 3
	if(TP_fec == 4) 
		TP_fec = 5;
#endif

	//
#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_T2)
#else
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_OFDM)
#endif
	{
		TP_band = getConfigValue(index, "TP_band", 0);
		TP_HP = getConfigValue(index, "TP_HP", 2);
		TP_LP = getConfigValue(index, "TP_LP", 1);
		TP_const = getConfigValue(index, "TP_const", 1);
		TP_trans = getConfigValue(index, "TP_trans", 1);
		TP_guard = getConfigValue(index, "TP_guard", 3);
		TP_hierarchy = getConfigValue(index, "TP_hierarchy", 0);
		sprintf(cfg_key, "fe%d_TP_plp_id", index);
		strcpy(TP_plp_id, configfile.getString(cfg_key, "000").c_str());
	}

#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_A)
#else
    	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_ATSC)
#endif
	{
		TP_mod = getConfigValue(index, "TP_mod", 3);
	}

	return true;
}

bool CScanSettings::saveSettings(const char * const fileName, int index)
{
	printf("CScanSettings::saveSettings: fe%d\n", index);
	
	// common
	setConfigValue(index, "scanType", scanType );
	setConfigValue(index, "bouquetMode", bouquetMode );
	setConfigValue(index, "scan_mode", scan_mode);
	
	//
	char cfg_key[81];
	sprintf(cfg_key, "fe%d_satNameNoDiseqc", index);
	configfile.setString(cfg_key, satNameNoDiseqc );
	
	// freq
	sprintf(cfg_key, "fe%d_TP_freq", index);
	configfile.setString(cfg_key, TP_freq);
	
	// rate
	sprintf(cfg_key, "fe%d_TP_rate", index);
	configfile.setString(cfg_key, TP_rate);
	
#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_S || CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_S2)
#else
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_QPSK)
#endif
	{
		setConfigValue(index, "TP_pol", TP_pol);
		setConfigValue(index, "TP_fec", TP_fec);
	}
	
#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_C)
#else
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_QAM)
#endif
	{
		setConfigValue(index, "TP_mod", TP_mod);
		setConfigValue(index, "TP_fec", TP_fec);
	}

#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_T || CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_T2)
#else
	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_OFDM)
#endif
	{
		setConfigValue(index, "TP_band", TP_band);
		setConfigValue(index, "TP_HP", TP_HP);
		setConfigValue(index, "TP_LP", TP_LP);
		setConfigValue(index, "TP_const", TP_const);
		setConfigValue(index, "TP_trans", TP_trans);
		setConfigValue(index, "TP_guard", TP_guard);
		setConfigValue(index, "TP_hierarchy", TP_hierarchy);
		//setConfigValue(index, "TP_plp_id", TP_plp_id);
		sprintf(cfg_key, "fe%d_TP_plp_id", index);
		configfile.setString(cfg_key, TP_plp_id);
	}

#if HAVE_DVB_API_VERSION >= 5
	if(CZapit::getInstance()->getFE(index)->getForcedDelSys() == DVB_A)
#else
    	if(CZapit::getInstance()->getFE(index)->getInfo()->type == FE_ATSC)
#endif
	{
		setConfigValue(index, "TP_mod", TP_mod);
	}

	if(configfile.getModifiedFlag())
	{
		// save neu configuration
		configfile.saveConfig(fileName);
	}

	return true;
}

CSatelliteSetupNotifier::CSatelliteSetupNotifier(int num)
{
	feindex = num;
}

// item1: comm uncomm
// item2: lnb diseqc input
// item3: auto scan all
// item4: unicable
// item5: diseqc repeats
bool CSatelliteSetupNotifier::changeNotify(const std::string&, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int type = *((int*) Data);

	if (type == NO_DISEQC) 
	{
		for(it = items1.begin(); it != items1.end(); it++)
		{
			(*it)->setActive(false);
		}

		for(it = items2.begin(); it != items2.end(); it++)
		{
			(*it)->setActive(false);
		}

		for(it = items3.begin(); it != items3.end(); it++)
		{
			(*it)->setActive(false);
		}

		for(it = items4.begin(); it != items4.end(); it++)
		{
			(*it)->setActive(false);
		}

		for(it = items5.begin(); it != items5.end(); it++)
		{
			(*it)->setActive(false);
		}
	}
	else if(type < DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			(*it)->setActive(true);
		}
	}
	else if(type == DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			(*it)->setActive(false);
		}
	}
	else if(type > DISEQC_ADVANCED) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			(*it)->setActive(false);
		}
	}

	dmode = type;
	CZapit::getInstance()->getFE(feindex)->setDiseqcType( CZapit::getInstance()->getFE(feindex)->diseqcType );
	CZapit::getInstance()->getFE(feindex)->setDiseqcRepeats( CZapit::getInstance()->getFE(feindex)->diseqcRepeats );

	return true;
}

void CSatelliteSetupNotifier::addItem(int list, CMenuItem* item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		case 3:
			items4.push_back(item);
			break;
		case 4:
			items5.push_back(item);
			break;
		default:
			break;
	}
}

// scansetup notifier
CScanSetupNotifier::CScanSetupNotifier(int num)
{
	feindex = num;
}

// items1 enabled for advanced diseqc settings, items2 for diseqc != NO_DISEQC, items3 disabled for NO_DISEQC
bool CScanSetupNotifier::changeNotify(const std::string&, void * Data)
{
	std::vector<CMenuItem*>::iterator it;
	int FeMode = *((int*) Data);
	
	dprintf(DEBUG_NORMAL, "CScanSetupNotifier::changeNotify: Femode:%d\n", FeMode);

	if ( (FeMode == FE_NOTCONNECTED) || (FeMode == FE_LOOP) ) 
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			(*it)->setActive(false);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			(*it)->setActive(false);
		}
	}
	else
	{
		for(it = items1.begin(); it != items1.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items2.begin(); it != items2.end(); it++) 
		{
			(*it)->setActive(true);
		}

		for(it = items3.begin(); it != items3.end(); it++) 
		{
			if (dmode != NO_DISEQC)
				(*it)->setActive(true);
		}

		for(it = items4.begin(); it != items4.end(); it++) 
		{
			if (dmode > DISEQC_ADVANCED)
				(*it)->setActive(true);
		}

		for(it = items5.begin(); it != items5.end(); it++) 
		{
			if (dmode != NO_DISEQC && dmode < DISEQC_ADVANCED)
				(*it)->setActive(true);
		}
	}

	return true;
}

void CScanSetupNotifier::addItem(int list, CMenuItem *item)
{
	switch(list) 
	{
		case 0:
			items1.push_back(item);
			break;	
		case 1:
			items2.push_back(item);
			break;
		case 2:
			items3.push_back(item);
			break;
		case 3:
			items4.push_back(item);
			break;
		case 4:
			items5.push_back(item);
			break;
		default:
			break;
	}
}

//
CTunerSetup::CTunerSetup()
{
}

CTunerSetup::~CTunerSetup()
{
}

int CTunerSetup::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CTunerSetup::exec: actionKey:%s\n", actionKey.c_str());
	
	int ret = RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return ret;
}

void CTunerSetup::showMenu()
{
	dprintf(DEBUG_NORMAL, "CTunerSetup::showMenu\n");
	
	CWidget* widget = NULL;
	ClistBox* TunerSetup = NULL;
	
	widget = CNeutrinoApp::getInstance()->getWidget("tunersetup");
	
	if (widget)
	{
		TunerSetup = (ClistBox*)widget->getWidgetItem(WIDGETITEM_LISTBOX);
	}
	else
	{
		TunerSetup = new ClistBox(0, 0, MENU_WIDTH, MENU_HEIGHT);
		
		TunerSetup->setWidgetMode(MODE_MENU);
		//TunerSetup->setWidgetType(TYPE_CLASSIC);
		TunerSetup->enableShrinkMenu();
		
		//
		TunerSetup->enablePaintHead();
		TunerSetup->setTitle(_("Servicescan"), NEUTRINO_ICON_SCAN);
		TunerSetup->enablePaintDate();
		
		//
		TunerSetup->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		TunerSetup->setFootButtons(&btn);
		
		//
		widget = new CWidget(TunerSetup->getWindowsPos().iX, TunerSetup->getWindowsPos().iY, TunerSetup->getWindowsPos().iWidth, TunerSetup->getWindowsPos().iHeight);
		widget->name = "tunersetup";
		widget->setMenuPosition(MENU_POSITION_CENTER);
		
		widget->addWidgetItem(TunerSetup);
	}
	
	TunerSetup->clearItems();
		
	// intros
	TunerSetup->addItem(new ClistBoxItem(_("back")));
	TunerSetup->addItem( new CMenuSeparator(LINE) );
				
	for(int i = 0; i < FrontendCount; i++)
	{
		CFrontend * fe = CZapit::getInstance()->getFE(i);
		char tbuf[255];
				
		sprintf(tbuf, "Tuner-%d: %s", i + 1, fe->getInfo()->name);
		TunerSetup->addItem(new ClistBoxItem(tbuf, true, NULL, new CScanSetup(i) ));
	}
	
	widget->exec(NULL, "");
}

bool CScanSetupDelSysNotifier::changeNotify(const std::string&, void *Data)
{
	CZapit::getInstance()->getFE(feindex)->changeDelSys(CZapit::getInstance()->getFE(feindex)->forcedDelSys);
	CZapit::getInstance()->getFE(feindex)->reset();
	
	return true;
}

