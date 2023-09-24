/*
	Neutrino-GUI  -   DBoxII-Project

	$id: mediaplayer.cpp 2015.12.22 12:07:30 mohousch $
	
	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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

#include <global.h>
#include <neutrino2.h>

#include <stdio.h>

#include <system/debug.h>

#include <gui/mediaplayer.h>
#include <gui/audioplayer.h>
#include <gui/movieplayer.h>
#include <gui/pictureviewer.h>


int CMediaPlayerMenu::exec(CMenuTarget* parent, const std::string& actionKey)
{
	dprintf(DEBUG_NORMAL, "CMediaplayerMenu::exec: actionKey:%s\n", actionKey.c_str());
	
	int res = CMenuTarget::RETURN_REPAINT;
	
	if(parent)
		parent->hide();
	
	showMenu();
	
	return res;
}

// showmenu
void CMediaPlayerMenu::showMenu()
{
	dprintf(DEBUG_NORMAL, "CMediaPlayerMenu::showMenu:\n");

	//
	CWidget* widget = NULL;
	ClistBox* mediaPlayer = NULL;
	
	//
	widget = CNeutrinoApp::getInstance()->getWidget("mediaplayer");
	
	if (widget)
	{
		mediaPlayer = (ClistBox*)widget->getWidgetItem(CWidgetItem::WIDGETITEM_LISTBOX);
	}
	else
	{
		//
		widget = new CWidget(0, 0, MENU_WIDTH, MENU_HEIGHT);
		widget->name = "mediaplayer";
		widget->setMenuPosition(CWidget::MENU_POSITION_CENTER);
		
		//
		mediaPlayer = new ClistBox(widget->getWindowsPos().iX, widget->getWindowsPos().iY, widget->getWindowsPos().iWidth, widget->getWindowsPos().iHeight);
		
		mediaPlayer->setWidgetMode(ClistBox::MODE_MENU);
		mediaPlayer->setWidgetType(CMenuItem::TYPE_CLASSIC);
		mediaPlayer->enableShrinkMenu();
		
		//
		mediaPlayer->enablePaintHead();
		mediaPlayer->setTitle(_("Media Player"), NEUTRINO_ICON_MULTIMEDIA);
		mediaPlayer->enablePaintDate();
		
		//
		mediaPlayer->enablePaintFoot();
			
		const struct button_label btn = { NEUTRINO_ICON_INFO, " "};
			
		mediaPlayer->setFootButtons(&btn); 
		
		//
		widget->addWidgetItem(mediaPlayer);
	}

	//
	mediaPlayer->integratePlugins(CPlugins::I_TYPE_MULTIMEDIA);
	
	//
	widget->setTimeOut(g_settings.timing_menu);
	widget->exec(NULL, "");
	
	delete mediaPlayer;
	mediaPlayer = NULL;
	delete widget;
	widget = NULL;
}

