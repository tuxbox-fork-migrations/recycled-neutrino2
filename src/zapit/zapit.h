/*
  Client-Interface f�r zapit  -   DBoxII-Project

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

#ifndef __zapit__
#define __zapit__


#include <algorithm>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
#include <string.h>
#include <ctype.h>

#include <inttypes.h>

#include <OpenThreads/Thread>
#include <OpenThreads/Condition>

// zapit
#include <zapit/zapittypes.h>
#include <zapit/frontend_c.h>
#include <zapit/channel.h>
//#include <zapit/bouquets.h>


#define CHANNEL_NAME_SIZE 40
#define RESPONSE_GET_BOUQUETS_END_MARKER 0xFFFFFFFF

//typedef vector<CZapitChannel*> ZapitChannelList;

class CZapit
{
	public:
		enum events
		{
			FIRST_EVENT_MARKER,           // <- no actual event, needed by pzapit
			EVT_ZAP_COMPLETE = FIRST_EVENT_MARKER,
			EVT_ZAP_COMPLETE_IS_NVOD,
			EVT_ZAP_FAILED,
			EVT_ZAP_SUB_COMPLETE,
			EVT_ZAP_SUB_FAILED,
			EVT_ZAP_MOTOR,
			EVT_RECORDMODE_ACTIVATED,
			EVT_RECORDMODE_DEACTIVATED,
			EVT_SCAN_COMPLETE,
			EVT_SCAN_FAILED,
			EVT_SCAN_NUM_TRANSPONDERS,
			EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS,
 			EVT_SCAN_REPORT_FREQUENCY,
 			EVT_SCAN_REPORT_FREQUENCYP,
 			EVT_SCAN_SERVICENAME,
 			EVT_SCAN_FOUND_A_CHAN,
 			EVT_SCAN_FOUND_TV_CHAN,
 			EVT_SCAN_FOUND_RADIO_CHAN,
 			EVT_SCAN_FOUND_DATA_CHAN,
			EVT_SCAN_SATELLITE,
			EVT_SCAN_NUM_CHANNELS,
			EVT_SCAN_PROVIDER,
			EVT_BOUQUETS_CHANGED,
			EVT_ZAP_CA_CLEAR,
			EVT_ZAP_CA_LOCK,
			EVT_ZAP_CA_FTA,
			EVT_ZAP_CA_ID,
			EVT_SDT_CHANGED,
			EVT_SERVICES_CHANGED,
			EVT_PMT_CHANGED,

			EVT_ZAP_COMPLETE_IS_WEBTV,
			
			LAST_EVENT_MARKER             // <- no actual event, needed by pzapit
		};

		enum zapStatus
		{
			ZAP_OK = 0x01,
			ZAP_IS_NVOD = 0x02,
			ZAP_INVALID_PARAM = 0x04,
			ZAP_IS_WEBTV = 0x08
		};

		enum bouquetMode
		{
			BM_DELETEBOUQUETS,
			BM_DONTTOUCHBOUQUETS,
			BM_UPDATEBOUQUETS,
			BM_CREATESATELLITEBOUQUET
		};

		enum scanType
  		{
  			ST_TVRADIO,
  			ST_TV,
  			ST_RADIO,
  			ST_ALL
  		};

		typedef enum channelsMode_
		{
			MODE_CURRENT,
			MODE_TV,
			MODE_RADIO
		} channelsMode;

		typedef enum channelsOrder_
		{
			SORT_ALPHA,
			SORT_BOUQUET
		} channelsOrder;

		struct commandAddSubServices
		{
			t_original_network_id original_network_id;
			t_service_id          service_id;
			t_transport_stream_id transport_stream_id;
		};
		typedef std::vector<commandAddSubServices> subServiceList;

		struct commandSetScanSatelliteList
		{
			char satName[50];
			int  position;
			
			int type;
		};
		typedef std::vector<commandSetScanSatelliteList> ScanSatelliteList;

		struct commandSetScanMotorPosList
		{
			t_satellite_position satPosition;
			int motorPos;
		};
		typedef std::vector<commandSetScanMotorPosList> ScanMotorPosList;

		struct responseGetLastChannel
		{
			unsigned int	channelNumber;
			char		mode;
		};

		struct responseGetBouquets
		{
			unsigned int bouquet_nr;
			char	 name[30];
			bool	 locked;
			bool	 hidden;
		};
		typedef std::vector<responseGetBouquets> BouquetList;

		struct responseChannels
		{
			unsigned int nr;
			t_channel_id channel_id;
			char	 name[CHANNEL_NAME_SIZE];
			t_satellite_position satellitePosition;
			unsigned char service_type;
		};

		struct responseGetBouquetChannels : public responseChannels
		{};

		typedef std::vector<responseGetBouquetChannels> BouquetChannelList;

		struct responseNChannels
		{
			unsigned int nr;
		};

		struct responseGetBouquetNChannels : public responseNChannels
		{};
		typedef std::vector<responseGetBouquetNChannels> BouquetNChannelList;

		struct responseGetAPIDs
		{
			uint32_t    pid;
			char    desc[25];
			int     is_ac3;
			int     component_tag;
		};
		typedef std::vector<responseGetAPIDs> APIDList;
		
		struct responseGetSubPIDs
		{
			uint    pid;
			char    desc[4];
			uint	composition_page;
			uint	ancillary_page;
			bool	hearingImpaired;
		};
		typedef std::vector<responseGetSubPIDs> SubPIDList;

		struct responseGetOtherPIDs
		{
			uint		vpid;
			uint		vtxtpid;
			uint		pcrpid;
			uint		selected_apid;
			uint		pmtpid;
			uint		privatepid;
		};

		class CCurrentServiceInfo
		{
			public:
				t_original_network_id onid;
				t_service_id           sid;
				t_transport_stream_id tsid;
				unsigned short	vpid;
				unsigned short  vtype;
				unsigned short	apid;
				unsigned short	pcrpid;
				unsigned short	vtxtpid;
				unsigned int	tsfrequency;
				unsigned char	polarisation;
				unsigned char	diseqc;
				unsigned short  pmtpid;
				unsigned short  pmt_version;
				uint32_t	rate;
				fe_code_rate	fec;
		};

		struct responseGetPIDs
		{
			responseGetOtherPIDs	PIDs;
			APIDList		APIDs;
			SubPIDList  		SubPIDs;
		};

		struct responseGetSatelliteList
		{
			char satName[50];
			t_satellite_position satPosition;
			uint8_t motorPosition;
			int satDiseqc;

			delivery_system_t system;
		};
		typedef std::vector<responseGetSatelliteList> SatelliteList;
		
		struct commandStartScan
		{
			int scan_mode;
			int feindex;
		};
		
		struct commandScanTP
		{
			TP_params TP;
			int feindex;
		};
		
	private:
		static OpenThreads::Mutex mutex_zapit;
		
		//
		void initFrontend();
		void OpenFE();
		void CloseFE();
		CFrontend * getVTuner();
		bool loopCanTune(CFrontend * fe, CZapitChannel * thischannel);
		CFrontend * getPreferredFrontend(CZapitChannel * thischannel);
		CFrontend * getFrontend(CZapitChannel * thischannel);
		CFrontend * getRecordFrontend(CZapitChannel * thischannel);
		void lockFrontend(CFrontend *fe);
		void unlockFrontend(CFrontend *fe);
		
		void loadAudioMap();
		void saveAudioMap();
		void loadVolumeMap();
		void saveVolumeMap();
		void saveZapitSettings(bool write, bool write_a);
		void loadZapitSettings();
		void sendCaPmtPlayBackStart(CZapitChannel * thischannel, CFrontend * fe);
		void sendcapmtPlayBackStop(bool _sendPmt);
		void sendCaPmtRecordStop(void);
		void save_channel_pids(CZapitChannel * thischannel);
		CZapitChannel * find_channel_tozap(const t_channel_id channel_id, bool in_nvod);
		bool tune_to_channel(CFrontend * frontend, CZapitChannel * thischannel, bool &transponder_change);
		bool parse_channel_pat_pmt(CZapitChannel * thischannel, CFrontend * fe);
		void restore_channel_pids(CZapitChannel * thischannel);
		int zapit(const t_channel_id channel_id, bool in_nvod, bool forupdate = 0);
		int zapTo_RecordID(const t_channel_id channel_id);
		void setPidVolume(t_channel_id channel_id, int pid, int percent);
		int getPidVolume(t_channel_id channel_id, int pid, bool ac3);
		void setVolumePercent(int percent);
		int change_audio_pid(uint8_t index);
		void setRecordMode(void);
		void unsetRecordMode(void);
		void setRadioMode(void);
		void setTVMode(void);
		int prepare_channels();
		
		unsigned int zapTo_ChannelID(const t_channel_id channel_id, const bool isSubService);

		//
		void sendAPIDs(APIDList &apids);
		void sendSubPIDs(SubPIDList &subpids);
		void sendRecordAPIDs(APIDList &apids);
		void sendRecordSubPIDs(SubPIDList &subpids);
		
		//
		void internalSendChannels(ZapitChannelList* channels, const unsigned int first_channel_nr);
		void internalSendNChannels(ZapitChannelList* channels, const unsigned int first_channel_nr);
		void sendBouquets(CZapit::responseGetBouquets &msgBouquet, const bool emptyBouquetsToo, CZapit::channelsMode mode);
		void sendBouquetChannels(const unsigned int bouquet, const CZapit::channelsMode mode);
		void sendBouquetNChannels(const unsigned int bouquet, const CZapit::channelsMode mode);
		void sendChannels(ZapitChannelList &channels, const CZapit::channelsMode mode, const CZapit::channelsOrder order);
		void sendNChannels(ZapitChannelList &channels, const CZapit::channelsMode mode, const CZapit::channelsOrder order);
		
		//
		void closeAVDecoder(void);
		void openAVDecoder(void);

		//
		void enterStandby(void);
		void leaveStandby(void);
		
		//
		int start_scan(CZapit::commandStartScan StartScan);
		
		CZapit(){};
	public:
		~CZapit(){};
		static CZapit *getInstance()
		{
			static CZapit * zapit = NULL;

			if(!zapit) 
			{
				zapit = new CZapit();
			} 

			return zapit;
		};
		
		//
		CFrontend * getFE(int index);
		void initTuner(CFrontend * fe);
		void saveFrontendConfig(int feindex);
		void loadFrontendConfig();
		
		void setZapitConfig(Zapit_config * Cfg);
		void getZapitConfig(Zapit_config *Cfg);
		
		void parseScanInputXml(fe_type_t fe_type);
		
		bool CanZap(CZapitChannel * thischannel);
		
		//
		void getLastChannel(unsigned int &channumber, char &mode);
		int getMode(void);
		void setMode(const CZapit::channelsMode mode);

		// playback
		int startPlayBack(CZapitChannel *);
		int stopPlayBack(bool sendPmt = false);
		void pausePlayBack(void);
		void continuePlayBack(void);
		void lockPlayBack();
		void unlockPlayBack();
		bool isPlayBackActive();
		void setStandby(bool enable);

		bool isChannelTVChannel(const t_channel_id channel_id);
		bool isChannelWEBTVChannel(const t_channel_id channel_id);
		bool isChannelRadioChannel(const t_channel_id channel_id);

		void zapTo_serviceID_NOWAIT(const t_channel_id channel_id);
		void zapTo_subServiceID_NOWAIT(const t_channel_id channel_id);
		unsigned int zapTo(const unsigned int bouquet, const unsigned int channel);
		unsigned int zapTo(const unsigned int channel);
		unsigned int zapTo_serviceID(const t_channel_id channel_id);
		unsigned int zapTo_subServiceID(const t_channel_id channel_id);
		unsigned int zapTo_record(const t_channel_id channel_id);

		std::string getChannelName(const t_channel_id channel_id);
		int getChannelNumber(const t_channel_id channel_id);
		std::string getChannelURL(const t_channel_id channel_id);
		std::string getChannelDescription(const t_channel_id channel_id);

		// current service
		void getPIDS(CZapit::responseGetPIDs &pids);
		t_channel_id getCurrentServiceID();
		CZapit::CCurrentServiceInfo getCurrentServiceInfo();
		int32_t getCurrentSatellitePosition();
		bool getCurrentTP(TP_params *TP);

		// novd
		void setSubServices( CZapit::subServiceList& subServices );

		// record
		void setRecordMode(const bool activate);
		bool isRecordModeActive();
		t_channel_id getRecordServiceID();
		CZapit::CCurrentServiceInfo getRecordServiceInfo();
		void getRecordPIDS(CZapit::responseGetPIDs &pids);

		void reinitChannels();
		void reloadCurrentServices();
		bool reZap();

		// frontend
		void sendMotorCommand(uint8_t cmdtype, uint8_t address, uint8_t cmd, uint8_t num_parameters, uint8_t param1, uint8_t param2, int feindex = 0);
		delivery_system_t getDeliverySystem(int feindex = 0);

		// audio / video
		void muteAudio(const bool mute);
		bool getMuteStatus();
		void getAudioMode(int * mode);
		void setAudioMode(int mode);
		void setVolume(const unsigned int left, const unsigned int right);
		void getVolume(unsigned int *left, unsigned int *right);
		void setVolumePercent(const unsigned int percent, t_channel_id channel_id = 0, const unsigned int apid = 0);
		void getVolumePercent(unsigned int *percent, t_channel_id channel_id = 0, const unsigned int apid = 0, const bool is_ac3 = false);
		void setAudioChannel(const unsigned int channel);
		void setVideoSystem(int video_system);

		// channels / bouquets
		//bool getChannels(CZapit::BouquetChannelList& channels, const CZapit::channelsMode mode = CZapit::MODE_CURRENT, const CZapit::channelsOrder order = CZapit::SORT_BOUQUET, const bool utf_encoded = false);
		//void getBouquets( CZapit::BouquetList& bouquets, const bool emptyBouquetsToo = false, const bool utf_encoded = false, CZapit::channelsMode mode = CZapit::MODE_CURRENT);
		bool getBouquetChannels(const unsigned int bouquet, CZapit::BouquetChannelList& channels, const CZapit::channelsMode mode = CZapit::MODE_CURRENT, const bool utf_encoded = false);
		//bool getBouquetNChannels(const unsigned int bouquet, CZapit::BouquetNChannelList& channels, const CZapit::channelsMode mode = CZapit::MODE_CURRENT, const bool utf_encoded = false);

		// bouquetManager
		void renumChannellist();
		void saveBouquets();
		void restoreBouquets();
		void addBouquet(const char * const name); // UTF-8 encoded
		void deleteBouquet(const unsigned int bouquet);
		void renameBouquet(const unsigned int bouquet, const char * const newName); // UTF-8 encoded
		void moveBouquet(const unsigned int bouquet, const unsigned int newPos);
		void moveChannel(const unsigned int bouquet, unsigned int oldPos, unsigned int newPos, CZapit::channelsMode mode = CZapit::MODE_CURRENT);
		signed int existsBouquet(const char * const name); // UTF-8 encoded
		void setBouquetLock(const unsigned int bouquet, const bool lock);
		void setBouquetHidden(const unsigned int bouquet, const bool hidden);
		void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id);
		bool existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id);
		void removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id);

		// scan
		bool tuneTP(TP_params TP, int feindex = 0);
		bool scanTP(TP_params TP, int feindex = 0);
		bool isScanReady(unsigned int &satellite, unsigned int &processed_transponder, unsigned int &transponder, unsigned int &services );
		void getScanSatelliteList( CZapit::SatelliteList &satelliteList );
		void setScanSatelliteList( CZapit::ScanSatelliteList &satelliteList );
		void setScanType(const CZapit::scanType mode);
		void setFEMode(const fe_mode_t mode, int feindex = 0);
		void setScanBouquetMode(const CZapit::bouquetMode mode);
		void setDiseqcType(const diseqc_t diseqc, int feindex = 0);
		void setDiseqcRepeat(const uint32_t repeat, int feindex = 0);
		void setScanMotorPosList( CZapit::ScanMotorPosList& motorPosList );
		bool startScan(int scan_mode, int feindex = 0);
		bool stopScan();

		//
		void Start(Z_start_arg *ZapStart_arg);
		void run();
		void Stop();
};

#endif