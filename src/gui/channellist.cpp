/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: channellist.cpp 16.11.2020 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

#include <gui/channellist.h>

#include <global.h>
#include <neutrino2.h>

#include <driver/gfx/fontrenderer.h>
#include <driver/gfx/color.h>
#include <driver/gfx/icons.h>

#include <driver/rcinput.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/epgplus.h>

#include <gui/widget/widget_helpers.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/infobox.h>

#include <system/settings.h>
#include <system/lastchannel.h>
#include <system/debug.h>
#include <system/channellogo.h>
#include <system/tmdbparser.h>

#include <gui/filebrowser.h>

#include <gui/bouquetlist.h>
#include <daemonc/remotecontrol.h>
#include <driver/vcrcontrol.h>

//
#include <zapit/bouquets.h>
#include <zapit/frontend_c.h>

#include <video_cs.h>
#include <gui/misc_setup.h>

#include <timerd/timerd.h>


//// globals
bool pip_selected = false;
//
extern bool autoshift;
extern int old_b_id;					// defined in neutrino2.cpp
//
extern CRemoteControl * g_RemoteControl; 		// defined in neutrino2.cpp	
//
extern CBouquetList * bouquetList;      		// defined in neutrino2.cpp
extern CBouquetList   * TVbouquetList;
extern CBouquetList   * TVsatList;
extern CBouquetList   * TVfavList;
extern CBouquetList   * TVallList;
extern CBouquetList   * RADIObouquetList;
extern CBouquetList   * RADIOsatList;
extern CBouquetList   * RADIOfavList;
extern CBouquetList   * RADIOallList;

#define CHANNEL_SMSKEY_TIMEOUT 800

#define NUM_LIST_BUTTONS 4
struct button_label CChannelListButtons[NUM_LIST_BUTTONS] =
{
	{ NEUTRINO_ICON_BUTTON_RED, _("Record"), COL_RED_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_GREEN, _("TMDB"), COL_GREEN_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_YELLOW, _("Schedule"), COL_YELLOW_PLUS_0 },
	{ NEUTRINO_ICON_BUTTON_BLUE, _("Bouquets"), COL_BLUE_PLUS_0 }
};

#define HEAD_BUTTONS_COUNT	3
const struct button_label HeadButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_EPG, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " }
};

const struct button_label HeadNewModeButtons[HEAD_BUTTONS_COUNT] =
{
	{ NEUTRINO_ICON_BUTTON_HELP, " " },
	{ NEUTRINO_ICON_BUTTON_EPG, " " },
	{ NEUTRINO_ICON_BUTTON_SETUP, " " }
};

//// class CZapProtection
bool CZapProtection::check()
{
	int res;
	char cPIN[5];
	std::string hint2 = " ";
	
	do
	{
		cPIN[0] = 0;

		CPLPINInput* PINInput = new CPLPINInput(_("Youth protection"), cPIN, 4, hint2.c_str(), fsk);

		res = PINInput->exec(NULL, "");
		delete PINInput;

		hint2 = _("PIN-Code was wrong! Try again.");
	} while ( (strncmp(cPIN,validPIN, 4) != 0) &&
		  (cPIN[0] != 0) &&
		  ( res == CMenuTarget::RETURN_REPAINT ) &&
		  ( fsk >= g_settings.parentallock_lockage ) );
		  
	return ( ( strncmp(cPIN, validPIN, 4) == 0 ) || ( fsk < g_settings.parentallock_lockage ) );
}

////
CChannelList::CChannelList(const char * const Name, bool _historyMode, bool _vlist)
{
	frameBuffer = CFrameBuffer::getInstance();

	name = Name;
	selected = 0;
	tuned = 0xfffffff;
	zapProtection = NULL;
	this->historyMode = _historyMode;
	vlist = _vlist;
	
	events.clear();

	//
	widget = NULL;
	
	listBox = NULL;
	item = NULL;
	
	window = NULL;
	
	head = NULL;
	foot = NULL;
	
	vline = NULL;
	hline = NULL;
	
	sec_timer_id = 0;

	// widget screen	
	cFrameBox.iWidth = frameBuffer->getScreenWidth() - 20;
	cFrameBox.iHeight = frameBuffer->getScreenHeight() - 20;
	
	cFrameBox.iX = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - cFrameBox.iWidth) / 2;
	cFrameBox.iY = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - cFrameBox.iHeight) / 2;
}

CChannelList::~CChannelList()
{
	chanlist.clear();
	events.clear();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

void CChannelList::ClearList(void)
{
	chanlist.clear();
	chanlist.resize(1);
}

void CChannelList::setSize(int newsize)
{
	chanlist.reserve(newsize);
}

void CChannelList::addChannel(CZapitChannel * channel)
{
	chanlist.push_back(channel);
}

void CChannelList::updateEvents(void)
{
	dprintf(DEBUG_NORMAL, "CChannelList::updateEvents\n");
	
	events.clear();

	//
	t_channel_id *p_requested_channels = NULL;
	int size_requested_channels = 0;

	if (chanlist.size()) 
	{
		size_requested_channels = chanlist.size()*sizeof(t_channel_id);
		p_requested_channels = (t_channel_id*)malloc(size_requested_channels);
			
		for (uint32_t count = 0; count < chanlist.size(); count++)
		{
			p_requested_channels[count] = chanlist[count]->channel_id & 0xFFFFFFFFFFFFULL;
		}

		CChannelEventList pevents;

		pevents.clear();
			
		CSectionsd::getInstance()->getChannelEvents(pevents, (CNeutrinoApp::getInstance()->getMode()) != NeutrinoMessages::mode_radio, p_requested_channels, size_requested_channels);
			
		for (uint32_t count = 0; count < chanlist.size(); count++) 
		{
			for ( CChannelEventList::iterator e = pevents.begin(); e != pevents.end(); ++e )
			{
				if ((chanlist[count]->channel_id & 0xFFFFFFFFFFFFULL) == (e->channelID & 0xFFFFFFFFFFFFULL))//FIXME: get_channel_id()
				{
					chanlist[count]->currentEvent = *e;

					break;
				}
			}
		}

		if (p_requested_channels != NULL) 
			free(p_requested_channels);
	}
}

struct CmpChannelBySat: public std::binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
        static bool comparetolower(const char a, const char b)
        {
		    return tolower(a) < tolower(b);
        };

        bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
        {
		    if(c1->getSatellitePosition() == c2->getSatellitePosition())
			    return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
		    else
			    return c1->getSatellitePosition() < c2->getSatellitePosition();
;
	};
};

struct CmpChannelByFreq: public std::binary_function <const CZapitChannel * const, const CZapitChannel * const, bool>
{
        static bool comparetolower(const char a, const char b)
        {
                return tolower(a) < tolower(b);
        };

        bool operator() (const CZapitChannel * const c1, const CZapitChannel * const c2)
        {
		    if(c1->getFreqId() == c2->getFreqId())
			    return std::lexicographical_compare(c1->getName().begin(), c1->getName().end(), c2->getName().begin(), c2->getName().end(), comparetolower);
		    else
			    return c1->getFreqId() < c2->getFreqId();
;
	};
};

void CChannelList::SortAlpha(void)
{
	sort(chanlist.begin(), chanlist.end(), CmpChannelByChName()); //FIXME:
}

void CChannelList::SortSat(void)
{
	sort(chanlist.begin(), chanlist.end(), CmpChannelBySat());
}

//
CZapitChannel * CChannelList::getChannel(int number)
{
	for (uint32_t i = 0; i< chanlist.size();i++) 
	{
		if (chanlist[i]->number == number)
			return chanlist[i];
	}
	
	return(NULL);
}

//
CZapitChannel * CChannelList::getChannel(t_channel_id channel_id)
{
	for (uint32_t i = 0; i< chanlist.size();i++) 
	{
		if (chanlist[i]->channel_id == channel_id)
			return chanlist[i];
	}
	
	return(NULL);
}

int CChannelList::getKey(int id)
{
	return chanlist[id]->number;
}

static const std::string empty_string;

const std::string & CChannelList::getActiveChannelName(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->name;
	else
		return empty_string;
}

t_satellite_position CChannelList::getActiveSatellitePosition(void) const
{
	if (selected < chanlist.size())
		return chanlist[selected]->getSatellitePosition();
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_ChannelID(void) const
{
	if (selected < chanlist.size()) 
	{
		dprintf(DEBUG_NORMAL, "CChannelList::getActiveChannel_ChannelID: 0x%llx\n", chanlist[selected]->channel_id);
		
		return chanlist[selected]->channel_id;
	} 
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_EPGID(void) const
{
	if (selected < chanlist.size()) 
	{
		printf("CChannelList::getActiveChannel_EPGID: 0x%llx\n", chanlist[selected]->epgid);
		
		return chanlist[selected]->epgid;
	} 
	else
		return 0;
}

t_channel_id CChannelList::getActiveChannel_LogoID(void) const
{
	if (selected < chanlist.size()) 
	{
		printf("CChannelList::getActiveChannel_LogoID: 0x%llx\n", chanlist[selected]->logoid);
		
		return chanlist[selected]->logoid;
	} 
	else
		return 0;
}

int CChannelList::getActiveChannelNumber(void) const
{
	return (selected + 1);
}

int CChannelList::getSize() const
{
	return this->chanlist.size();
}

int CChannelList::getSelectedChannelIndex() const
{
	return this->selected;
}

// >= 0 to zap
// -1 on cancel
// -3 on list mode change
// -4 list edited
// -2 zap but no restore old list/chan
int CChannelList::exec(bool customMode)
{
	dprintf(DEBUG_NORMAL, "CChannelList::exec: zap:%s\n", customMode? "no" : "yes");
	
	// show list
	int nNewChannel = show(customMode);

	dprintf(DEBUG_NORMAL, "CChannelList::exec: chanlist.size:%d nNewChannel:%d\n", (int)chanlist.size(), nNewChannel);
	
	// zapto
	if(!customMode)
	{
		if ( nNewChannel > -1 && nNewChannel < (int) chanlist.size()) 
			CNeutrinoApp::getInstance()->channelList->zapTo(getKey(nNewChannel) - 1);
	}

	return nNewChannel;
}

// >= 0 to zap
// -1 on cancel
// -3 on list mode change
// -4 list edited
// -2 zap but no restore old list/chan
int CChannelList::show(bool customMode)
{
	dprintf(DEBUG_NORMAL, "CChannelList::show: zap:%s\n", customMode? "no" : "yes");

	neutrino_msg_t      msg;
	neutrino_msg_data_t data;
	int res = -1;

	// display channame in vfd	
	CVFD::getInstance()->setMode(CVFD::MODE_MENU_UTF8);	
	
	// update events
	updateEvents();
	
	// 
	paint(customMode);
	CFrameBuffer::getInstance()->blit();

	int oldselected = selected;
	int zapOnExit = false;
	bool bShowBouquetList = false;

	// add sec timer
	sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	// loop control
	uint64_t timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

	bool loop = true;
	while (loop) 
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		
		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_channellist);

		if ( ( msg == CRCInput::RC_timeout ) || ( msg == (neutrino_msg_t)g_settings.key_channelList_cancel) ) 
		{
			selected = oldselected;
			
			loop = false;
			res = -1;
		}
		else if (msg == CRCInput::RC_epg) // epg
		{
			selected = listBox? listBox->getSelected() : 0;

			hide();

			if ( chanlist.size() && g_EventList->exec(chanlist[selected]->epgid, chanlist[selected]->name) == CMenuTarget::RETURN_EXIT_ALL) 
			{
				res = -2;
				loop = false;
			}
			
			paint();
		}
		else if (msg == CRCInput::RC_red) // record
		{
			selected = listBox? listBox->getSelected() : 0;

			hide();
			
			if (chanlist.size() && CTimerd::getInstance()->isTimerdAvailable()) 
			{
				CTimerd::getInstance()->addRecordTimerEvent(chanlist[selected]->channel_id, chanlist[selected]->currentEvent.startTime, chanlist[selected]->currentEvent.startTime + chanlist[selected]->currentEvent.duration, chanlist[selected]->currentEvent.eventID, chanlist[selected]->currentEvent.startTime, chanlist[selected]->currentEvent.startTime - (ANNOUNCETIME + 120) , TIMERD_APIDS_CONF, true);

				MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);	// UTF-8
			} 
			
			paint();
		}
		else if ( msg == CRCInput::RC_blue && ( bouquetList != NULL ) )
		{
			//FIXME: show bqt list
			bShowBouquetList = true;

			loop = false;
		}
		else if( msg == CRCInput::RC_yellow )
		{
			selected = listBox? listBox->getSelected() : 0;

			hide();
			
			if (chanlist.size() && CTimerd::getInstance()->isTimerdAvailable ()) 
			{
				CTimerd::getInstance()->addZaptoTimerEvent (chanlist[selected]->channel_id, chanlist[selected]->currentEvent.startTime, chanlist[selected]->currentEvent.startTime - ANNOUNCETIME, 0, chanlist[selected]->currentEvent.eventID, chanlist[selected]->currentEvent.startTime, 0);
		
				MessageBox(_("Schedule Event"), _("The event is scheduled.\nThe box will power on and \nswitch to this channel at the given time."), CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO);	// UTF-8
			} 

			paint();
		}
		else if ( msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
		{
			g_RCInput->postMsg(msg, 0);
			loop = false;
			res = -1;
		}
		else if ( msg == CRCInput::RC_setup ) 
		{
			selected = listBox? listBox->getSelected() : 0;
			
			if (chanlist.size() && !IS_WEBTV(chanlist[selected]->channel_id))
			{
				// channellist setup (add/move)
				old_b_id = bouquetList->getActiveBouquetNumber();
				int ret = doChannelMenu();
		
				if(ret) 
				{
					res = -4;
					loop = false;
				} 
				else 
				{
					old_b_id = -1;

					paint();
				}
			}
		}
		else if (msg == (neutrino_msg_t) g_settings.key_list_start) 
		{
			selected = 0;

			paint();
		}
		else if (msg == (neutrino_msg_t) g_settings.key_list_end) 
		{
			selected = chanlist.size() - 1;

			paint();
		}
                else if (msg == CRCInput::RC_up)
                {
			if (listBox) listBox->scrollLineUp();
			
			selected = listBox? listBox->getSelected() : 0;
				
			//
			paintCurrentNextEvent(selected);
                }
		else if ( msg == CRCInput::RC_page_up || (int) msg == g_settings.key_channelList_pageup)
                {
			if (listBox) listBox->scrollPageUp();
			
			selected = listBox? listBox->getSelected() : 0;
				
			//
			paintCurrentNextEvent(selected);
                }
                else if (msg == CRCInput::RC_down)
                {
			if (listBox) listBox->scrollLineDown();
			
			selected = listBox? listBox->getSelected() : 0;
				
			//
			paintCurrentNextEvent(selected);
                }
		else if (msg == CRCInput::RC_page_down || (int) msg == g_settings.key_channelList_pagedown)
                {
			if (listBox) listBox->scrollPageDown();
			
			selected = listBox? listBox->getSelected() : 0;
				
			//
			paintCurrentNextEvent(selected);
                }
		else if ((msg == (neutrino_msg_t)g_settings.key_bouquet_up) && (bouquetList != NULL)) 
		{
			if (bouquetList->Bouquets.size() > 0) 
			{
				bool found = true;
				uint32_t nNext = (bouquetList->getActiveBouquetNumber() + 1) % bouquetList->Bouquets.size();
				
				if(bouquetList->Bouquets[nNext]->channelList->getSize() <= 0) 
				{
					found = false;
					nNext = nNext < bouquetList->Bouquets.size() - 1 ? nNext + 1 : 0;
					
					for(uint32_t i = nNext; i < bouquetList->Bouquets.size(); i++) 
					{
						if(bouquetList->Bouquets[i]->channelList->getSize() > 0) 
						{
							found = true;
							nNext = i;
							break;
						}
					}
				}
				
				if(found) 
				{
					bouquetList->activateBouquet(nNext);
					res = bouquetList->showChannelList(nNext, customMode);
					loop = false;
				}
			}
		}
		else if ((msg == (neutrino_msg_t)g_settings.key_bouquet_down) && (bouquetList != NULL)) 
		{
			if (bouquetList->Bouquets.size() > 0) 
			{
				bool found = true;
				int nNext = (bouquetList->getActiveBouquetNumber() + bouquetList->Bouquets.size() - 1) % bouquetList->Bouquets.size();
				
				if(bouquetList->Bouquets[nNext]->channelList->getSize() <= 0) 
				{
					found = false;
					nNext = nNext > 0 ? nNext - 1 : bouquetList->Bouquets.size() - 1;
					
					for(int i = nNext; i > 0; i--) 
					{
						if(bouquetList->Bouquets[i]->channelList->getSize() > 0) 
						{
							found = true;
							nNext = i;
							break;
						}
					}
				}
				
				if(found) 
				{
					bouquetList->activateBouquet(nNext);
					res = bouquetList->showChannelList(nNext, customMode);
					loop = false;
				}
			}
		}
		else if ( msg == CRCInput::RC_ok ) 
		{
			selected = listBox? listBox->getSelected() : 0;
	  
			zapOnExit = !customMode;
			
			loop = false;
		}
		else if (CRCInput::isNumeric(msg) && this->historyMode) 
		{ 
			if (this->historyMode) 
			{ 
				selected = CRCInput::getNumericValue(msg);
				zapOnExit = !customMode;
				loop = false;
    			}
		}
		else if(CRCInput::isNumeric(msg)) 
		{
			//pushback key if...
			selected = oldselected;
			g_RCInput->postMsg( msg, data );
			loop = false;
		}
		else if ( msg == CRCInput::RC_green ) //next
		{
			selected = listBox? listBox->getSelected() : 0;
			
			hide();
			
			//
			if (chanlist.size())
				::getTMDBInfo(chanlist[selected]->currentEvent.description.c_str());

			paint();
		} 
		else if ( (msg == CRCInput::RC_info) )
		{
			selected = listBox? listBox->getSelected() : 0;

			hide();
			
			if (chanlist.size())
				g_EpgData->show(chanlist[selected]->epgid); 

			paint();
		}
		else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
		{
			if (head) head->refresh();
		} 
		else 
		{
			if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
			{
				loop = false;
				res = - 2;
			}
		}
			
		frameBuffer->blit();	
	}
	
	hide();
	
	// bouquets mode
	if (bShowBouquetList)
	{
		res = bouquetList->exec(true, customMode);
	}
	
	CVFD::getInstance()->setMode(CVFD::MODE_TVRADIO);

	//
	g_RCInput->killTimer(sec_timer_id);
	sec_timer_id = 0;

	if(NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode())
		return -1;

	if(zapOnExit)
		res = selected;
		
	if (customMode)
		res = selected;

	dprintf(DEBUG_NORMAL, "CChannelList::show res %d\n", res);
			
	return(res);
}

void CChannelList::hide()
{
	dprintf(DEBUG_NORMAL, "CChannelList::hide\n");

	
	if (widget) 
		widget->hide();
	else
		CFrameBuffer::getInstance()->clearFrameBuffer();
		
	frameBuffer->blit();
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
}

bool CChannelList::showInfo(int pos, int epgpos, bool fromNumZap)
{
	dprintf(DEBUG_NORMAL, "CChannelList::showInfo:\n");
	
	if((pos >= (signed int) chanlist.size()) || (pos < 0))
		return false;

	CZapitChannel * chan = chanlist[pos];
	
	// channel infobar
	g_InfoViewer->showTitle(pos + 1, chan->name, chan->getSatellitePosition(), chan->channel_id, fromNumZap, epgpos); // UTF-8
	
	return true;
}

int CChannelList::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ( msg == NeutrinoMessages::EVT_PROGRAMLOCKSTATUS) 
	{
		// 0x100 als FSK-Status zeigt an, dass (noch) kein EPG zu einem Kanal der NICHT angezeigt
		// werden sollte (vorgesperrt) da ist
		// oder das bouquet des Kanals ist vorgesperrt

		if ((g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_ONSIGNAL) || (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED))
		{
			if ( zapProtection != NULL )
				zapProtection->fsk = data;
			else 
			{
				// require password if either
				// CHANGETOLOCK mode and channel/bouquet is pre locked (0x100)
				// ONSIGNAL mode and fsk(data) is beyond configured value
				// if programm has already been unlocked, dont require pin
				if ((data >= (neutrino_msg_data_t)g_settings.parentallock_lockage) &&
					 ((chanlist[selected]->last_unlocked_EPGid != g_RemoteControl->current_EPGid) || (g_RemoteControl->current_EPGid == 0)) &&
					 ((g_settings.parentallock_prompt != PARENTALLOCK_PROMPT_CHANGETOLOCKED) || (data >= 0x100)))
				{
					g_RemoteControl->stopvideo();
					
					zapProtection = new CZapProtection( g_settings.parentallock_pincode, data );
					
					if ( zapProtection->check() )
					{
						g_RemoteControl->startvideo(chanlist[selected]->channel_id);
						
						// remember it for the next time
						chanlist[selected]->last_unlocked_EPGid = g_RemoteControl->current_EPGid;
					}
					delete zapProtection;
					zapProtection = NULL;
				}
				else
					g_RemoteControl->startvideo(chanlist[selected]->channel_id);
			}
		}
		else
			g_RemoteControl->startvideo(chanlist[selected]->channel_id);

		return messages_return::handled;
	}
	else
		return messages_return::unhandled;
}

// bToo default to true
bool CChannelList::adjustToChannelID(const t_channel_id channel_id, bool bToo)
{
	unsigned int i;

	dprintf(DEBUG_NORMAL, "CChannelList::adjustToChannelID: channel_id 0x%llx\n", channel_id);
	
	for (i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i] != NULL)
		{
			if (chanlist[i]->channel_id == channel_id) 
			{
				selected = i;
				lastChList.store(selected, channel_id, false);

				tuned = i;
				
				if (bToo && (bouquetList != NULL)) 
				{
					//FIXME
					if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_tv) 
					{
						TVbouquetList->adjustToChannelID(channel_id);
						TVsatList->adjustToChannelID(channel_id);
						TVfavList->adjustToChannelID(channel_id);
						TVallList->adjustToChannelID(channel_id);
					} 
					else if(CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio) 
					{
						RADIObouquetList->adjustToChannelID(channel_id);
						RADIOsatList->adjustToChannelID(channel_id);
						RADIOfavList->adjustToChannelID(channel_id);
						RADIOallList->adjustToChannelID(channel_id);
					}
				}
				
				return true;
			}
		}
	}

	return false;
}

int CChannelList::hasChannel(int nChannelNr)
{
	for (uint32_t i = 0; i < chanlist.size(); i++) 
	{
		if (getKey(i) == nChannelNr)
			return(i);
	}
	
	return(-1);
}

int CChannelList::hasChannelID(t_channel_id channel_id)
{
	for (uint32_t i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i]->channel_id == channel_id)
			return i;
	}
	
	return -1;
}

// for adjusting bouquet's channel list after numzap or quickzap
void CChannelList::setSelected( int nChannelNr)
{
	selected = nChannelNr;
}

// -- Zap to channel with channel_id
bool CChannelList::zapToChannelID(const t_channel_id channel_id, bool rezap)
{
	dprintf(DEBUG_NORMAL, "CChannelList::zapToChannelID 0x%llx\n", channel_id);
	
	for (unsigned int i = 0; i < chanlist.size(); i++) 
	{
		if (chanlist[i]->channel_id == channel_id) 
		{
			zapTo(i, rezap);
			return true;
		}
	}
	
	return false;
}

// 
void CChannelList::zapTo(int pos, bool rezap)
{
	dprintf(DEBUG_NORMAL, "CChannelList::zapTo: %d\n", pos);
	
	// show emty channellist error msg
	if (chanlist.empty()) 
	{
		if (CZapit::getInstance()->getFrontendCount() >= 1) 
			MessageBox(_("Error"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> System)"), CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);
			
		return;
	}

	if ( (pos >= (signed int) chanlist.size()) || (pos < 0) ) 
	{
		pos = 0;
	}

	CZapitChannel * chan = chanlist[pos];
	
	dprintf(DEBUG_NORMAL, "CChannelList::zapTo me %s tuned %d new %d %s id: 0x%llx\n", name.c_str(), tuned, pos, chan->name.c_str(), chan->channel_id);
	
	if ( (pos != (int)tuned) || rezap ) //FIXME: allow after scan to tun
	{  
		if ((g_settings.radiotext_enable) && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio) && (g_Radiotext))
		{
			// stop radiotext PES decoding before zapping
			g_Radiotext->radiotext_stop();
		}		
		
		tuned = pos;
		g_RemoteControl->zapToChannelID(chan->channel_id, chan->name, !chan->bAlwaysLocked); // UTF-8
		
		// TODO check is it possible bouquetList is NULL ?
		if (bouquetList != NULL) 
		{
			CNeutrinoApp::getInstance()->channelList->adjustToChannelID(chan->channel_id);
		}
	}

	//
	selected = pos;

	// remove recordModeActive from infobar
	if(g_settings.auto_timeshift && !CNeutrinoApp::getInstance()->recordingstatus) 
	{
		g_InfoViewer->handleMsg(NeutrinoMessages::EVT_RECORDMODE, 0);
	}
}

// -1: channellist not found
int CChannelList::numericZap(int key)
{
	dprintf(DEBUG_NORMAL, "CChannelList::numericZap\n");
	
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	if (chanlist.empty()) 
	{
		if (CZapit::getInstance()->getFrontendCount() >= 1) 
			MessageBox(_("Error"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);
			
		return res;
	}

	// lastchannel key
	if (key == g_settings.key_lastchannel) 
	{
		t_channel_id channel_id = lastChList.getlast(1);
		if(channel_id) 
		{
			lastChList.clear_storedelay(); // ignore store delay
			zapToChannelID(channel_id);
		}
		
		return res;
	}

	// zap history key
	if (key == g_settings.key_zaphistory) 
	{
		// current transponder bouquet
		if(!autoshift && CNeutrinoApp::getInstance()->recordingstatus && !IS_WEBTV(CZapit::getInstance()->getRecordChannelID())) 
		{
			CChannelList * orgList = bouquetList->orgChannelList;
			CChannelList * channelList = new CChannelList(_("Current transponder"), false, true);
			
			t_channel_id recid = CZapit::getInstance()->getRecordChannelID() >> 16;
			
			// get channels from the same tp as recording channel
			for ( unsigned int i = 0 ; i < orgList->chanlist.size(); i++) 
			{
				if((orgList->chanlist[i]->channel_id >> 16) == recid) 
				{
					channelList->addChannel(orgList->chanlist[i]);
				}
			}

			if (channelList->getSize() != 0) 
			{
				channelList->adjustToChannelID(orgList->getActiveChannel_ChannelID(), false);
				
				this->frameBuffer->paintBackground();
				
				this->frameBuffer->blit();

				res = channelList->exec();
			}
			
			delete channelList;
			return res;
		}
		
		// -- zap history bouquet
		if (this->lastChList.size() > 1) 
		{
			CChannelList * channelList = new CChannelList(_("History"), true, true);

			for(unsigned int i = 1 ; i < this->lastChList.size() ; ++i) 
			{
				t_channel_id channel_id = this->lastChList.getlast(i);

				if(channel_id) 
				{
					CZapitChannel * channel = getChannel(channel_id);
					if(channel) 
						channelList->addChannel(channel);
				}
			}

			if (channelList->getSize() != 0) 
			{
				this->frameBuffer->paintBackground();

				this->frameBuffer->blit();

				res = channelList->exec();
			}
			delete channelList;
		}
		
		return res;
	}
	
	// pip key
	if(key == g_settings.key_pip )
	{
		CChannelList * orgList = bouquetList->orgChannelList;
		CChannelList * channelList = new CChannelList(_("Current transponder"), false, true);
			
		t_channel_id pipid = CZapit::getInstance()->getCurrentChannelID() >> 16;
			
		for ( unsigned int i = 0 ; i < orgList->chanlist.size(); i++) 
		{
			if((orgList->chanlist[i]->channel_id >> 16) == pipid) 
			{
				channelList->addChannel(orgList->chanlist[i]);
			}
		}
			
		pip_selected = true;

		if (channelList->getSize() != 0) 
		{
			channelList->adjustToChannelID(orgList->getActiveChannel_ChannelID(), false);
			this->frameBuffer->paintBackground();

			this->frameBuffer->blit();

			res = channelList->exec();
		}
		delete channelList;
		return res;
	}

	// sms numeric zap
	int sx = 4 * g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getRenderWidth(widest_number) + 14;
	int sy = g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->getHeight() + 6;

	int ox = frameBuffer->getScreenX() + (frameBuffer->getScreenWidth() - sx)/2;
	int oy = frameBuffer->getScreenY() + (frameBuffer->getScreenHeight() - sy)/2;
	char valstr[10];
	int chn = CRCInput::getNumericValue(key);
	int pos = 1;
	int lastchan = -1;
	bool doZap = true;
	bool showEPG = false;

	while(1) 
	{
		if (lastchan != chn) 
		{
			sprintf((char*) &valstr, "%d", chn);
			
			while(strlen(valstr) < 4)
				strcat(valstr,"-");   //"_"

			frameBuffer->paintBoxRel(ox, oy, sx, sy, COL_INFOBAR_PLUS_0);

			for (int i = 3; i >= 0; i--) 
			{
				valstr[i + 1] = 0;
				g_Font[SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP]->RenderString(ox + 7 + i*((sx - 14)>>2), oy + sy - 3, sx, &valstr[i], COL_INFOBAR_TEXT_PLUS_0);
			}
			
//			frameBuffer->blit();

			// show infobar
			showInfo(chn - 1);
			
			lastchan = chn;
		}

		//
		g_RCInput->getMsg(&msg, &data, g_settings.timing_numericzap);

		if ( msg == CRCInput::RC_timeout ) 
		{
			if ( ( chn > (int)chanlist.size() ) || (chn == 0) )
				chn = tuned + 1;
			break;
		}
		else if (CRCInput::isNumeric(msg)) 
		{
			if (pos == 4) 
			{
				chn = 0;
				pos = 1;
			} 
			else 
			{
				chn *= 10;
				pos++;
			}
			chn += CRCInput::getNumericValue(msg);
		}
		else if ( msg == CRCInput::RC_ok ) 
		{
			if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) ) 
			{
				chn = tuned + 1;
			}
			break;
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_quickzap_down ) 
		{
			if ( chn == 1 )
				chn = chanlist.size();
			else {
				chn--;

				if (chn > (int)chanlist.size())
					chn = (int)chanlist.size();
			}
		}
		else if ( msg == (neutrino_msg_t)g_settings.key_quickzap_up ) 
		{
			chn++;

			if (chn > (int)chanlist.size())
				chn = 1;
		}
		else if ( ( msg == CRCInput::RC_home ) || ( msg == CRCInput::RC_left ) || ( msg == CRCInput::RC_right) )
		{
			doZap = false;
			break;
		}
		else if ( msg == CRCInput::RC_red ) 
		{
			if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) ) 
			{
				doZap = false;
				showEPG = true;
				break;
			}
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all ) 
		{
			doZap = false;
			break;
		}
		
		frameBuffer->blit();
	}

	// hide
	frameBuffer->paintBackgroundBoxRel(ox, oy, sx, sy);
	frameBuffer->blit();

	chn--;
	
	if (chn < 0)
		chn = 0;
	
	if ( doZap ) 
	{
		// zapto selected channel
		zapTo( chn );
		showInfo(chn - 1, 0, false);
	} 
	else 
	{
		// show last tuned channel infobar
		showInfo(tuned, 0, false);

		if ( showEPG )
			g_EventList->exec(chanlist[chn]->epgid, chanlist[chn]->name);
	}
	
	return res;
}

void CChannelList::virtual_zap_mode(bool up)
{
	dprintf(DEBUG_NORMAL, "CChannelList::virtual_zap_mode\n");

        neutrino_msg_t      msg;
        neutrino_msg_data_t data;

        if (chanlist.empty()) 
	{
		if (CZapit::getInstance()->getFrontendCount() >= 1) 
			MessageBox(_("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), _("No channels were found!\nPlease execute a scan\n(MENU-key -> service)"), CMessageBox::mbrCancel, CMessageBox::mbCancel, NEUTRINO_ICON_ERROR);
			
                return;
        }

        int chn = getActiveChannelNumber() + (up ? 1 : -1);
        if (chn > (int)chanlist.size())
		chn = 1;
	
        if (chn == 0)
		chn = (int)chanlist.size();
	
        int lastchan = -1;
        bool doZap = true;
        bool showEPG = false;
	int epgpos = 0;

        while(1)
        {
                if (lastchan != chn || (epgpos != 0))
                {
                	showInfo(chn - 1, epgpos);
                	
                        lastchan = chn;
                }

		epgpos = 0;
		g_RCInput->getMsg( &msg, &data, 15); // 15 seconds, not user changable

                if ( msg == CRCInput::RC_ok )
                {
                        if ( ( chn > (signed int) chanlist.size() ) || ( chn == 0 ) )
                        {
                                chn = tuned + 1;
                        }
                        break;
                }
                else if ( msg == CRCInput::RC_left )
                {
                        if ( chn == 1 )
                                chn = chanlist.size();
                        else
                        {
                                chn--;

                                if (chn > (int)chanlist.size())
                                        chn = (int)chanlist.size();
                        }
                }
                else if ( msg == CRCInput::RC_right )
                {
                        chn++;

                        if (chn > (int)chanlist.size())
                                chn = 1;
                }
                else if ( msg == CRCInput::RC_up )
                {
                        epgpos = -1;
                }
                else if ( msg == CRCInput::RC_down )
                {
                        epgpos = 1;
                }
                else if ( ( msg == CRCInput::RC_home ) || ( msg == CRCInput::RC_timeout ) )
                {
                        // Abbruch ohne Channel zu wechseln
                        doZap = false;
                        break;
                }
                else if ( msg == CRCInput::RC_red )
                {
                        // Rote Taste zeigt EPG fuer gewaehlten Kanal an
                        if ( ( chn <= (signed int) chanlist.size() ) && ( chn != 0 ) )
                        {
                                doZap = false;
                                showEPG = true;
                                break;
                        }
                }
                else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
                {
                        doZap = false;
                        break;
                }
        }
        
	g_InfoViewer->clearVirtualZapMode();

        chn--;
        if (chn < 0)
                chn = 0;

        if ( doZap )
        {
		//if(g_settings.timing_infobar == 0)
		//	g_InfoViewer->killTitle();

                zapTo(chn);
        }
        else
        {
        	showInfo(tuned, 0, false);
        	
                //g_InfoViewer->killTitle();

                // Rote Taste zeigt EPG fuer gewaehlten Kanal an
                if ( showEPG )
                        g_EventList->exec(chanlist[chn]->epgid, chanlist[chn]->name);
        }
}

void CChannelList::quickZap(int key, bool cycle)
{
	dprintf(DEBUG_NORMAL, "CChannelList::quickZap:\n");
	
        if(chanlist.size() == 0)
                return;

	if ( (key == g_settings.key_quickzap_down) || (key == CRCInput::RC_left) )
	{
                if(selected == 0)
                        selected = chanlist.size() - 1;
                else
                        selected--;
        }
	else if ((key == g_settings.key_quickzap_up) || (key == CRCInput::RC_right) )
	{
                selected = (selected + 1)%chanlist.size();
        }

	dprintf(DEBUG_NORMAL, "CChannelList::quickZap: quick zap selected = %d getActiveBouquetNumber %d\n", selected, bouquetList->getActiveBouquetNumber());

	if(cycle)
		bouquetList->orgChannelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(selected) - 1);
	else
        	zapTo(selected);

	g_RCInput->clearRCMsg(); //FIXME test for n.103
}

bool CChannelList::canZap(CZapitChannel * channel)
{
	bool iscurrent = true;

	if(!autoshift && CNeutrinoApp::getInstance()->recordingstatus) 
	{
		if(channel == NULL)
			channel = chanlist[selected];
		
		iscurrent = CZapit::getInstance()->CanZap(channel);
	}
	
	return iscurrent;
}

void CChannelList::paint(bool customMode)
{
	//dprintf(DEBUG_NORMAL, "CChannelList::paint: selected: %d\n", selected);
	
	if (!customMode && selected < 0)
		selected = 0;
		
	dprintf(DEBUG_NORMAL, "CChannelList::paint: selected: %d\n", selected);
	
	if (widget)
	{
		delete widget;
		widget = NULL;
	}
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("channellist");
	
	if (widget)
	{
		listBox = (ClistBox*)widget->getCCItem(CComponent::CC_LISTBOX);
		head = (CCHeaders*)widget->getCCItem(CComponent::CC_HEAD);
		foot = (CCFooters*)widget->getCCItem(CComponent::CC_FOOT);
		window = (CCWindow*)widget->getCCItem(CComponent::CC_WINDOW);
		vline = (CCVline*)widget->getCCItem(CComponent::CC_VLINE);
		hline = (CCHline*)widget->getCCItem(CComponent::CC_HLINE);
	}
	else
	{
		widget = new CWidget(&cFrameBox);
		widget->name = "channellist";
		widget->paintMainFrame(true);
		
		// listBox
		listBox = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY + 50, (widget->getWindowsPos().iWidth/3)*2, widget->getWindowsPos().iHeight - 100);
		listBox->paintMainFrame(false);
		
		//
		head = new CCHeaders(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, 50);
		head->enablePaintDate();
//		head->setLine(true, true);
		
		// foot
		foot = new CCFooters(widget->getWindowsPos().iX, widget->getWindowsPos().iY + widget->getWindowsPos().iHeight - 50, widget->getWindowsPos().iWidth, 50);
//		foot->setLine(true, true);
		
		//
		window = new CCWindow(widget->getWindowsPos().iX + (widget->getWindowsPos().iWidth/3)*2, widget->getWindowsPos().iY + 50, widget->getWindowsPos().iWidth/3, widget->getWindowsPos().iHeight - 100);
		
		// vline
		vline = new CCVline(widget->getWindowsPos().iX + (widget->getWindowsPos().iWidth/3)*2, widget->getWindowsPos().iY + 60, 2, widget->getWindowsPos().iHeight - 120);
		vline->setGradient(3);
			
		// hline
		hline = new CCHline(widget->getWindowsPos().iX + (widget->getWindowsPos().iWidth/3)*2 + 10, widget->getWindowsPos().iY + 50 + (widget->getWindowsPos().iHeight - 100)/2, widget->getWindowsPos().iWidth/3 - 20, 2);
		hline->setGradient(3);
		
		widget->addCCItem(listBox);
		//
		widget->addCCItem(head);
		widget->addCCItem(foot);
		widget->addCCItem(window);	
		widget->addCCItem(vline);
		widget->addCCItem(hline);
	}
	
	// wionTop
	if (window)
	{
		winTopBox.iX = window->getWindowsPos().iX + 2;
		winTopBox.iY = window->getWindowsPos().iY + 2;
		winTopBox.iWidth = window->getWindowsPos().iWidth - 4;
		winTopBox.iHeight = window->getWindowsPos().iHeight/2 - 4;
		
		// winBottom
		winBottomBox.iX = window->getWindowsPos().iX + 2;
		winBottomBox.iY = window->getWindowsPos().iY + window->getWindowsPos().iHeight/2 + 2;
		winBottomBox.iWidth = window->getWindowsPos().iWidth - 4;
		winBottomBox.iHeight = window->getWindowsPos().iHeight/2 - 4;
	}

	if (head) 
		head->clear();
		
	if (listBox) 
		listBox->clear();
		
	if (foot) 
		foot->clear();
	
	//
	CChannelEvent * p_event = NULL;
	time_t jetzt = time(NULL);
	unsigned int runningPercent = 0;

	char cSeit[11] = " ";
	char cNoch[11] = " ";

	if(chanlist.size())
	{
		for(unsigned int i = 0; i < chanlist.size(); i++)
		{
			p_event = NULL;
			jetzt = time(NULL);
			runningPercent = 0;

			// desc
			std::string desc = chanlist[i]->description;

			p_event = &chanlist[i]->currentEvent;

			// runningPercent	
			if (((jetzt - p_event->startTime + 30) / 60) < 0 )
			{
				runningPercent = 0;
			}
			else
			{
				if(p_event->duration > 0)
					runningPercent = (unsigned) ((float) (jetzt - p_event->startTime) / (float) p_event->duration * 100.);
					
				if(runningPercent > 100)
					runningPercent = 0;
			}

			// description
			if (p_event != NULL && !(p_event->description.empty())) 
			{
				desc = p_event->description;
			}
			
			std::string option = "";
			
			if (!p_event->description.empty())
			{
			 	option = " - ";
				option += p_event->description.c_str();
			}

			item = new CMenuForwarder(chanlist[i]->name.c_str(), true, option.c_str());

			// channel number
			item->setNumber(/*chanlist[i]->number*/i + 1);
			
			// timescale
			if (g_settings.channellist_timescale) item->setPercent(runningPercent);
			
			// hd / ca / webtv icon
			if (g_settings.channellist_ca)
			{
				item->setIcon1(chanlist[i]->isWEBTV()? NEUTRINO_ICON_WEBTV_SMALL : chanlist[i]->isHD() ? NEUTRINO_ICON_HD : chanlist[i]->isUHD()? NEUTRINO_ICON_UHD : "");
				
				//
				item->setIcon2(chanlist[i]->scrambled? NEUTRINO_ICON_SCRAMBLED : "");
			}
			
			// logo
			if (g_settings.logos_show_logo)
			{
				std::string logo;

				logo = CChannellogo::getInstance()->getLogoName(chanlist[i]->getLogoID());
				
				item->setIconName(logo.c_str());
			}
			
			// option font color
			item->setOptionFontColor(COL_INFOBAR_COLORED_EVENTS_TEXT_PLUS_0);
			

			if (listBox) listBox->addItem(item);
		}
	}
	
	// head
	if (head) 
	{
		head->setTitle(_(name.c_str()));
		head->setButtons(HeadButtons, HEAD_BUTTONS_COUNT);
	}

	// foot
	if (foot) 
	{
		foot->setButtons(CChannelListButtons, NUM_LIST_BUTTONS);
	}

	//
	if (listBox) listBox->setSelected(selected);

	if (widget) widget->paint();
	
	//
	paintCurrentNextEvent(selected);
}

void CChannelList::paintCurrentNextEvent(int _selected)
{
	if (_selected < 0 || chanlist.size() == 0)
		return;
		
	if (window)
	{
		window->refresh();
	}
	
	if (hline) 
		hline->paint();
	if (vline) 
		vline->paint();
	
	CChannelEvent * p_event = NULL;
	time_t jetzt = time(NULL);
	unsigned int runningPercent = 0;

	char cSeit[11] = " ";
	char cNoch[11] = " ";
	
	// now
	p_event = &chanlist[_selected]->currentEvent;
	
	// title
	CCLabel epgTitle(winTopBox.iX + 10, winTopBox.iY + 10, winTopBox.iWidth - 20, 60);
	epgTitle.setText(p_event->description.c_str());
	epgTitle.setHAlign(CComponent::CC_ALIGN_CENTER);
	epgTitle.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	// runningPercent
	runningPercent = 0;
		
	if (((jetzt - p_event->startTime + 30) / 60) < 0 )
	{
		runningPercent = 0;
	}
	else
	{
		if(p_event->duration > 0)
			runningPercent = (unsigned) ((float) (jetzt - p_event->startTime) / (float) p_event->duration * 100.);
			
		if(runningPercent > 100)
			runningPercent = 0;
	}
	
	CCProgressBar pb(winTopBox.iX + 100, winTopBox.iY + 10 + 60 + 20, winTopBox.iWidth - 200, 5);
	
	//
	if (p_event != NULL && !(p_event->description.empty())) 
	{
		// start
		struct tm * pStartZeit = localtime(&p_event->startTime);
		sprintf(cSeit, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		
		// end
		long int uiEndTime(p_event->startTime + p_event->duration);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);

		sprintf(cNoch, "%02d:%02d", pEndeZeit->tm_hour, pEndeZeit->tm_min);
	}
	
	CCLabel startTime(winTopBox.iX + 10, winTopBox.iY + 10 + 60 + 10, 80, 20);
	startTime.setText(cSeit);
	startTime.setHAlign(CComponent::CC_ALIGN_CENTER);
	startTime.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	
	CCLabel restTime(winTopBox.iX + 100 + winTopBox.iWidth - 200 + 10, winTopBox.iY + 10 + 60 + 10, 80, 20);
	restTime.setText(cNoch);
	restTime.setHAlign(CComponent::CC_ALIGN_CENTER);
	restTime.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	
	// text
	CCText text(winTopBox.iX + 10, winTopBox.iY + 10 + 60 + 10 + 30, winTopBox.iWidth - 20, winTopBox.iHeight - 120);
	text.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	text.setText(p_event->text.c_str());
	text.setHAlign(CComponent::CC_ALIGN_CENTER);
	
	// next
	time_t atime = time(NULL);
					
	events.clear();

	CSectionsd::getInstance()->getEventsServiceKey(chanlist[_selected]->channel_id & 0xFFFFFFFFFFFFULL, events);
	chanlist[_selected]->nextEvent.startTime = (long)0x7fffffff;
				
	for ( CChannelEventList::iterator e = events.begin(); e != events.end(); ++e ) 
	{
		if (((long)(e->startTime) > atime) && ((e->startTime) < (long)(chanlist[_selected]->nextEvent.startTime)))
		{
			chanlist[_selected]->nextEvent= *e;
					
			break;
		}
	}
	
	p_event = &chanlist[_selected]->nextEvent;
	
	//
	CCLabel nextTitle(winBottomBox.iX + 10, winBottomBox.iY + 10, winBottomBox.iWidth - 20, 20);
	nextTitle.setText(p_event->description.c_str());
	nextTitle.setHAlign(CComponent::CC_ALIGN_CENTER);
	nextTitle.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	// vom/bis
	std::string fromto = "";
	CCLabel nextTime(winBottomBox.iX + 10, winBottomBox.iY + 10 + 30, winBottomBox.iWidth - 20, 20);
	nextTime.setHAlign(CComponent::CC_ALIGN_CENTER);
	nextTime.setFont(SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE);
	
	//
	if (p_event != NULL && !(p_event->description.empty())) 
	{
		// start
		struct tm * pStartZeit = localtime(&p_event->startTime);
		sprintf(cSeit, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		
		// end
		long int uiEndTime(p_event->startTime + p_event->duration);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);

		sprintf(cNoch, "%02d:%02d", pEndeZeit->tm_hour, pEndeZeit->tm_min);
	}
	
	fromto = cSeit;
	fromto += "   -   ";
	fromto += cNoch;
	
	nextTime.setText(fromto.c_str());
	
	// nextText
	CCText nextText(winBottomBox.iX + 10, winBottomBox.iY + 10 + 60, winBottomBox.iWidth - 20, winBottomBox.iHeight - 80);
	nextText.setFont(SNeutrinoSettings::FONT_TYPE_EPG_INFO2);
	nextText.setText(p_event->text.c_str());
	nextText.setHAlign(CComponent::CC_ALIGN_CENTER);
	
	// current
	epgTitle.paint();
	pb.reset();
	pb.refresh(runningPercent);
	startTime.paint();
	restTime.paint();
	text.paint();
	// next
	nextTitle.paint();
	nextTime.paint();
	nextText.paint();
}

//
int CChannelList::doChannelMenu(void)
{
	int i = 0;
	int select = -1;
	static int old_selected = 0;
	signed int bouquet_id, old_bouquet_id, new_bouquet_id;
	int result;
	t_channel_id channel_id;
	
	if( !bouquetList )
		return 0;
	
	//
	CWidget* mWidget = NULL;
	ClistBox* menu = NULL;
	
	mWidget = CNeutrinoApp::getInstance()->getWidget("channellistedit");
	
	if (mWidget)
	{
		menu = (ClistBox*)mWidget->getCCItem(CComponent::CC_LISTBOX);
	}
	else
	{
		//
		mWidget = new CWidget(0, 0, 500, 250);
		mWidget->name = "channellistedit";
		mWidget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		menu = new ClistBox(mWidget->getWindowsPos().iX, mWidget->getWindowsPos().iY, mWidget->getWindowsPos().iWidth, mWidget->getWindowsPos().iHeight);

		menu->setWidgetMode(ClistBox::MODE_MENU);
		
		//
		menu->enablePaintHead();
		menu->setTitle(_("Edit"), NEUTRINO_ICON_SETTINGS);
//		menu->setHeadLine(true, true);

		//
		menu->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		menu->setFootButtons(&btn);
//		menu->setFootLine(true, true);
		
		//
		mWidget->addCCItem(menu);
	}
	
	//
	mWidget->setCorner(g_settings.Head_radius > g_settings.Foot_radius? g_settings.Head_radius : g_settings.Foot_radius, g_settings.Head_corner | g_settings.Foot_corner);
	mWidget->setBorderMode();
	mWidget->paintMainFrame(true);
	mWidget->enableSaveScreen();
	
	//
	menu->addItem(new CMenuForwarder(_("delete")), old_selected == i++);
	menu->addItem(new CMenuForwarder(_("Move")), old_selected == i++);
	menu->addItem(new CMenuForwarder(_("Add to Bouquets")), old_selected == i++);
	menu->addItem(new CMenuForwarder(_("add channel to my favorites")), old_selected == i++);

	mWidget->exec(NULL, "");
	select = menu->getSelected();
	
	if (widget)
	{
		delete mWidget;
		mWidget = NULL;
	}

	if(select >= 0) 
	{
		hide();
		
		old_selected = select;
		channel_id = chanlist[selected]->channel_id;
		
		switch(select) 
		{
			case 0: //delete
				result = MessageBox(_("Delete"), _("Delete channel from bouquet?"), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );

				if(result == CMessageBox::mbrYes) 
				{
					bouquet_id = bouquetList->getActiveBouquetNumber();
					bouquet_id = CZapit::getInstance()->existsBouquet(bouquetList->Bouquets[bouquet_id]->channelList->getName());

					if (bouquet_id == -1)
						return 0;
					
					if(CZapit::getInstance()->existsChannelInBouquet(bouquet_id, channel_id)) 
					{
						CZapit::getInstance()->Bouquets[bouquet_id]->removeService(channel_id);
						return 1;
					}
				}
				break;
				
			case 1: // move
				old_bouquet_id = bouquetList->getActiveBouquetNumber();
				old_bouquet_id = CZapit::getInstance()->existsBouquet(bouquetList->Bouquets[old_bouquet_id]->channelList->getName());

				do {
					new_bouquet_id = bouquetList->exec(false, false);
				} while(new_bouquet_id == -3);

				hide();
				
				if(new_bouquet_id < 0)
					return 0;
					
				new_bouquet_id = CZapit::getInstance()->existsBouquet(bouquetList->Bouquets[new_bouquet_id]->channelList->getName());
				if ((new_bouquet_id == -1) || (new_bouquet_id == old_bouquet_id))
					return 0;

				if(!CZapit::getInstance()->existsChannelInBouquet(new_bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(new_bouquet_id, channel_id);
				}
				
				if(CZapit::getInstance()->existsChannelInBouquet(old_bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->Bouquets[old_bouquet_id]->removeService(channel_id);
				}
				return 1;

				break;
				
			case 2: // add to
				do {
					bouquet_id = bouquetList->exec(false, false);
				} while(bouquet_id == -3);
				
				hide();
				
				if(bouquet_id < 0)
					return 0;
				
				bouquet_id = CZapit::getInstance()->existsBouquet(bouquetList->Bouquets[bouquet_id]->channelList->getName());
				if (bouquet_id == -1)
					return 0;
				
				if(!CZapit::getInstance()->existsChannelInBouquet(bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(bouquet_id, channel_id);
					return 1;
				}
				break;
				
			case 3: // add to my favorites
				bouquet_id = CZapit::getInstance()->existsBouquet("Favorites");

				if(bouquet_id == -1) 
				{
					CZapit::getInstance()->addBouquet("Favorites", true);
					bouquet_id = CZapit::getInstance()->existsBouquet("Favorites");
				}
				
				if(!CZapit::getInstance()->existsChannelInBouquet(bouquet_id, channel_id)) 
				{
					CZapit::getInstance()->addChannelToBouquet(bouquet_id, channel_id);
					return 1;
				}
				
				break;
				
			default:
				break;
		}
	}
	
	return 0;
}

