/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: epgplus.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2004 Martin Griep 'vivamiga'

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

#include <iostream>

#include <global.h>
#include <neutrino2.h>

#include <gui/epgplus.h>
#include <sectionsd/sectionsd.h>

#include <timerd/timerd.h>

#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/infobox.h>

#include <gui/bouquetlist.h>

#include <driver/rcinput.h>

#include <algorithm>
#include <sstream>

#include <system/debug.h>
#include <system/channellogo.h>
#include <system/tmdbparser.h>


#define COL_MENUCONTENT_P1                 254-8*4+1
#define COL_MENUCONTENT_P2                 254-8*4+2

extern CBouquetList *bouquetList;

int sizes[EpgPlus::NumberOfSizeSettings];

time_t EpgPlus::duration = 0;

int EpgPlus::horGap1Height = 0;
int EpgPlus::horGap2Height = 0;
int EpgPlus::verGap1Width = 0;
int EpgPlus::verGap2Width = 0;

int EpgPlus::horGap1Color = 0;
int EpgPlus::horGap2Color = 0;
int EpgPlus::verGap1Color = 0;
int EpgPlus::verGap2Color = 0;

int EpgPlus::sliderWidth = 0;
int EpgPlus::channelsTableWidth = 0;

static EpgPlus::SizeSetting sizeSettingTable[] = {
	{EpgPlus::EPGPlus_channelentry_width, 100},
	{EpgPlus::EPGPlus_channelentry_separationlineheight, 2},
	{EpgPlus::EPGPlus_slider_width, 15},
	{EpgPlus::EPGPlus_horgap1_height, 4},
	{EpgPlus::EPGPlus_horgap2_height, 4},
	{EpgPlus::EPGPlus_vergap1_width, 4},
	{EpgPlus::EPGPlus_vergap2_width, 4},
};

// Header
CFont * EpgPlus::Header::font = NULL;
CHeaders *EpgPlus::Header::head = NULL;

EpgPlus::Header::Header(CFrameBuffer * _frameBuffer, int _x, int _y, int _width)
{
	this->frameBuffer = _frameBuffer;
	this->x = _x;
	this->y = _y;
	this->width = _width;
}

EpgPlus::Header::~Header()
{
	if (head)
	{
		delete head;
		head = NULL;
	}
}

void EpgPlus::Header::init()
{
  	font = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE];
  	head = NULL;
}

void EpgPlus::Header::paint()
{
	head = new CHeaders(this->x, this->y, this->width, /*this->font->getHeight() + 10*/40, _("Eventlist overview"), NEUTRINO_ICON_BUTTON_EPG);

	head->setCorner(RADIUS_SMALL, CORNER_TOP);
	head->setGradient(LIGHT2DARK);
	head->setLine(true, true);
	head->enablePaintDate();
	head->setFormat("%d.%m.%Y %H:%M:%S");
	head->addButton(NEUTRINO_ICON_BUTTON_HELP);
	
	head->paint();
}

void EpgPlus::Header::refresh()
{
	head->refresh();
}

int EpgPlus::Header::getUsedHeight()
{
  	return 40; //font->getHeight() + 10;
}

// TimeLine
CFont *EpgPlus::TimeLine::fontTime = NULL;
CFont *EpgPlus::TimeLine::fontDate = NULL;

EpgPlus::TimeLine::TimeLine(CFrameBuffer * _frameBuffer, int _x, int _y, int _width, int _startX, int _durationX)
{
	this->frameBuffer = _frameBuffer;
	this->x = _x;
	this->y = _y;
	this->width = _width;
	this->startX = _startX;
	this->durationX = _durationX;
}

void EpgPlus::TimeLine::init()
{
	fontTime = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL];
	fontDate = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL];
}

EpgPlus::TimeLine::~TimeLine()
{
}

void EpgPlus::TimeLine::paint(time_t startTime, int _duration)
{
	this->clearMark ();
	
	int xPos = this->startX;
	
	this->currentDuration = _duration;
	int numberOfTicks = this->currentDuration / (60 * 60) * 2;
	int tickDist = (this->durationX) / numberOfTicks;
	time_t tickTime = startTime;
	bool toggleColor = false;
	
	// display date of begin
	this->frameBuffer->paintBoxRel(this->x, this->y, this->width, this->fontTime->getHeight(), toggleColor ? COL_MENUCONTENT_PLUS_2 : COL_MENUCONTENT_PLUS_1);
	
	this->fontDate->RenderString(this->x + 4, this->y + this->fontDate->getHeight(), this->width, EpgPlus::getTimeString(startTime, "%d-%b") , COL_MENUCONTENT, 0, true);	// UTF-8
	
	// paint ticks
	for (int i = 0; i < numberOfTicks; ++i, xPos += tickDist, tickTime += _duration / numberOfTicks) 
	{
		int xWidth = tickDist;
		if (xPos + xWidth > this->x + width)
			xWidth = this->x + width - xPos;
	
		this->frameBuffer->paintBoxRel(xPos, this->y, xWidth, this->fontTime->getHeight(), toggleColor ? COL_MENUCONTENT_PLUS_1 : COL_MENUCONTENT_PLUS_2);
	
		std::string timeStr = EpgPlus::getTimeString(tickTime, "%H");
	
		int textWidth = this->fontTime->getRenderWidth(timeStr, true);
	
		this->fontTime->RenderString (xPos - textWidth - 4, this->y + this->fontTime->getHeight(), textWidth, timeStr, toggleColor ? COL_MENUCONTENT_P1 : COL_MENUCONTENT_P2, 0, true);	// UTF-8
	
		timeStr = EpgPlus::getTimeString (tickTime, "%M");
		textWidth = this->fontTime->getRenderWidth (timeStr, true);
		this->fontTime->RenderString (xPos + 4, this->y + this->fontTime->getHeight(), textWidth, timeStr, toggleColor ? COL_MENUCONTENT_P1 : COL_MENUCONTENT_P2, 0, true);	// UTF-8
	
		toggleColor = !toggleColor;
	}
}

void EpgPlus::TimeLine::paintGrid()
{
	int xPos = this->startX;
	int numberOfTicks = this->currentDuration / (60 * 60) * 2;
	int tickDist = (this->durationX) / numberOfTicks;
	
	// paint ticks
	for (int i = 0; i < numberOfTicks; ++i, xPos += tickDist) 
	{
		// display a line for the tick
		this->frameBuffer->paintVLineRel(xPos, this->y, this->fontTime->getHeight(), COL_MENUCONTENT_PLUS_5);
	}
}

void EpgPlus::TimeLine::paintMark(time_t startTime, int _duration, int _x, int _width)
{
	// clear old mark
	this->clearMark();
	
	// paint new mark
	this->frameBuffer->paintBoxRel(_x, this->y + this->fontTime->getHeight(), _width, this->fontTime->getHeight() , COL_MENUCONTENTSELECTED_PLUS_0);
	
	// display start time before mark
	std::string timeStr = EpgPlus::getTimeString (startTime, "%H:%M");
	int textWidth = this->fontTime->getRenderWidth (timeStr, true);
	
	this->fontTime->RenderString(_x - textWidth, this->y + this->fontTime->getHeight() + this->fontTime->getHeight(), textWidth, timeStr, COL_MENUCONTENT, 0, true);	// UTF-8
	
	// display end time after mark
	timeStr = EpgPlus::getTimeString (startTime + _duration, "%H:%M");
	textWidth = fontTime->getRenderWidth (timeStr, true);
	
	if (_x + _width + textWidth < this->x + this->width) 
	{
		this->fontTime->RenderString(_x + _width, this->y + this->fontTime->getHeight() + this->fontTime->getHeight(), textWidth, timeStr, COL_MENUCONTENT, 0, true);	// UTF-8
	} 
	else if (textWidth < _width - 10) 
	{
		this->fontTime->RenderString(_x + _width - textWidth, this->y + this->fontTime->getHeight() + this->fontTime->getHeight(), textWidth, timeStr, COL_MENUCONTENTSELECTED, 0, true);	// UTF-8
  	}
}

void EpgPlus::TimeLine::clearMark()
{
	this->frameBuffer->paintBoxRel(this->x, this->y + this->fontTime->getHeight(), this->width, this->fontTime->getHeight() , COL_MENUCONTENT_PLUS_0);
}

int EpgPlus::TimeLine::getUsedHeight()
{
	return std::max(fontDate->getHeight(), fontTime->getHeight()) + fontTime->getHeight();
}

// ChannelEventEntry
CFont * EpgPlus::ChannelEventEntry::font = NULL;
int EpgPlus::ChannelEventEntry::separationLineHeight = 0;

EpgPlus::ChannelEventEntry::ChannelEventEntry(const CChannelEvent * _channelEvent, CFrameBuffer * _frameBuffer, TimeLine * _timeLine, Footer * _footer, int _x, int _y, int _width)
{
	// copy neccessary?
	if (_channelEvent != NULL)
		this->channelEvent = *_channelEvent;
	
	this->frameBuffer = _frameBuffer;
	this->timeLine = _timeLine;
	this->footer = _footer;
	this->x = _x;
	this->y = _y;
  	this->width = _width;
}

void EpgPlus::ChannelEventEntry::init()
{
	font = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL];
	separationLineHeight = sizes[EPGPlus_channelentry_separationlineheight];
}

EpgPlus::ChannelEventEntry::~ChannelEventEntry()
{
}

bool EpgPlus::ChannelEventEntry::isSelected (time_t selectedTime) const
{
	   return (selectedTime >= this->channelEvent.startTime) && (selectedTime < this->channelEvent.startTime + time_t (this->channelEvent.duration));
}

void EpgPlus::ChannelEventEntry::paint (bool _isSelected, bool toggleColor)
{
	this->frameBuffer->paintBoxRel(this->x, this->y, this->width, this->font->getHeight() + 10, this->channelEvent.description.empty()? COL_MENUCONTENT_PLUS_0 : (_isSelected ? COL_MENUCONTENTSELECTED_PLUS_0 : (toggleColor ? COL_MENUCONTENT_PLUS_1 : COL_MENUCONTENT_PLUS_2)));
	
	this->font->RenderString(this->x + 2, this->y + this->font->getHeight() + 5, this->width - 4 > 0 ? this->width - 4 : 0, this->channelEvent.description, _isSelected ? COL_MENUCONTENTSELECTED : (toggleColor ? COL_MENUCONTENT_P1 : COL_MENUCONTENT_P2), 0, true);
	
	// paint the separation line
	if (separationLineHeight > 0) 
	{
		this->frameBuffer->paintBoxRel(this->x, this->y + this->font->getHeight() + 10, this->width, this->separationLineHeight, COL_MENUCONTENT_PLUS_5);
	}
	
	if (_isSelected) 
	{
		if (this->channelEvent.description.empty ()) 
		{	// dummy channel event
			this->timeLine->clearMark();
		} 
		else 
		{
			this->timeLine->paintMark(this->channelEvent.startTime, this->channelEvent.duration, this->x, this->width);
		}
	
		CShortEPGData shortEpgData;
	
		this->footer->paintEventDetails(this->channelEvent.description, CSectionsd::getInstance()->getEPGidShort(this->channelEvent.eventID, &shortEpgData) ? shortEpgData.info1 : "");
	
		this->timeLine->paintGrid();
	}
}

int EpgPlus::ChannelEventEntry::getUsedHeight()
{
  	return font->getHeight() + 10 + separationLineHeight;
}

// ChannelEntry
CFont * EpgPlus::ChannelEntry::font = NULL;
int EpgPlus::ChannelEntry::separationLineHeight = 0;

EpgPlus::ChannelEntry::ChannelEntry(const CZapitChannel * _channel, int _index, CFrameBuffer * _frameBuffer, Footer * _footer, CBouquetList * _bouquetList, int _x, int _y, int _width)
{
	this->channel = _channel;
	
	if (_channel != NULL) 
	{
		std::stringstream _displayName;
		//_displayName << index + 1 << " " << _channel->getName();
		_displayName << _channel->number << " " << _channel->getName();
	
		this->displayName = _displayName.str ();
	}
	
	this->index = _index;
	
	this->frameBuffer = _frameBuffer;
	this->footer = _footer;
	this->bouquetList = _bouquetList;
	
	this->x = _x;
	this->y = _y;
	this->width = _width;
}

void EpgPlus::ChannelEntry::init()
{
	font = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL];
	separationLineHeight = sizes[EPGPlus_channelentry_separationlineheight];
}

EpgPlus::ChannelEntry::~ChannelEntry()
{
	for (TCChannelEventEntries::iterator It = this->channelEventEntries.begin (); It != this->channelEventEntries.end (); It++) 
	{
		delete *It;
	}
	
	this->channelEventEntries.clear();
}

void EpgPlus::ChannelEntry::paint (bool isSelected, time_t selectedTime)
{
	this->frameBuffer->paintBoxRel(this->x, this->y, this->width, this->font->getHeight() + 10, isSelected ? COL_MENUCONTENTSELECTED_PLUS_0 : COL_MENUCONTENT_PLUS_0);
	
	//FIXME
	// display channel picon
	bool logo_ok = false;
	
	if(g_settings.logos_show_logo)
	{
		int logo_w = this->width -2; 
		int logo_h = this->font->getHeight() + 10 - 2;
		int pic_w = (this->font->getHeight() - 2)*1.67;
		int logo_bpp = 0;
		
		// check logo
		logo_ok = CChannellogo::getInstance()->checkLogo(this->channel->getLogoID());
		
		if(logo_ok)
		{
			// get logo size	
			CChannellogo::getInstance()->getLogoSize(this->channel->getLogoID(), &logo_w, &logo_h, &logo_bpp);
		
			// paint logo (png with alpha channel)
			CChannellogo::getInstance()->displayLogo(this->channel->getLogoID(), this->x + 1 + ((logo_bpp == 4)? 0 : (this->width - 2 - pic_w)/2), this->y + 1, (logo_bpp == 4)? this->width -2 : pic_w, this->font->getHeight() + 10 - 2, true);
		}
	}
	
	if(!logo_ok)
		// display channel number+ channel name
		this->font->RenderString (this->x + 2, this->y + this->font->getHeight() + 5, this->width - 4, this->displayName, isSelected ? COL_MENUCONTENTSELECTED : COL_MENUCONTENT, 0, true);
	
	if (isSelected) 
	{
		for (uint32_t i = 0; i < this->bouquetList->Bouquets.size (); ++i) 
		{
			CBouquet *bouquet = this->bouquetList->Bouquets[i];
			for (int j = 0; j < bouquet->channelList->getSize (); ++j) 
			{
				if ((*bouquet->channelList)[j]->number == this->channel->number) 
				{
					this->footer->setBouquetChannelName(bouquet->channelList->getName(), this->channel->getName());
	
					bouquet = NULL;
	
					break;
				}
			}
			if (bouquet == NULL)
				break;
		}
	}
	
	// paint the separation line
	if (separationLineHeight > 0) 
	{
		this->frameBuffer->paintBoxRel (this->x, this->y + this->font->getHeight() + 10, this->width, this->separationLineHeight, COL_MENUCONTENT_PLUS_5);
	}
	
	bool toggleColor = false;
	for (TCChannelEventEntries::iterator It = this->channelEventEntries.begin (); It != this->channelEventEntries.end (); ++It) 
	{
		(*It)->paint (isSelected && (*It)->isSelected (selectedTime), toggleColor);
	
		toggleColor = !toggleColor;
	}
}

int EpgPlus::ChannelEntry::getUsedHeight ()
{
	return font->getHeight() + 10 + separationLineHeight;
}

// Footer
CFont *EpgPlus::Footer::fontBouquetChannelName = NULL;
CFont *EpgPlus::Footer::fontEventDescription = NULL;
CFont *EpgPlus::Footer::fontEventShortDescription = NULL;
CFont *EpgPlus::Footer::fontButtons = NULL;
int EpgPlus::Footer::color = 0;

EpgPlus::Footer::Footer (CFrameBuffer * _frameBuffer, int _x, int _y, int _width)
{
	this->frameBuffer = _frameBuffer;
	this->x = _x;
	this->y = _y;
	this->width = _width;
}

EpgPlus::Footer::~Footer ()
{
}

void EpgPlus::Footer::init()
{
	fontBouquetChannelName = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL];
	fontEventDescription = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE];
	fontEventShortDescription = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL];
	fontButtons = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL];
}

void EpgPlus::Footer::setBouquetChannelName (const std::string & newBouquetName, const std::string & newChannelName)
{
	this->currentBouquetName = newBouquetName;
	this->currentChannelName = newChannelName;
}

int EpgPlus::Footer::getUsedHeight ()
{
	return fontBouquetChannelName->getHeight() + fontEventDescription->getHeight() + fontEventShortDescription->getHeight() + /*fontButtons->getHeight() + 10*/40;
}

void EpgPlus::Footer::paintEventDetails (const std::string & description, const std::string & shortDescription)
{
	int yPos = this->y;
	
	int height = this->fontBouquetChannelName->getHeight ();
	
	// clear the region
	this->frameBuffer->paintBoxRel (this->x, yPos, this->width, height, COL_MENUHEAD_PLUS_0);
	
	yPos += height;
	
	// display new text
	this->fontBouquetChannelName->RenderString (this->x + 10, yPos, this->width - 20, this->currentBouquetName + " : " + this->currentChannelName, COL_MENUHEAD, 0, true);
	
	height = this->fontEventDescription->getHeight ();
	
	// clear the region
	this->frameBuffer->paintBoxRel (this->x, yPos, this->width, height, COL_MENUHEAD_PLUS_0);
	
	yPos += height;
	
	// display new text
	this->fontEventDescription->RenderString (this->x + 10, yPos, this->width - 20, description, COL_MENUHEAD, 0, true);
	
	height = this->fontEventShortDescription->getHeight ();
	
	// clear the region
	this->frameBuffer->paintBoxRel (this->x, yPos, this->width, height, COL_MENUHEAD_PLUS_0);
	
	yPos += height;
	
	// display new text
	this->fontEventShortDescription->RenderString (this->x + 10, yPos, this->width - 20, shortDescription, COL_MENUHEAD, 0, true);
}

struct button_label buttonLabels[] = {
	{NEUTRINO_ICON_BUTTON_RED, _("Record") },
	{NEUTRINO_ICON_BUTTON_GREEN, _("Refresh EPG") },
	{NEUTRINO_ICON_BUTTON_YELLOW, _("Schedule") },
	{NEUTRINO_ICON_BUTTON_BLUE, _("TMDB info") }
};

void EpgPlus::Footer::paintButtons(button_label * _buttonLabels, int numberOfButtons)
{
	CFooters foot(this->x, this->y + this->getUsedHeight() - (/*this->fontButtons->getHeight() + 10*/40), this->width, /*this->fontButtons->getHeight() + 10*/40);

	foot.setCorner(RADIUS_SMALL, CORNER_BOTTOM);
	foot.setGradient(DARK2LIGHT);
	foot.setLine(true, true);
	foot.setButtons(buttonLabels, numberOfButtons);
	foot.paint();
}

EpgPlus::EpgPlus()
{
  	this->init();
}

EpgPlus::~EpgPlus()
{
 	 this->free();
}

void EpgPlus::createChannelEntries (int selectedChannelEntryIndex)
{
	for (TChannelEntries::iterator It = this->displayedChannelEntries.begin (); It != this->displayedChannelEntries.end (); It++) 
	{
		delete *It;
	}
	
	this->displayedChannelEntries.clear ();
	
	this->selectedChannelEntry = NULL;
	
	if (selectedChannelEntryIndex < this->channelList->getSize ()) 
	{
		for (;;) 
		{
			if (selectedChannelEntryIndex < this->channelListStartIndex) 
			{
				this->channelListStartIndex -= this->maxNumberOfDisplayableEntries;
				if (this->channelListStartIndex < 0)
					this->channelListStartIndex = 0;
			} 
			else if (selectedChannelEntryIndex >= this->channelListStartIndex + this->maxNumberOfDisplayableEntries) 
			{
				this->channelListStartIndex += this->maxNumberOfDisplayableEntries;
			} 
			else
				break;
		}
	
		int yPosChannelEntry = this->channelsTableY;
		int yPosEventEntry = this->eventsTableY;
	
		for (int i = this->channelListStartIndex; (i < this->channelListStartIndex + this->maxNumberOfDisplayableEntries)
			&& (i < this->channelList->getSize ());
			++i, yPosChannelEntry += this->entryHeight, yPosEventEntry += this->entryHeight) 
		{
			CZapitChannel * channel = (*this->channelList)[i];
	
			ChannelEntry * channelEntry = new ChannelEntry (channel, i, this->frameBuffer, this->footer, this->bouquetList, this->channelsTableX + 2, yPosChannelEntry, this->channelsTableWidth);
			
			CChannelEventList channelEventList;
			CSectionsd::getInstance()->getEventsServiceKey(channel->channel_id & 0xFFFFFFFFFFFFULL, channelEventList);
	
			int xPosEventEntry = this->eventsTableX;
			int widthEventEntry = 0;
			time_t lastEndTime = this->startTime;
		
			CChannelEventList::const_iterator lastIt (channelEventList.end ());
			
			for (CChannelEventList::const_iterator It = channelEventList.begin (); It != channelEventList.end (); ++It) 
			{
				if(!(It->startTime < (this->startTime + this->duration)) )
					continue;
				
				if ((lastIt == channelEventList.end ()) || (lastIt->startTime != It->startTime)) 
				{
					int startTimeDiff = It->startTime - this->startTime;
					int endTimeDiff = this->startTime + time_t (this->duration) - It->startTime - time_t (It->duration);

					if ((startTimeDiff >= 0) && (endTimeDiff >= 0)) 
					{
						// channel event fits completely in the visible part of time line
						startTimeDiff = 0;
						endTimeDiff = 0;
					} 
					else if ((startTimeDiff < 0) && (endTimeDiff < 0))
					{
						// channel event starts and ends outside visible part of the time line but covers complete visible part
					} 
					else if ((startTimeDiff < 0) && (endTimeDiff < this->duration)) 
					{
						// channel event starts before visible part of the time line but ends in the visible part
						endTimeDiff = 0;
					} 
					else if ((endTimeDiff < 0) && (startTimeDiff < this->duration)) 
					{
						// channel event ends after visible part of the time line but starts in the visible part
						startTimeDiff = 0;
					} 
					else if (startTimeDiff > 0) 
					{	
						// channel event starts and ends after visible part of the time line => break the loop
						break;
					} 
					else 
					{				
						// channel event starts and ends after visible part of the time line => ignore the channel event
						continue;
					}
		
					if (lastEndTime < It->startTime) 
					{	
						// there is a gap between last end time and new start time => fill it with a new event entry
						CChannelEvent channelEvent;
						channelEvent.startTime = lastEndTime;
						channelEvent.duration = It->startTime - channelEvent.startTime;
				
						ChannelEventEntry *channelEventEntry = new ChannelEventEntry (&channelEvent, this->frameBuffer, this->timeLine, this->footer, this->eventsTableX + ((channelEvent.startTime - this->startTime) * this->eventsTableWidth) / this->duration, yPosEventEntry, (channelEvent.duration * this->eventsTableWidth) / this->duration + 1);
						channelEntry->channelEventEntries.push_back (channelEventEntry);
					}
					
					// correct position
					xPosEventEntry = this->eventsTableX + ((It->startTime - startTimeDiff - this->startTime) * this->eventsTableWidth) / this->duration;
		
					// correct width
					widthEventEntry = ((It->duration + startTimeDiff + endTimeDiff) * this->eventsTableWidth) / this->duration + 1;
		
					if (widthEventEntry < 0)
						widthEventEntry = 0;
		
					if (xPosEventEntry + widthEventEntry > this->eventsTableX + this->eventsTableWidth)
						widthEventEntry = this->eventsTableX + this->eventsTableWidth - xPosEventEntry;
		
					ChannelEventEntry *channelEventEntry = new ChannelEventEntry (&(*It) , this->frameBuffer, this->timeLine, this->footer, xPosEventEntry, yPosEventEntry, widthEventEntry);
		
					channelEntry->channelEventEntries.push_back (channelEventEntry);
					lastEndTime = It->startTime + It->duration;
				}
				
				lastIt = It;
			}
	
			if (lastEndTime < this->startTime + time_t (this->duration)) 
			{	
				// there is a gap between last end time and end of the timeline => fill it with a new event entry
				CChannelEvent channelEvent;
				channelEvent.startTime = lastEndTime;
				channelEvent.duration = this->startTime + this->duration - channelEvent.startTime;
			
				ChannelEventEntry *channelEventEntry = new ChannelEventEntry (&channelEvent, this->frameBuffer, this->timeLine, this->footer, this->eventsTableX + ((channelEvent.startTime - this->startTime) * this->eventsTableWidth) / this->duration, yPosEventEntry, (channelEvent.duration * this->eventsTableWidth) / this->duration + 1);
				channelEntry->channelEventEntries.push_back (channelEventEntry);
			}
		
			this->displayedChannelEntries.push_back (channelEntry);
		}
	
		this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];
	}
}

void EpgPlus::init()
{
	frameBuffer = CFrameBuffer::getInstance ();
	usableScreenWidth = g_settings.screen_EndX;
	usableScreenHeight = g_settings.screen_EndY;
	
	//
	for (size_t i = 0; i < NumberOfSizeSettings; ++i) 
	{
		sizes[i] = sizeSettingTable[i].size;
	}
	
	Header::init();
	TimeLine::init();
	ChannelEntry::init();
	ChannelEventEntry::init();
	Footer::init();
	
	this->selectedChannelEntry = NULL;
	
	channelsTableWidth = sizes[EPGPlus_channelentry_width];
	sliderWidth = sizes[EPGPlus_slider_width];
	
	horGap1Height = sizes[EPGPlus_horgap1_height];
	horGap2Height = sizes[EPGPlus_horgap2_height];
	verGap1Width = sizes[EPGPlus_vergap1_width];
	verGap2Width = sizes[EPGPlus_vergap2_width];
	
	int headerHeight = Header::getUsedHeight();
	int timeLineHeight = TimeLine::getUsedHeight();
	this->entryHeight = ChannelEntry::getUsedHeight();
	int footerHeight = Footer::getUsedHeight();
	
	this->maxNumberOfDisplayableEntries = (this->usableScreenHeight - headerHeight - timeLineHeight - horGap1Height - horGap2Height - footerHeight) / this->entryHeight;
	
	this->usableScreenHeight = headerHeight + timeLineHeight + horGap1Height + this->maxNumberOfDisplayableEntries * this->entryHeight + horGap2Height + footerHeight;	// recalc deltaY
	this->usableScreenX = (((g_settings.screen_EndX - g_settings.screen_StartX) - this->usableScreenWidth) / 2) + g_settings.screen_StartX;
	this->usableScreenY = (((g_settings.screen_EndY - g_settings.screen_StartY) - this->usableScreenHeight) / 2) + g_settings.screen_StartY;
	
	this->headerX = this->usableScreenX;
	this->headerY = this->usableScreenY;
	this->headerWidth = this->usableScreenWidth;
	
	this->timeLineX = this->usableScreenX;
	this->timeLineY = this->usableScreenY + headerHeight;
	this->timeLineWidth = this->usableScreenWidth;
	
	this->horGap1X = this->usableScreenX;
	this->horGap1Y = this->timeLineY + timeLineHeight;
	this->horGap1Width = this->usableScreenWidth;
	
	this->footerX = usableScreenX;
	this->footerY = this->usableScreenY + this->usableScreenHeight - footerHeight;
	this->footerWidth = this->usableScreenWidth;
	
	this->horGap2X = this->usableScreenX;
	this->horGap2Y = this->footerY - horGap2Height;
	this->horGap2Width = this->usableScreenWidth;
	
	this->channelsTableX = this->usableScreenX;
	this->channelsTableY = this->timeLineY + timeLineHeight + horGap1Height;
	this->channelsTableHeight = this->maxNumberOfDisplayableEntries * entryHeight;
	
	this->verGap1X = this->channelsTableX + channelsTableWidth;
	this->verGap1Y = this->channelsTableY;
	this->verGap1Height = this->channelsTableHeight;
	
	this->eventsTableX = this->channelsTableX + channelsTableWidth + verGap1Width;
	this->eventsTableY = this->channelsTableY;
	this->eventsTableWidth = this->usableScreenWidth - this->channelsTableWidth - this->sliderWidth - verGap1Width - verGap2Width;
	this->eventsTableHeight = this->channelsTableHeight;
	
	this->sliderX = this->usableScreenX + this->usableScreenWidth - this->sliderWidth;
	this->sliderY = this->eventsTableY;
	this->sliderHeight = this->channelsTableHeight;
	
	this->verGap2X = this->sliderX - verGap2Width;
	this->verGap2Y = this->channelsTableY;
	this->verGap2Height = this->channelsTableHeight;
	
	this->channelListStartIndex = 0;
	this->startTime = 0;
	this->duration = 2 * 60 * 60;
	
	this->refreshAll = false;
	
	this->header = new Header (this->frameBuffer, this->headerX, this->headerY, this->headerWidth);
	
	this->timeLine = new TimeLine (this->frameBuffer, this->timeLineX, this->timeLineY, this->timeLineWidth, this->eventsTableX, this->eventsTableWidth);
	
	this->footer = new Footer (this->frameBuffer, this->footerX, this->footerY, this->footerWidth);
	
	//
	this->refreshEpg = new MenuTargetRefreshEpg(this);
	this->addRecordTimer = new MenuTargetAddRecordTimer(this);
	this->addReminder = new MenuTargetAddReminder(this);
}

void EpgPlus::free()
{
	delete this->header;
	delete this->timeLine;
	delete this->footer;
}

int EpgPlus::exec(CChannelList * _channelList, int selectedChannelIndex, CBouquetList * _bouquetList)
{
	dprintf(DEBUG_NORMAL, "EpgPlus::exec:\n");

	this->channelList = _channelList;
	this->channelListStartIndex = int (selectedChannelIndex / maxNumberOfDisplayableEntries) * maxNumberOfDisplayableEntries;
	this->bouquetList = _bouquetList;
	
	int res = CMenuTarget::RETURN_REPAINT;

  	do {
		this->refreshAll = false;
		this->refreshFooterButtons = false;
		time_t currentTime = time (NULL);
		tm tmStartTime = *localtime (&currentTime);

		tmStartTime.tm_sec = 0;
		tmStartTime.tm_min = int (tmStartTime.tm_min / 15) * 15;

		this->startTime = mktime (&tmStartTime);
		this->selectedTime = this->startTime;
		this->firstStartTime = this->startTime;

		if (this->selectedChannelEntry != NULL) 
		{
	  		selectedChannelIndex = this->selectedChannelEntry->index;
		}

		neutrino_msg_t msg;
		neutrino_msg_data_t data;

		this->createChannelEntries(selectedChannelIndex);

		this->header->paint();

		this->footer->paintButtons(buttonLabels, sizeof (buttonLabels) / sizeof (button_label));

		this->paint();

		this->frameBuffer->blit();

		// add sec timer
		sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

		uint64_t timeoutEnd = CRCInput::calcTimeoutEnd (g_settings.timing_channellist);
		bool loop = true;

		while (loop) 
		{
	  		g_RCInput->getMsgAbsoluteTimeout (&msg, &data, &timeoutEnd);

	  		if (msg <= CRCInput::RC_MaxRC)
				timeoutEnd = CRCInput::calcTimeoutEnd (g_settings.timing_channellist);

	  		if (msg == CRCInput::RC_page_down) 
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				selectedChannelEntryIndex += this->maxNumberOfDisplayableEntries;

				if (selectedChannelEntryIndex > this->channelList->getSize () - 1)
			  		selectedChannelEntryIndex = 0;

				this->createChannelEntries (selectedChannelEntryIndex);

				this->paint ();
	  		}
	  		else if (msg == CRCInput::RC_page_up) 
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				selectedChannelEntryIndex -= this->maxNumberOfDisplayableEntries;

				if (selectedChannelEntryIndex < 0)
			  		selectedChannelEntryIndex = this->channelList->getSize () - 1;

				this->createChannelEntries (selectedChannelEntryIndex);

				this->paint ();
	  		} 
			else if (msg == (neutrino_msg_t) CRCInput::RC_red) 
			{
				this->addRecordTimer->exec(NULL, "");
	  		} 
	  		else if (msg == (neutrino_msg_t) CRCInput::RC_green) 
	  		{
	  			this->refreshEpg->exec(NULL, "");
	  		}
	  		else if (msg == (neutrino_msg_t) CRCInput::RC_yellow) 
	  		{
	  			this->addReminder->exec(NULL, "");
	  		}
			else if (msg == (neutrino_msg_t) CRCInput::RC_blue) 
			{
				TCChannelEventEntries::const_iterator It = this->getSelectedEvent ();

				if (It != this->selectedChannelEntry->channelEventEntries.end ()) 
				{
		  			if ((*It)->channelEvent.eventID != 0) 
					{
						this->hide();
						
						::getTMDBInfo((*It)->channelEvent.description.c_str());
						
						this->header->paint();
		  				this->footer->paintButtons(buttonLabels, sizeof (buttonLabels) / sizeof (button_label));
						this->paint();
					}
				}
	  		}
			else if (CRCInput::isNumeric (msg)) 
			{	//numeric zap
				this->hide ();
				this->channelList->numericZap(msg);

				int selectedChannelEntryIndex = this->channelList->getSelectedChannelIndex();
				if (selectedChannelEntryIndex < this->channelList->getSize()) 
				{
		  			this->hide ();
		  			this->createChannelEntries(selectedChannelEntryIndex);

		  			this->header->paint();
		  			this->footer->paintButtons(buttonLabels, sizeof (buttonLabels) / sizeof (button_label));
		  			this->paint();
				}
	  		} 
			else if (msg == CRCInput::RC_up) 
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				int prevSelectedChannelEntryIndex = selectedChannelEntryIndex;

				--selectedChannelEntryIndex;
				if (selectedChannelEntryIndex < 0) 
				{
		  			selectedChannelEntryIndex = this->channelList->getSize() - 1;
				}

				int oldChannelListStartIndex = this->channelListStartIndex;

				this->channelListStartIndex = (selectedChannelEntryIndex / this->maxNumberOfDisplayableEntries) * this->maxNumberOfDisplayableEntries;

				if (oldChannelListStartIndex != this->channelListStartIndex) 
				{
		  			this->createChannelEntries (selectedChannelEntryIndex);

		  			this->paint();
				} 
				else 
				{
		  			this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];

		  			this->paintChannelEntry (prevSelectedChannelEntryIndex - this->channelListStartIndex);
		  			this->paintChannelEntry (selectedChannelEntryIndex - this->channelListStartIndex);
				}
	  		} 
			else if (msg == CRCInput::RC_down) 
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				int prevSelectedChannelEntryIndex = this->selectedChannelEntry->index;

				selectedChannelEntryIndex = (selectedChannelEntryIndex + 1) % this->channelList->getSize ();

				int oldChannelListStartIndex = this->channelListStartIndex;
				this->channelListStartIndex = (selectedChannelEntryIndex / this->maxNumberOfDisplayableEntries) * this->maxNumberOfDisplayableEntries;

				if (oldChannelListStartIndex != this->channelListStartIndex) 
				{
		  			this->createChannelEntries (selectedChannelEntryIndex);

		  			this->paint ();
				} 
				else 
				{
		  			this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];

		  			this->paintChannelEntry (prevSelectedChannelEntryIndex - this->channelListStartIndex);
		  			this->paintChannelEntry (this->selectedChannelEntry->index - this->channelListStartIndex);
				}
	  		} 
			else if ((msg == CRCInput::RC_timeout) || (msg == CRCInput::RC_home) || (msg == CRCInput::RC_epg)) 
			{
				loop = false;
	  		}
	  		else if (msg == CRCInput::RC_left) 
			{
				TCChannelEventEntries::const_iterator It = this->getSelectedEvent();

				if ( (It != this->selectedChannelEntry->channelEventEntries.begin()) && (It != this->selectedChannelEntry->channelEventEntries.end ()) ) 
				{
			  		--It;
			  		this->selectedTime = (*It)->channelEvent.startTime + (*It)->channelEvent.duration / 2;
			  		if (this->selectedTime < this->startTime)
						this->selectedTime = this->startTime;

			  		this->selectedChannelEntry->paint (true, this->selectedTime);
				} 
				else 
				{
			  		if (this->startTime != this->firstStartTime) 
					{
						if (this->startTime - this->duration > this->firstStartTime) 
						{
				  			this->startTime -= this->duration;
						} 
						else 
						{
				  			this->startTime = this->firstStartTime;
						}

						this->selectedTime = this->startTime + this->duration - 1;	// select last event
						this->createChannelEntries (this->selectedChannelEntry->index);

						this->paint ();
			  		}
				}
	  		} 
			else if (msg == CRCInput::RC_right) 
			{
				TCChannelEventEntries::const_iterator It = this->getSelectedEvent ();

				if ((It != this->selectedChannelEntry->channelEventEntries.end () - 1)
						&& (It != this->selectedChannelEntry->channelEventEntries.end ())) 
				{
			  		++It;

			  		this->selectedTime = (*It)->channelEvent.startTime + (*It)->channelEvent.duration / 2;

			  		if (this->selectedTime > this->startTime + time_t (this->duration))
						this->selectedTime = this->startTime + this->duration;

			  		this->selectedChannelEntry->paint (true, this->selectedTime);
				} 
				else 
				{
			  		this->startTime += this->duration;
			  		this->createChannelEntries (this->selectedChannelEntry->index);

			  		this->selectedTime = this->startTime;
			  		this->createChannelEntries (this->selectedChannelEntry->index);

			  		this->paint ();
				}
	  		} 
			else if (msg == CRCInput::RC_ok) 
			{
				this->channelList->zapTo(this->selectedChannelEntry->index);
			} 
			else if ( msg == CRCInput::RC_info ) 
			{
				TCChannelEventEntries::const_iterator It = this->getSelectedEvent ();

				if (It != this->selectedChannelEntry->channelEventEntries.end ()) 
				{
		  			if ((*It)->channelEvent.eventID != 0) 
					{
						this->hide ();

						time_t _startTime = (*It)->channelEvent.startTime;
						res = g_EpgData->show (this->selectedChannelEntry->channel->channel_id, (*It)->channelEvent.eventID, &_startTime);

						if (res == CMenuTarget::RETURN_EXIT_ALL) 
						{
			  				loop = false;
						} 
						else 
						{
			  				g_RCInput->getMsg (&msg, &data, 0);

			  				if ((msg != CRCInput::RC_red) && (msg != CRCInput::RC_timeout)) 
							{
								// RC_red schlucken
								g_RCInput->postMsg (msg, data);
			  				}

			  				this->header->paint ();
			  				this->footer->paintButtons (buttonLabels, sizeof (buttonLabels) / sizeof (button_label));
			  				this->paint ();
						}
		  			}
				}
			}
			else if ( msg == CRCInput::RC_sat || msg == CRCInput::RC_favorites)
			{
				g_RCInput->postMsg (msg, 0);
				res = CMenuTarget::RETURN_EXIT_ALL;
				loop = false;
	 		}
			else if ( (msg == NeutrinoMessages::EVT_TIMER) && (data == sec_timer_id) )
			{
				this->header->refresh();
			} 
	 		else 
			{
				if (CNeutrinoApp::getInstance ()->handleMsg (msg, data) & messages_return::cancel_all) 
				{
		  			loop = false;
		  			res = CMenuTarget::RETURN_EXIT_ALL;
				}
	 		}

	 		if (this->refreshAll)
				loop = false;
	  		else if (this->refreshFooterButtons)
				this->footer->paintButtons (buttonLabels, sizeof (buttonLabels) / sizeof (button_label));
			
			this->frameBuffer->blit();		
		}

		this->hide ();

		//
		g_RCInput->killTimer(sec_timer_id);
		sec_timer_id = 0;

		for (TChannelEntries::iterator It = this->displayedChannelEntries.begin (); It != this->displayedChannelEntries.end (); It++) 
		{
	  		delete *It;
		}

  		this->displayedChannelEntries.clear ();
  	}
  	while (this->refreshAll);

  	return res;
}

EpgPlus::TCChannelEventEntries::const_iterator EpgPlus::getSelectedEvent() const
{
	for (TCChannelEventEntries::const_iterator It = this->selectedChannelEntry->channelEventEntries.begin ();
		It != this->selectedChannelEntry->channelEventEntries.end ();
		++It) 
	{
		if ((*It)->isSelected (this->selectedTime)) 
		{
			return It;
		}
	}

	return this->selectedChannelEntry->channelEventEntries.end ();
}

void EpgPlus::hide ()
{
  	this->frameBuffer->paintBackgroundBoxRel(this->usableScreenX, this->usableScreenY, this->usableScreenWidth, this->usableScreenHeight);	

	this->frameBuffer->blit();
}

void EpgPlus::paintChannelEntry (int position)
{
	ChannelEntry *channelEntry = this->displayedChannelEntries[position];
	
	bool currentChannelIsSelected = false;
	if (this->channelListStartIndex + position == this->selectedChannelEntry->index) 
	{
		currentChannelIsSelected = true;
	}

	channelEntry->paint (currentChannelIsSelected, this->selectedTime);
}

std::string EpgPlus::getTimeString (const time_t & time, const std::string & format)
{
	char tmpstr[256];
	struct tm *tmStartTime = localtime (&time);
	
	strftime (tmpstr, sizeof (tmpstr), format.c_str (), tmStartTime);

	return tmpstr;
}

void EpgPlus::paint()
{
	// clear
	this->frameBuffer->paintBackgroundBoxRel (this->channelsTableX, this->channelsTableY, this->usableScreenWidth, this->channelsTableHeight);
	
	this->frameBuffer->blit();
	
	// paint the gaps
	this->frameBuffer->paintBoxRel (this->horGap1X, this->horGap1Y, this->horGap1Width, horGap1Height, horGap1Color);
	this->frameBuffer->paintBoxRel (this->horGap2X, this->horGap2Y, this->horGap2Width, horGap2Height, horGap2Color);
	this->frameBuffer->paintBoxRel (this->verGap1X, this->verGap1Y, verGap1Width, this->verGap1Height, verGap1Color);
	this->frameBuffer->paintBoxRel (this->verGap2X, this->verGap2Y, verGap2Width, this->verGap2Height, verGap2Color);
	
	// paint the time line
	timeLine->paint (this->startTime, this->duration);
	
	// paint the channel entries
	for (int i = 0; i < (int) this->displayedChannelEntries.size (); ++i) 
	{
		this->paintChannelEntry (i);
	}
	
	// paint the time line grid
	this->timeLine->paintGrid ();
	
	// paint slider
	this->frameBuffer->paintBoxRel (this->sliderX, this->sliderY, this->sliderWidth, this->sliderHeight, COL_MENUCONTENT_PLUS_0);
	
	int tmp = ((this->channelList->getSize () - 1) / this->maxNumberOfDisplayableEntries) + 1;
	float sliderKnobHeight = (sliderHeight - 4) / tmp;
	int sliderKnobPosition = this->selectedChannelEntry == NULL ? 0 : (this->selectedChannelEntry->index / this->maxNumberOfDisplayableEntries);
	
	this->frameBuffer->paintBoxRel (this->sliderX + 2, this->sliderY + int (sliderKnobPosition * sliderKnobHeight), this->sliderWidth - 4, int (sliderKnobHeight) , COL_MENUCONTENT_PLUS_3);	
}

int CEPGplusHandler::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "CEPGplusHandler::exec:\n");

	int res = CMenuTarget::RETURN_REPAINT;

	EpgPlus* e;
	CChannelList* channelList;
	
	if (parent)
		parent->hide ();
	
	e = new EpgPlus;

	channelList = CNeutrinoApp::getInstance()->channelList;

	res = e->exec(channelList, channelList->getSelectedChannelIndex(), bouquetList);

	delete e;
	e = NULL;
	
	return res;
}

EpgPlus::MenuTargetAddReminder::MenuTargetAddReminder(EpgPlus * _epgPlus) 
{
  	this->epgPlus = _epgPlus;
}

int EpgPlus::MenuTargetAddReminder::exec(CMenuTarget */*parent*/, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "EpgPlus::MenuTargetAddReminder::exec:\n");

	TCChannelEventEntries::const_iterator It = this->epgPlus->getSelectedEvent ();
	
	if ((It != this->epgPlus->selectedChannelEntry->channelEventEntries.end ()) 
		&& (!(*It)->channelEvent.description.empty ())
		) 
	{
		if (CTimerd::getInstance()->isTimerdAvailable ()) 
		{
			CTimerd::getInstance()->addZaptoTimerEvent (this->epgPlus->selectedChannelEntry->channel->channel_id, (*It)->channelEvent.startTime, (*It)->channelEvent.startTime - ANNOUNCETIME, 0, (*It)->channelEvent.eventID, (*It)->channelEvent.startTime, 0);
	
			MessageBox(_("Schedule Event"), _("The event is scheduled.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO);	// UTF-8
		} 
		else
			printf ("timerd not available\n");
	}

	return CMenuTarget::RETURN_EXIT_ALL;
}

EpgPlus::MenuTargetAddRecordTimer::MenuTargetAddRecordTimer (EpgPlus * _epgPlus) 
{
  	this->epgPlus = _epgPlus;
}

int EpgPlus::MenuTargetAddRecordTimer::exec(CMenuTarget */*parent*/, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "EpgPlus::MenuTargetAddRecordTimer::exec:\n");

	TCChannelEventEntries::const_iterator It = this->epgPlus->getSelectedEvent ();
	
	if ((It != this->epgPlus->selectedChannelEntry->channelEventEntries.end ())
		&& (!(*It)->channelEvent.description.empty ())
		) 
	{
		if (CTimerd::getInstance()->isTimerdAvailable ()) 
		{
			CTimerd::getInstance()->addRecordTimerEvent (this->epgPlus->selectedChannelEntry->channel->channel_id, (*It)->channelEvent.startTime, (*It)->channelEvent.startTime + (*It)->channelEvent.duration, (*It)->channelEvent.eventID, (*It)->channelEvent.startTime, (*It)->channelEvent.startTime - (ANNOUNCETIME + 120) , TIMERD_APIDS_CONF, true);

			MessageBox(_("Schedule Record"), _("The event is flagged for record.\nThe box will power on and \nswitch to this channel at the given time."), mbrBack, mbBack, NEUTRINO_ICON_INFO);	// UTF-8
		} 
	}

	return CMenuTarget::RETURN_EXIT_ALL;
}

EpgPlus::MenuTargetRefreshEpg::MenuTargetRefreshEpg (EpgPlus * _epgPlus) 
{
  	this->epgPlus = _epgPlus;
}

int EpgPlus::MenuTargetRefreshEpg::exec(CMenuTarget */*parent*/, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_NORMAL, "EpgPlus::MenuTargetRefreshEpg::exec:\n");

	this->epgPlus->refreshAll = true;

	return CMenuTarget::RETURN_EXIT_ALL;
}

