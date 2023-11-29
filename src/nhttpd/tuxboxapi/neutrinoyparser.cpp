//=============================================================================
// NHTTPD
// Neutrino yParser Extenstion
//=============================================================================
// c++
#include <string>
#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>
// system
#include <sys/types.h> //ntohs
#include <netinet/in.h> //ntohs
#include <inttypes.h> //ntohs
// yhttpd
#include "yhttpd.h"
#include "ytypes_globals.h"
#include "mod_yparser.h"

// nhttpd
#include "neutrinoyparser.h"
#include "neutrinoapi.h"

#include <global.h>
#include <neutrino2.h>
#include <system/settings.h>

/*zapit includes*/
#include <zapit/channel.h>
#include <zapit/bouquets.h>


////
extern tallchans allchans;
extern t_channel_id live_channel_id;

//=============================================================================
// Constructor & Destructor & Initialization
//=============================================================================
CNeutrinoYParser::CNeutrinoYParser(CNeutrinoAPI	*_NeutrinoAPI)
{
	NeutrinoAPI = _NeutrinoAPI;
}

//-------------------------------------------------------------------------
CNeutrinoYParser::~CNeutrinoYParser(void)
{
}

//=============================================================================
// Hooks!
//=============================================================================
//-----------------------------------------------------------------------------
// HOOK: response_hook Handler
// This is the main dispatcher for this module
//-----------------------------------------------------------------------------
THandleStatus CNeutrinoYParser::Hook_SendResponse(CyhookHandler *hh)
{
	hh->status = HANDLED_NONE;

	log_level_printf(4,"Neutrinoparser Hook Start url:%s\n",hh->UrlData["url"].c_str());
	init(hh);

	CNeutrinoYParser *yP = new CNeutrinoYParser(NeutrinoAPI);		// create a Session
	if (hh->UrlData["fileext"] == "yhtm" || hh->UrlData["fileext"] == "yjs" || hh->UrlData["fileext"] == "ysh") // yParser for y*-Files
		yP->ParseAndSendFile(hh);
	else if(hh->UrlData["path"] == "/y/")	// /y/<cgi> commands
	{
		yP->Execute(hh);
		if(hh->status == HANDLED_NOT_IMPLEMENTED)
			hh->status = HANDLED_NONE;	// y-calls can be implemented anywhere
	}

	delete yP;

	log_level_printf(4,"Neutrinoparser Hook Ende status:%d\n",(int)hh->status);
//	log_level_printf(5,"Neutrinoparser Hook Result:%s\n",hh->yresult.c_str());

	return hh->status;
}

//-----------------------------------------------------------------------------
// HOOK: webserver_readconfig_hook Handler
// This hook ist called from ReadCong (webserver)
//-----------------------------------------------------------------------------
THandleStatus CNeutrinoYParser::Hook_ReadConfig(CConfigFile *Config, CStringList &ConfigList)
{
//	ConfigList["ExtrasDocumentRoot"] = Config->getString("ExtrasDocRoot", EXTRASDOCUMENTROOT);
//	ConfigList["ExtrasDocumentURL"]	= Config->getString("ExtrasDocURL", EXTRASDOCUMENTURL);
	ConfigList["TUXBOX_LOGOS_URL"] = Config->getString("Tuxbox.LogosURL", TUXBOX_LOGOS_URL);

	if (Config->getInt32("configfile.version") < 3)
	{
		Config->setString("Tuxbox.LogosURL", Config->getString("ExtrasDocURL", EXTRASDOCUMENTURL) +"/logos");
		Config->setInt32("configfile.version", CONF_VERSION);
		Config->saveConfig(HTTPD_CONFIGFILE);
	}
	std::string logo = ConfigList["TUXBOX_LOGOS_URL"];
	return HANDLED_CONTINUE;
}

//=============================================================================
// y-func : Dispatching
//=============================================================================
const CNeutrinoYParser::TyFuncCall CNeutrinoYParser::yFuncCallList[]=
{
	{"mount-get-list", 				&CNeutrinoYParser::func_mount_get_list},
	{"mount-set-values", 				&CNeutrinoYParser::func_mount_set_values},
	{"get_bouquets_as_dropdown",			&CNeutrinoYParser::func_get_bouquets_as_dropdown},
	{"get_bouquets_as_templatelist",		&CNeutrinoYParser::func_get_bouquets_as_templatelist},
	{"get_actual_bouquet_number",			&CNeutrinoYParser::func_get_actual_bouquet_number},
	{"get_channels_as_dropdown",			&CNeutrinoYParser::func_get_channels_as_dropdown},
	{"get_bouquets_with_epg",			&CNeutrinoYParser::func_get_bouquets_with_epg},
	{"get_actual_channel_id",			&CNeutrinoYParser::func_get_actual_channel_id},
	{"get_mode",					&CNeutrinoYParser::func_get_mode},
	{"get_video_pids",				&CNeutrinoYParser::func_get_video_pids},
	{"get_audio_pid",				&CNeutrinoYParser::func_get_radio_pid},
	{"get_audio_pids_as_dropdown",			&CNeutrinoYParser::func_get_audio_pids_as_dropdown},
	{"umount_get_list",				&CNeutrinoYParser::func_unmount_get_list},
	{"get_partition_list",				&CNeutrinoYParser::func_get_partition_list},
	{"get_boxtype",					&CNeutrinoYParser::func_get_boxtype},
	{"get_current_stream_info",			&CNeutrinoYParser::func_get_current_stream_info},
	{"get_timer_list",				&CNeutrinoYParser::func_get_timer_list},
	{"set_timer_form",				&CNeutrinoYParser::func_set_timer_form},
	{"bouquet_editor_main",				&CNeutrinoYParser::func_bouquet_editor_main},
	{"set_bouquet_edit_form",			&CNeutrinoYParser::func_set_bouquet_edit_form},

};

//-------------------------------------------------------------------------
// y-func : dispatching and executing
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::YWeb_cgi_func(CyhookHandler *hh, std::string ycmd)
{
	std::string func="", para="", yresult="ycgi func not found";
	bool found = false;
	ySplitString(ycmd," ",func, para);

	log_level_printf(4,"NeutrinoYParser: func:(%s)\n", func.c_str());
	for(unsigned int i = 0;i < (sizeof(yFuncCallList)/sizeof(yFuncCallList[0])); i++)
		if (func == yFuncCallList[i].func_name)
		{
			yresult = (this->*yFuncCallList[i].pfunc)(hh, para);
			found = true;
			break;
		}
	log_level_printf(8,"NeutrinoYParser: func:(%s) para:(%s) Result:(%s)\n", func.c_str(), para.c_str(), yresult.c_str() );
	if(!found)
		yresult = CyParser::YWeb_cgi_func(hh, ycmd);
	return yresult;
}

//=============================================================================
// y-func : Functions for Neutrino
//=============================================================================
//-------------------------------------------------------------------------
// y-func : mount_get_list
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_mount_get_list(CyhookHandler *, std::string)
{
	CConfigFile *Config = new CConfigFile(',');
	std::string ysel, ytype, yip, ylocal_dir, ydir, ynr, yresult;
	int yitype;

	Config->loadConfig(NEUTRINO_CONFIGFILE);
	for(unsigned int i=0; i <= 7; i++)
	{
		ynr=itoa(i);
		ysel = ((i==0) ? "checked=\"checked\"" : "");
		yitype = Config->getInt32("network_nfs_type_"+ynr,0);
		ytype = ( (yitype==0) ? "NFS" :((yitype==1) ? "CIFS" : "FTPFS") );
		yip = Config->getString("network_nfs_ip_"+ynr,"");
		ydir = Config->getString("network_nfs_dir_"+ynr,"");
		ylocal_dir = Config->getString("network_nfs_local_dir_"+ynr,"");
		if(ydir != "")
			ydir="("+ydir+")";

		yresult += string_printf("<input type='radio' name='R1' value='%d' %s />%d %s - %s %s %s<br/>",
			i,ysel.c_str(),i,ytype.c_str(),yip.c_str(),ylocal_dir.c_str(), ydir.c_str());
	}
	delete Config;
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : mount_set_values
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_mount_set_values(CyhookHandler *hh, std::string)
{
	CConfigFile *Config = new CConfigFile(',');
	std::string ynr, yresult;

	Config->loadConfig(NEUTRINO_CONFIGFILE);
	ynr = hh->ParamList["nr"];
	Config->setString("network_nfs_type_"+ynr,hh->ParamList["type"]);
	Config->setString("network_nfs_ip_"+ynr,hh->ParamList["ip"]);
	Config->setString("network_nfs_dir_"+ynr,hh->ParamList["dir"]);
	Config->setString("network_nfs_local_dir_"+ynr,hh->ParamList["localdir"]);
	Config->setString("network_nfs_mac_"+ynr,hh->ParamList["mac"]);
	Config->setString("network_nfs_mount_options1_"+ynr,hh->ParamList["opt1"]);
	Config->setString("network_nfs_mount_options2_"+ynr,hh->ParamList["opt2"]);
	Config->setString("network_nfs_automount_"+ynr,hh->ParamList["automount"]);
	Config->setString("network_nfs_username_"+ynr,hh->ParamList["username"]);
	Config->setString("network_nfs_password_"+ynr,hh->ParamList["password"]);
	Config->saveConfig(NEUTRINO_CONFIGFILE);

	delete Config;
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_bouquets_as_dropdown [<bouquet>] <doshowhidden>
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_bouquets_as_dropdown(CyhookHandler *, std::string para)
{
	std::string ynr, yresult, sel, nr_str, do_show_hidden;
	int nr=1;

	ySplitString(para," ",nr_str, do_show_hidden);
	if(nr_str != "")
		nr = atoi(nr_str.c_str());

	int mode = CZapit::getInstance()->getMode();
	for (int i = 0; i < (int) CZapit::getInstance()->Bouquets.size(); i++) 
	{
		ZapitChannelList * channels = mode == CZapit::MODE_RADIO ? &CZapit::getInstance()->Bouquets[i]->radioChannels : &CZapit::getInstance()->Bouquets[i]->tvChannels;
		sel=(nr==(i+1)) ? "selected=\"selected\"" : "";
		if(!channels->empty() && (!CZapit::getInstance()->Bouquets[i]->bHidden || do_show_hidden == "true"))
			yresult += string_printf("<option value=%u %s>%s</option>\n", i + 1, sel.c_str(),
				(encodeString(std::string(CZapit::getInstance()->Bouquets[i]->bFav ? _("Favorites") :CZapit::getInstance()->Bouquets[i]->Name.c_str()))).c_str());
				
			//yresult += string_printf("<option value=%u %s>%s</option>\n", i + 1, sel.c_str(), (encodeString(std::string(CZapit::getInstance()->Bouquets[i]->Name.c_str()))).c_str());
	}
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_bouquets_as_templatelist <tempalte> [~<do_show_hidden>]
// TODO: select actual Bouquet
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_bouquets_as_templatelist(CyhookHandler *, std::string para)
{
	std::string yresult, ytemplate, do_show_hidden;

	ySplitString(para,"~",ytemplate, do_show_hidden);
	//ytemplate += "\n"; //FIXME add newline to printf
	for (int i = 0; i < (int) CZapit::getInstance()->Bouquets.size(); i++) 
	{
		if(!CZapit::getInstance()->Bouquets[i]->bHidden || do_show_hidden == "true") 
		{
			yresult += string_printf(ytemplate.c_str(), i + 1, CZapit::getInstance()->Bouquets[i]->bFav ? _("Favorites") : CZapit::getInstance()->Bouquets[i]->Name.c_str());
			yresult += "\r\n";
		}
	}
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_actual_bouquet_number
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_actual_bouquet_number(CyhookHandler *, std::string)
{
	int actual=0;
	for (int i = 0; i < (int) CZapit::getInstance()->Bouquets.size(); i++) 
	{
		if(CZapit::getInstance()->existsChannelInBouquet(i, live_channel_id)) 
		{
			actual=i+1;
			break;
		}
	}
	return std::string(itoa(actual));
}

//-------------------------------------------------------------------------
// y-func : get_channel_dropdown [<bouquet_nr> [<channel_id>]]
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_channels_as_dropdown(CyhookHandler *, std::string para)
{
	std::string abouquet, achannel_id, yresult, sel, sid;

	int bnumber = 1;
	int mode = CZapit::getInstance()->getMode();

	ySplitString(para," ",abouquet, achannel_id);
	if(abouquet != "")
		bnumber = atoi(abouquet.c_str());
	if(bnumber > 0) 
	{
		bnumber--;
		ZapitChannelList channels;
		channels = mode == CZapit::MODE_RADIO ? CZapit::getInstance()->Bouquets[bnumber]->radioChannels : CZapit::getInstance()->Bouquets[bnumber]->tvChannels;
		for(int j = 0; j < (int) channels.size(); j++) 
		{
			CEPGData epg;
			CZapitChannel * channel = channels[j];
			char buf[100],id[20];
			sprintf(id, "%llx", channel->channel_id);
			std::string _sid = std::string(id);
			sel = (_sid == achannel_id) ? "selected=\"selected\"" : "";
			CSectionsd::getInstance()->getActualEPGServiceKey(channel->channel_id&0xFFFFFFFFFFFFULL, &epg);
			sprintf(buf, "<option value=""%llx"" %s>%.20s - %.30s</option>\n", channel->channel_id, sel.c_str(), channel->getName().c_str(),epg.title.c_str());
			yresult += buf;
		}
	}
	return yresult;
}

//-------------------------------------------------------------------------
// TODO: clean up code
// use templates?
//-------------------------------------------------------------------------
std::string CNeutrinoYParser::func_get_bouquets_with_epg(CyhookHandler *hh, std::string para)
{
	int BouquetNr = 0;
	std::string abnumber, tmp,yresult;
	ZapitChannelList channels;
	int num;
	int mode = CZapit::getInstance()->getMode();

	ySplitString(para," ",abnumber, tmp);
	if(abnumber != "")
		BouquetNr = atoi(abnumber.c_str());
	if (BouquetNr > 0) 
	{
		BouquetNr--;
		channels = mode == CZapit::MODE_RADIO ? CZapit::getInstance()->Bouquets[BouquetNr]->radioChannels : CZapit::getInstance()->Bouquets[BouquetNr]->tvChannels;
		num = 1 + (mode == CZapit::MODE_RADIO ? CZapit::getInstance()->radioChannelsBegin().getNrofFirstChannelofBouquet(BouquetNr) : CZapit::getInstance()->tvChannelsBegin().getNrofFirstChannelofBouquet(BouquetNr)) ;
	} 
	else 
	{
		CZapit::ChannelIterator cit = mode == CZapit::MODE_RADIO ? CZapit::getInstance()->radioChannelsBegin() : CZapit::getInstance()->tvChannelsBegin();
		for (; !(cit.EndOfChannels()); cit++)
			channels.push_back(*cit);
		num = 1;
	}
	NeutrinoAPI->GetChannelEvents();

	int i = 1;
	char classname;
	t_channel_id current_channel = live_channel_id;
	int prozent;
	CSectionsd::CurrentNextInfo currentNextInfo;
	std::string timestr;
	bool have_logos = false;

	if(hh->WebserverConfigList["Tuxbox.LogosURL"] != "")
		have_logos = true;
	for(int j = 0; j < (int) channels.size(); j++)
	{
		CZapitChannel * channel = channels[j];
		CChannelEvent *event;
		event = NeutrinoAPI->ChannelListEvents[channel->channel_id];

		classname = (i++ & 1) ? 'a' : 'b';
		if (channel->channel_id == current_channel)
			classname = 'c';

		std::string bouquetstr = (BouquetNr >= 0) ? ("&amp;bouquet=" + itoa(BouquetNr)) : "";
		yresult += "<tr>";

		if(have_logos)
			yresult += string_printf("<td class=\"%c\" width=\"44\" rowspan=\"2\"><a href=\"javascript:do_zap('"
					"%llx"
					"')\"><img class=\"channel_logo\" src=\"%s\"/></a></td>", classname, channel->channel_id,
					(NeutrinoAPI->getLogoFile(hh->WebserverConfigList["Tuxbox.LogosURL"], channel->channel_id)).c_str());

		/* timer slider */
		if(event && event->duration > 0)
			prozent = 100 * (time(NULL) - event->startTime) / event->duration;
		else
			prozent = 100;
		yresult += string_printf("<td class=\"%c\"><table border=\"0\" cellspacing=\"0\" cellpadding=\"3\"><tr><td>\n"
				"\t<table border=\"0\" rules=\"none\" class=\"cslider_table\" width=\"32\">"
				"<tr>"
				"<td class=\"cslider_used\" width=\"%d\"></td>"
				"<td class=\"cslider_free\" width=\"%d\"></td>"
				"</tr>"
				"</table>\n</td>\n"
				, classname
				, (prozent / 10) * 3
				, (10 - (prozent / 10))*3
			);

		/* channel name and buttons */
		yresult += string_printf("<td>\n%s<a class=\"clist\" href=\"javascript:do_zap('"
				"%llx"
				"')\">&nbsp;%d. %s%s</a>&nbsp;<a href=\"javascript:do_epg('"
				"%llx"
				"','"
				"%llx"
				"')\">%s</a>\n",
				((channel->channel_id == current_channel) ? "<a name=\"akt\"></a>" : " "),
				channel->channel_id,
				num + j /*channel->nr*/,
				channel->getName().c_str(),
				(channel->getServiceType() == ST_NVOD_REFERENCE_SERVICE) ? " (NVOD)" : "",
				channel->channel_id,
				channel->channel_id & 0xFFFFFFFFFFFFULL,
				((NeutrinoAPI->ChannelListEvents[channel->channel_id]) ? "<img src=\"/images/elist.gif\" alt=\"Program preview\" style=\"border: 0px\" />" : ""));

		if (channel->channel_id == current_channel)
			yresult += string_printf("\n&nbsp;&nbsp;<a href=\"javascript:do_streaminfo()\"><img src=\"/images/streaminfo.png\" alt=\"Streaminfo\" style=\"border: 0px\" /></a>");

		yresult += string_printf("</td></tr></table>\n</td>\n</tr>\n");

		if (channel->getServiceType() == ST_NVOD_REFERENCE_SERVICE)
		{
			CSectionsd::NVODTimesList nvod_list;
			if (CSectionsd::getInstance()->getNVODTimesServiceKey(channel->channel_id&0xFFFFFFFFFFFFULL, nvod_list))
			{
				CZapit::subServiceList subServiceList;

				for (CSectionsd::NVODTimesList::iterator ni = nvod_list.begin(); ni != nvod_list.end(); ni++)
				{
					CZapit::commandAddSubServices cmd;
					CEPGData epg;

					// Byte Sequence by ntohs
					cmd.original_network_id = ntohs(ni->original_network_id);
					cmd.service_id = ntohs(ni->service_id);
					cmd.transport_stream_id = ntohs(ni->transport_stream_id);

					t_channel_id channel_id = create_channel_id(cmd.service_id, cmd.original_network_id, cmd.transport_stream_id);

					timestr = timeString(ni->zeit.startzeit); // FIXME: time is wrong (at least on little endian)!
					CSectionsd::getInstance()->getActualEPGServiceKey(channel_id&0xFFFFFFFFFFFFULL, &epg); // FIXME: der scheissendreck geht nit!!!
					yresult += string_printf("<tr>\n<td align=\"left\" style=\"width: 31px\" class=\"%cepg\">&nbsp;</td>", classname);
					yresult += string_printf("<td class=\"%cepg\">%s&nbsp;", classname, timestr.c_str());
					yresult += string_printf("%s<a href=\"javascript:do_zap('"
							"%llx"
							")'\">%04x:%04x:%04x %s</a>", // FIXME: get name
							(channel_id == current_channel) ? "<a name=\"akt\"></a>" : " ",
							channel_id,
							bouquetstr.c_str(),
							cmd.transport_stream_id,
							cmd.original_network_id,
							cmd.service_id,
							epg.title.c_str());
					yresult += string_printf("</td>\n</tr>");

					subServiceList.push_back(cmd);
				}

				if (!(subServiceList.empty()))
					CZapit::getInstance()->setSubServices(subServiceList);
			}
		}
		else if ((event = NeutrinoAPI->ChannelListEvents[channel->channel_id]))
		{
			bool has_current_next = true;
			CSectionsd::getInstance()->getCurrentNextServiceKey(channel->channel_id&0xFFFFFFFFFFFFULL, currentNextInfo);
			timestr = timeString(event->startTime);

			yresult += string_printf("<tr><td class=\"%cepg\">",classname);
			yresult += string_printf("%s&nbsp;%s&nbsp;"
					"<span style=\"font-size: 8pt; white-space: nowrap\">(%ld von %d min, %d%%)</span>"
					, timestr.c_str()
					, event->description.c_str()
					, (time(NULL) - event->startTime)/60
					, event->duration / 60,prozent);

			if ((has_current_next) && (currentNextInfo.flags & CSectionsd::epgflags::has_next)) 
			{
				timestr = timeString(currentNextInfo.next_zeit.startzeit);
				yresult += string_printf("<br />%s&nbsp;%s", timestr.c_str(), currentNextInfo.next_name.c_str());
			}

			yresult += string_printf("</td></tr>\n");
		}
		else
		yresult += string_printf("<tr style=\"height: 2px\"><td></td></tr>\n");
	}
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_actual_channel_id
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_actual_channel_id(CyhookHandler *, std::string)
{
	return string_printf("%llx", live_channel_id);
}

//-------------------------------------------------------------------------
// y-func : get_mode (returns tv|radio|unknown)
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_mode(CyhookHandler *, std::string)
{
	std::string yresult;
	int mode = CZapit::getInstance()->getMode();

	if ( mode == CZapit::MODE_TV)
		yresult = "tv";
	else if ( mode == CZapit::MODE_RADIO)
		yresult = "radio";
	else
		yresult = "unknown";
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get_video_pids (para=audio channel, returns: 0x0000,0x0000,0x0000)
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_video_pids(CyhookHandler *, std::string para)
{
	CZapit::responseGetPIDs pids;
	int apid=0,apid_no=0,apid_idx=0;
	pids.PIDs.vpid=0;

	if(para != "")
		apid_no = atoi(para.c_str());
	CZapit::getInstance()->getCurrentPIDS(pids);

	if( apid_no < (int)pids.APIDs.size())
		apid_idx=apid_no;
	if(!pids.APIDs.empty())
		apid = pids.APIDs[apid_idx].pid;
	return string_printf("0x%04x,0x%04x,0x%04x",pids.PIDs.pmtpid,pids.PIDs.vpid,apid);
}

//-------------------------------------------------------------------------
// y-func : get_radio_pids (returns: 0x0000)
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_radio_pid(CyhookHandler *, std::string)
{
	CZapit::responseGetPIDs pids;
	int apid = 0;

	CZapit::getInstance()->getCurrentPIDS(pids);
	if(!pids.APIDs.empty())
		apid = pids.APIDs[0].pid;

	return string_printf("0x%04x",apid);
}

//-------------------------------------------------------------------------
// y-func : get_audio_pids_as_dropdown (from controlapi)
// prara: [apid] option value = apid-Value. Default apid-Index
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_audio_pids_as_dropdown(CyhookHandler *, std::string para)
{
	std::string yresult;
	bool idx_as_id=true;

	if(para == "apid")
		idx_as_id=false;
		
	bool eit_not_ok=true;
	CZapit::responseGetPIDs pids;

	CSectionsd::ComponentTagList tags;
	pids.PIDs.vpid = 0;
	CZapit::getInstance()->getCurrentPIDS(pids);

	t_channel_id current_channel = live_channel_id; //CZapit::getInstance()->getCurrentServiceID();
	CSectionsd::CurrentNextInfo currentNextInfo;
	CSectionsd::getInstance()->getCurrentNextServiceKey(current_channel&0xFFFFFFFFFFFFULL, currentNextInfo);
	if (CSectionsd::getInstance()->getComponentTagsUniqueKey(currentNextInfo.current_uniqueKey,tags))
	{
		for (unsigned int i=0; i< tags.size(); i++)
		{
			for (unsigned short j=0; j< pids.APIDs.size(); j++)
			{
				if ( pids.APIDs[j].component_tag == tags[i].componentTag )
				{
 					if(!tags[i].component.empty())
					{
						if(!(isalnum(tags[i].component[0])))
							tags[i].component=tags[i].component.substr(1,tags[i].component.length()-1);
						yresult += string_printf("<option value=%05u>%s</option>\r\n",idx_as_id ? j : pids.APIDs[j].pid,encodeString(tags[i].component).c_str());
					}
					else
					{
						strcpy( pids.APIDs[j].desc, getISO639Description( pids.APIDs[j].desc ) );

			 			yresult += string_printf("<option value=%05u>%s %s</option>\r\n",idx_as_id ? j : pids.APIDs[j].pid,encodeString(std::string(pids.APIDs[j].desc)).c_str(),pids.APIDs[j].is_ac3 ? " (AC3)": " ");
					}
					eit_not_ok=false;
					break;
				}
			}
		}
	}
	if(eit_not_ok)
	{
		unsigned short i = 0;
		for (CZapit::APIDList::iterator it = pids.APIDs.begin(); it!=pids.APIDs.end(); it++)
		{
			strcpy( pids.APIDs[i].desc, getISO639Description( pids.APIDs[i].desc ) );

 			yresult += string_printf("<option value=%05u>%s %s</option>\r\n",idx_as_id ? i : it->pid,pids.APIDs[i].desc,pids.APIDs[i].is_ac3 ? " (AC3)": " ");
			i++;
		}
	}

	if(pids.APIDs.empty())
		yresult = "00000"; // shouldnt happen, but print at least one apid
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : build umount list
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_unmount_get_list(CyhookHandler *, std::string)
{
	std::string ysel, ymount, ylocal_dir, yfstype, ynr, yresult, mounts;

	std::ifstream in;
	in.open("/proc/mounts", std::ifstream::in);
	int j=0;
	while(in.good() && (j<8))
	{
		yfstype="";
		in >> ymount >> ylocal_dir >> yfstype;
		in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		yfstype = trim(yfstype);
		if( (yfstype == "nfs") << (yfstype == "ftp") || (yfstype == "lufsd") )
		{
			mounts=ylocal_dir +" on "+ ymount + " ("+yfstype+")";
			ysel = ((j==0) ? "checked=\"checked\"" : "");
			yresult += string_printf("<input type='radio' name='R1' value='%s' %s />%d %.120s<br/>",
				ylocal_dir.c_str(),ysel.c_str(),j,mounts.c_str());
			j++;
		}
	}
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : build partition list
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_partition_list(CyhookHandler *, std::string)
{
	std::string ysel, ymtd, yname, dummy, yresult;
	char ytmp[200];

	std::ifstream in;
	in.open("/proc/mtd", std::ifstream::in);
	int j=0;
	while(in.good() && (j<8))
	{
		ymtd="";
		in >> ymtd >> dummy >> dummy; //format:  mtd# start end "name  "
		in.getline(ytmp, 200); // Rest of line is the mtd description
		yname = ytmp;
		if((j>0) && (ymtd != ""))// iggnore first line
		{
			ysel = ((j==1) ? "checked=\"checked\"" : "");
			yresult += string_printf("<input type='radio' name='R1' value='%d' %s title='%s' />%d %s<br/>",
				j-1,ysel.c_str(),yname.c_str(),j-1,yname.c_str());
		}
		j++;
	}
	return yresult;
}

//-------------------------------------------------------------------------
// y-func : get boxtypetext (Nokia, Philips, Sagem)
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_boxtype(CyhookHandler *, std::string)
{
	//return "generic";//FIXME
#if defined (BOXMODEL_ATEVIO700)
	return "atevio700";
#elif defined (BOXMODEL_ATEVIO7000)
	return "atevio7000";
#elif defined (BOXMODEL_ATEVIO7500)
	return "atevio7500";
#elif defined (BOXMODEL_ATEVIO7600)
	return "atevio7600";
#elif defined (BOXMODEL_AZBOXHD)
	return "azboxhd";
#elif defined (BOXMODEL_AZBOXME)
	return "azboxme";
#elif defined (BOXMODEL_AZBOXMINIME)
	return "azboxminime";
#elif defined (BOXMODEL_DM500)
	return "dm500";
#elif defined (BOXMODEL_DM500HD)
	return "dm500hd";
#elif defined (BOXMODEL_DM500PLUS)
	return "dm500plus";
#elif defined (BOXMODEL_DM56x0)
	return "dm56x0";
#elif defined (BOXMODEL_DM600PVR)
	return "dm600pvr";
#elif defined (BOXMODEL_DM7000)
	return "dm7000";
#elif defined (BOXMODEL_DM7000HD)
	return "dm7000hd";
#elif defined (BOXMODEL_DM8000HD)
	return "dm8000";
#elif defined (BOXMODEL_DM800HD)
	return "dm800hd";
#elif defined (BOXMODEL_DM800SE)
	return "dm800se";
#elif defined (BOXMODEL_E3HD)
	return "e3hd";
#elif defined (BOXMODEL_ET4X00)
	return "et4x00";
#elif defined (BOXMODEL_ET5X00)
	return "et5x00";
#elif defined (BOXMODEL_ET6X00)
	return "et6x00";
#elif defined (BOXMODEL_ET9X00)
	return "et9x00";
#elif defined (BOXMODEL_GB800SE)
	return "gb800se";
#elif defined (BOXMODEL_GB800SEPLUS)
	return "gb800seplus";
#elif defined (BOXMODEL_GB800SOLO)
	return "gb800solo";
#elif defined (BOXMODEL_GB800UE)
	return "gb800ue";
#elif defined (BOXMODEL_GB800UEPLUS)
	return "gb800ueplus";
#elif defined (BOXMODEL_GBQUAD)
	return "gbquad";
#elif defined (BOXMODEL_INIHDE)
	return "inihde";
#elif defined (BOXMODEL_INIHDP)
	return "inihdp";
#elif defined (BOXMODEL_IQONIOS100HD)
	return "iqonios100hd";
#elif defined (BOXMODEL_IQONIOS300HD)
	return "iqonios300hd";
#elif defined (BOXMODEL_IXUSSONE)
	return "ixussone";
#elif defined (BOXMODEL_IXUSSZERO)
	return "ixusszero";
#elif defined (BOXMODEL_MEDIABOX)
	return "mediabox";
#elif defined (BOXMODEL_ODINM6)
	return "odinm6";
#elif defined (BOXMODEL_ODINM7)
	return "odinm7";
#elif defined (BOXMODEL_ODINM9)
	return "odinm9";
#elif defined (BOXMODEL_OPTIMUSSOS1)
	return "optimuss0s1";
#elif defined (BOXMODEL_OPTIMUSSOS2)
	return "optimuss0s2";
#elif defined (BOXMODEL_TM2T)
	return "tm2t";
#elif defined (BOXMODEL_TMNANO)
	return "tmnano";
#elif defined (BOXMODEL_TMSINGLE)
	return "tmsingle";
#elif defined (BOXMODEL_TMTWIN)
	return "tmtwin";
#elif defined (BOXMODEL_VENTONHDE)
	return "ventonhde";
#elif defined (BOXMODEL_VENTONHDX)
	return "ventonhdx";
#elif defined (BOXMODEL_VUDUO)
	return "vuduo";
#elif defined (BOXMODEL_VUDUO2)
	return "vuduo2";
#elif defined (BOXMODEL_VUSOLO)
	return "vusolo";
#elif defined (BOXMODEL_VUSOLO2)
	return "vusolo2";
#elif defined (BOXMODEL_VUULTIMO)
	return "vuultimo";
#elif defined (BOXMODEL_VUUNO)
	return "vuuno";
#else	
	return "generic";
#endif
}
//-------------------------------------------------------------------------
// y-func : get stream info
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_current_stream_info(CyhookHandler *hh, std::string)
{
	CZapit::CServiceInfo serviceinfo;

	serviceinfo = CZapit::getInstance()->getCurrentServiceInfo();
	hh->ParamList["onid"] = itoh(serviceinfo.onid);
	hh->ParamList["sid"] = itoh(serviceinfo.sid);
	hh->ParamList["tsid"] = itoh(serviceinfo.tsid);
	hh->ParamList["vpid"] = itoh(serviceinfo.vpid);
	hh->ParamList["apid"] = itoh(serviceinfo.apid);
	hh->ParamList["vtxtpid"] = (serviceinfo.vtxtpid != 0)?itoh(serviceinfo.vtxtpid):"not available";
	hh->ParamList["pmtpid"] = (serviceinfo.pmtpid != 0)?itoh(serviceinfo.pmtpid):"not available";
	hh->ParamList["tsfrequency"] = string_printf("%d.%d MHz", serviceinfo.tsfrequency/1000, serviceinfo.tsfrequency%1000);
	hh->ParamList["polarisation"] = serviceinfo.polarisation==1?"h":"v";
	hh->ParamList["ServiceName"] = NeutrinoAPI->GetServiceName(live_channel_id);//CZapit::getInstance()->getCurrentServiceID());
	hh->ParamList["VideoFormat"] = NeutrinoAPI->getVideoResolutionAsString();
	hh->ParamList["BitRate"] = NeutrinoAPI->getVideoFramerateAsString();
	hh->ParamList["AspectRatio"] = NeutrinoAPI->getVideoAspectRatioAsString();
	hh->ParamList["FPS"] = NeutrinoAPI->getVideoFramerateAsString();
//	hh->ParamList["AudioType"] = NeutrinoAPI->getAudioInfoAsString();
//	hh->ParamList["Crypt"] = NeutrinoAPI->getCryptInfoAsString();
	return "";
}

//-------------------------------------------------------------------------
// Template 1:classname, 2:zAlarmTime, 3: zStopTime, 4:zRep, 5:zRepCouunt
//		6:zType, 7:sAddData, 8:timer->eventID, 9:timer->eventID
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_get_timer_list(CyhookHandler *, std::string para)
{
	std::string yresult;
	CTimerd::TimerList timerlist;				// List of bouquets

	timerlist.clear();
	CTimerd::getInstance()->getTimerList(timerlist);
	sort(timerlist.begin(), timerlist.end());

	CZapit::BouquetChannelList channellist_tv;
	CZapit::BouquetChannelList channellist_radio;
	channellist_tv.clear();
	channellist_radio.clear();

	int i = 1;
	char classname= 'a';
	CTimerd::TimerList::iterator timer = timerlist.begin();
	for(; timer != timerlist.end();timer++)
	{
		classname = (i++&1)?'a':'b';

		// build alarm/stoptime
		char zAlarmTime[25] = {0};
		struct tm *alarmTime = localtime(&(timer->alarmTime));
		strftime(zAlarmTime,20,"%d.%m. %H:%M",alarmTime);

		char zAnnounceTime[25] = {0};
		struct tm *announceTime = localtime(&(timer->announceTime));
		strftime(zAnnounceTime,20,"%d.%m. %H:%M",announceTime);

		char zStopTime[25] = {0};
		if(timer->stopTime > 0)
		{
			struct tm *stopTime = localtime(&(timer->stopTime));
			strftime(zStopTime,20,"%d.%m. %H:%M",stopTime);
		}
		// repeat
		std::string zRep = NeutrinoAPI->timerEventRepeat2Str(timer->eventRepeat);
		std::string zRepCount;
		if (timer->eventRepeat == CTimerd::TIMERREPEAT_ONCE)
			zRepCount = "-";
		else
			zRepCount = (timer->repeatCount == 0) ? "&#x221E;" : string_printf("%dx",timer->repeatCount);
		// timer type
		std::string zType = NeutrinoAPI->timerEventType2Str(timer->eventType);
		// Add Data
		std::string sAddData="";
		switch(timer->eventType)
		{
			case CTimerd::TIMER_NEXTPROGRAM :
			case CTimerd::TIMER_ZAPTO :
			case CTimerd::TIMER_RECORD :
			{
				sAddData = NeutrinoAPI->GetServiceName(timer->channel_id);
				if (sAddData.empty())
					sAddData = CZapit::getInstance()->isChannelTVChannel(timer->channel_id) ? "Unknown TV-Channel" : "Unknown Radio-Channel";

				if( timer->apids != TIMERD_APIDS_CONF)
				{
					std::string separator = "";
					sAddData += '(';
					if( timer->apids & TIMERD_APIDS_STD )
					{
						sAddData += "STD";
						separator = "/";
					}
					if( timer->apids & TIMERD_APIDS_ALT )
					{
						sAddData += separator;
						sAddData += "ALT";
						separator = "/";
					}
					if( timer->apids & TIMERD_APIDS_AC3 )
					{
						sAddData += separator;
						sAddData += "AC3";
						separator = "/";
					}
					sAddData += ')';
				}
				if(timer->epgID!=0)
				{
					CEPGData epgdata;
					if (CSectionsd::getInstance()->getEPGid(timer->epgID, timer->epg_starttime, &epgdata))
						sAddData+="<br/>" + epgdata.title;
					else
						sAddData+=std::string("<br/>")+timer->epgTitle;
				}
				else
					sAddData+=std::string("<br/>")+timer->epgTitle;
			}
			break;
			case CTimerd::TIMER_STANDBY :
			{
				sAddData = "Standby: ";
				if(timer->standby_on)
					sAddData+= "An";
				else
					sAddData+="Aus";
			}
			break;
			case CTimerd::TIMER_REMIND :
				sAddData = std::string(timer->message).substr(0,20);
				break;
			case CTimerd::TIMER_EXEC_PLUGIN :
				sAddData = std::string(timer->pluginName);
				break;

			default:{}
		}
		yresult += string_printf(para.c_str(), classname, zAlarmTime, zStopTime, zRep.c_str(), zRepCount.c_str(),
					zType.c_str(), sAddData.c_str(),timer->eventID,timer->eventID);
	}
	classname = (i++&1)?'a':'b';

	return yresult;
}

/*------------------------------------------------------------------------------
	CTimerd::TIMER_RECORD = 5
	CTimerd::TIMER_STANDBY = 4
	CTimerd::TIMER_NEXTPROGRAM = 2
	CTimerd::TIMER_ZAPTO = 3
	CTimerd::TIMER_REMIND = 6
	CTimerd::TIMER_EXEC_PLUGIN = 8
	CTimerd::TIMERREPEAT_WEEKDAYS = 256
	CTimerd::TIMERREPEAT_ONCE = 0
 */

// para ::= <type=new|modify> [timerid]
std::string  CNeutrinoYParser::func_set_timer_form(CyhookHandler *hh, std::string para)
{
	unsigned timerId = 0;
	std::string cmd, stimerid;
	CTimerd::responseGetTimer timer;             // Timer
	time_t now_t = time(NULL);
	ySplitString(para, " ", cmd, stimerid);
	
	if(cmd != "new")
	{
		// init timerid
		if(stimerid != "")
			timerId = (unsigned)atoi(stimerid.c_str());

		CTimerd::getInstance()->getTimer(timer, timerId);
		std::string zType = NeutrinoAPI->timerEventType2Str(timer.eventType);

		hh->ParamList["timerId"] = itoa(timerId);
		hh->ParamList["zType"] = zType;
	}
	
	// Alarm/StopTime
	struct tm *alarmTime = (cmd == "new") ? localtime(&now_t) : localtime(&(timer.alarmTime));

	hh->ParamList["alarm_mday"] = string_printf("%02d", alarmTime->tm_mday);
	hh->ParamList["alarm_mon"]  = string_printf("%02d", alarmTime->tm_mon +1);
	hh->ParamList["alarm_year"] = string_printf("%04d", alarmTime->tm_year + 1900);
	hh->ParamList["alarm_hour"] = string_printf("%02d", alarmTime->tm_hour);
	hh->ParamList["alarm_min"]  = string_printf("%02d", alarmTime->tm_min);

	struct tm *stopTime = (cmd == "new") ? alarmTime : ( (timer.stopTime > 0) ? localtime(&(timer.stopTime)) : NULL );
	if(stopTime != NULL)
	{
		hh->ParamList["stop_mday"] = string_printf("%02d", stopTime->tm_mday);
		hh->ParamList["stop_mon"]  = string_printf("%02d", stopTime->tm_mon +1);
		hh->ParamList["stop_year"] = string_printf("%04d", stopTime->tm_year + 1900);
		hh->ParamList["stop_hour"] = string_printf("%02d", stopTime->tm_hour);
		hh->ParamList["stop_min"]  = string_printf("%02d", stopTime->tm_min);

		// APid settings for Record
		if(timer.apids == TIMERD_APIDS_CONF)
			hh->ParamList["TIMERD_APIDS_CONF"] = "y";
		if(timer.apids & TIMERD_APIDS_STD)
			hh->ParamList["TIMERD_APIDS_STD"]  = "y";
		if(timer.apids & TIMERD_APIDS_ALT)
			hh->ParamList["TIMERD_APIDS_ALT"]  = "y";
		if(timer.apids & TIMERD_APIDS_AC3)
			hh->ParamList["TIMERD_APIDS_AC3"]  = "y";
	}
	else
		hh->ParamList["stop_mday"] = "0";
	// event type
	std::string sel;
	for(int i=1; i<=8;i++)
	{
		if(i!=(int)CTimerd::TIMER_NEXTPROGRAM)
		{
			std::string zType = NeutrinoAPI->timerEventType2Str((CTimerd::CTimerEventTypes) i);
			if(cmd != "new")
				sel = (i==(int)timer.eventType) ? "selected=\"selected\"" : "";
			else
				sel = (i==(int)CTimerd::TIMER_RECORD) ? "selected=\"selected\"" : "";
			hh->ParamList["timertype"] +=
				string_printf("<option value=\"%d\" %s>%s\n",i,sel.c_str(),zType.c_str());
		}
	}
	// Repeat types
	std::string zRep;
	sel = "";
	for(int i=0; i<=6;i++)
	{
		if(i!=(int)CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION)
		{

			zRep = NeutrinoAPI->timerEventRepeat2Str((CTimerd::CTimerEventRepeat) i);
			if(cmd != "new")
				sel = (((int)timer.eventRepeat) == i) ? "selected=\"selected\"" : "";
			hh->ParamList["repeat"] +=
				string_printf("<option value=\"%d\" %s>%s</option>\n",i,sel.c_str(),zRep.c_str());
		}
	}
	// Repeat Weekdays
	zRep = NeutrinoAPI->timerEventRepeat2Str(CTimerd::TIMERREPEAT_WEEKDAYS);
	if(timer.eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS && cmd != "new")
		sel = "selected=\"selected\"";
	else
		sel = "";
	hh->ParamList["repeat"] +=
		string_printf("<option value=\"%d\" %s>%s</option>\n",(int)CTimerd::TIMERREPEAT_WEEKDAYS, sel.c_str(), zRep.c_str());

	// Weekdays
	char weekdays[8];
	CTimerd::getInstance()->setWeekdaysToStr(timer.eventRepeat, weekdays);
	hh->ParamList["weekdays"]=	 weekdays;

	// timer repeats
	if (timer.eventRepeat == CTimerd::TIMERREPEAT_ONCE)
		hh->ParamList["TIMERREPEAT_ONCE"]  = "y";
	hh->ParamList["timer_repeatCount"]  = itoa(timer.repeatCount);

	// program row
	t_channel_id current_channel = (cmd == "new") ? live_channel_id /*CZapit::getInstance()->getCurrentServiceID()*/ : timer.channel_id;
	CZapit::ChannelIterator cit = CZapit::getInstance()->tvChannelsBegin();
	for (; !(cit.EndOfChannels()); cit++) {
		sel = ((*cit)->channel_id == current_channel) ? "selected=\"selected\"" : "";
		hh->ParamList["program_row"] +=
			string_printf("<option value=\"%llx\" %s>%s</option>\n",
				(*cit)->channel_id, sel.c_str(), (*cit)->getName().c_str());
	}
	cit = CZapit::getInstance()->radioChannelsBegin();
	for (; !(cit.EndOfChannels()); cit++) {
		sel = ((*cit)->channel_id == current_channel) ? "selected=\"selected\"" : "";
		hh->ParamList["program_row"] +=
			string_printf("<option value=\"%llx\" %s>%s</option>\n",
				(*cit)->channel_id, sel.c_str(), (*cit)->getName().c_str());
	}
	// recordingDir
	hh->ParamList["RECORD_DIR_MAXLEN"] = itoa(RECORD_DIR_MAXLEN-1);
	if(cmd != "new") {
		if(timer.eventType == CTimerd::TIMER_RECORD)
			hh->ParamList["timer_recordingDir"] = timer.recordingDir;
	}
	else
	{
		// get Default Recordingdir
		CConfigFile *Config = new CConfigFile(',');
		Config->loadConfig(NEUTRINO_CONFIGFILE);
		hh->ParamList["timer_recordingDir"] = Config->getString("network_nfs_recordingdir", "/mnt/filme");
		delete Config;
	}
	hh->ParamList["standby"] = (cmd == "new")? "0" : ((timer.standby_on)?"1":"0");
	hh->ParamList["message"] = (cmd == "new")? "" : timer.message;
	hh->ParamList["pluginname"] = (cmd == "new")? "" : timer.pluginName;

	return "";
}

//-------------------------------------------------------------------------
// func: Bouquet Editor List
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_bouquet_editor_main(CyhookHandler *hh, std::string para)
{
	std::string yresult;
	int selected = -1;
	if (hh->ParamList["saved"] == "1")
		hh->ParamList["have_saved"]="true";

	if (hh->ParamList["selected"] != "")
		selected = atoi(hh->ParamList["selected"].c_str());

	int bouquetSize = (int) CZapit::getInstance()->Bouquets.size();
	for (int i = 0; i < (int) CZapit::getInstance()->Bouquets.size(); i++) {

		CZapitBouquet * bouquet = CZapit::getInstance()->Bouquets[i];

		char classname = ((i & 1) == 0) ? 'a' : 'b';
		classname = (selected == (int) i + 1)?'c':classname;

		std::string akt = (selected == (int) (i + 1)) ? "<a name=\"akt\"></a>" : "";
		// lock/unlock
		std::string lock_action = (bouquet->bLocked) ? "unlock" : "lock";
		std::string lock_img 	= (bouquet->bLocked) ? "lock" : "unlock";
		std::string lock_alt 	= (bouquet->bLocked) ? "unlock" : "lock";
		// hide/show
		std::string hidden_action= (bouquet->bHidden) ? "show" : "hide";
		std::string hidden_img 	= (bouquet->bHidden) ? "hidden" : "visible";
		std::string hidden_alt 	= (bouquet->bHidden) ? "hide" : "show";
		// move down
		std::string down_show 	= (i + 1 < bouquetSize) ? "visible" : "hidden";
		//move up
		std::string up_show 	= (i > 0) ? "visible" : "hidden";
		// build from template
		yresult += string_printf(para.c_str(), classname, akt.c_str(),
			i + 1, lock_action.c_str(), lock_img.c_str(), lock_alt.c_str(), //lock
			i + 1, hidden_action.c_str(), hidden_img.c_str(), hidden_alt.c_str(), //hhidden
			i + 1, bouquet->Name.c_str(), bouquet->Name.c_str(), //link
			i + 1, bouquet->Name.c_str(), //rename
			i + 1, bouquet->Name.c_str(), //delete
			down_show.c_str(), i + 1, //down arrow
			up_show.c_str(), i + 1); //up arrow
	}
	return yresult;
}

//-------------------------------------------------------------------------
// func: Bouquet Edit
//-------------------------------------------------------------------------
std::string  CNeutrinoYParser::func_set_bouquet_edit_form(CyhookHandler *hh, std::string)
{
	if (!(hh->ParamList["selected"].empty()))
	{
		int selected = atoi(hh->ParamList["selected"].c_str()) - 1;
		int mode = CZapit::getInstance()->getMode();
		ZapitChannelList* channels = mode == CZapit::MODE_TV ? &(CZapit::getInstance()->Bouquets[selected]->tvChannels) : &(CZapit::getInstance()->Bouquets[selected]->radioChannels);
		
		for(int j = 0; j < (int) channels->size(); j++) 
		{
			hh->ParamList["bouquet_channels"] +=
				string_printf("<option value=\"%llx\">%s</option>\n",
					(*channels)[j]->channel_id,
					(*channels)[j]->getName().c_str());
		}
		
		ZapitChannelList Channels;
		Channels.clear();
		
		if (mode == CZapit::MODE_RADIO) 
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() == ST_DIGITAL_RADIO_SOUND_SERVICE)
					Channels.push_back(&(it->second));
		} 
		else 
		{
			for (tallchans_iterator it = allchans.begin(); it != allchans.end(); it++)
				if (it->second.getServiceType() != ST_DIGITAL_RADIO_SOUND_SERVICE)
					Channels.push_back(&(it->second));
		}
		sort(Channels.begin(), Channels.end(), CmpChannelByChName());

		for (int i = 0; i < (int) Channels.size(); i++) 
		{
			if (!CZapit::getInstance()->existsChannelInBouquet(selected, Channels[i]->channel_id))
			{
				hh->ParamList["all_channels"] +=
					string_printf("<option value=\"%llx\">%s</option>\n",
						Channels[i]->channel_id,
						Channels[i]->getName().c_str());
			}
		}
		return "";
	}
	else
		return "No Bouquet selected";
}
