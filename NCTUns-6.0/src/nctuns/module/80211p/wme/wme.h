/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

/*
 *	WME module version information: IEEE P1609.3/D1.0, IEEE P 1609.4/D1.0
 *	03/26/2009 modify
 */


#include <object.h>
#include <packet.h>
#include <arpa/inet.h>
#include <timer.h>
#include <ethernet.h>
/*
 *	define WME MIB constant also used in WSA and WSM
 */

#define MaxServceInfoEntry		128

#define	MaxRateSet			127
#define	MaxAdvertiserIdentifier		32
#define DefaultWsmMaxLength		1400
#define MaxProviderServiceContext	31
#define MaxAdvertiserID			32

#define Channel_Best_Available		0
#define NumOfSch			6



/*
 *	primitive enum define
 */
enum	ServiceStatus {
	empty,
	used
};

enum	Action {
//	provider/user/WSM/cch service request - Action
	add,
	del,
	change	//provider service request(only)
};
/*
enum	UserRequestType {
//	user service request - UserRequestType
	Auto_access_on_service_match,
	Auto_access_unconditionally,
	no_sch_access
};
*/

/*
 *	WME MIB ELEMENT
 */
struct LocalInfo {
//	Used by All device
	
	int		operational_rate_set[MaxRateSet];			// (not used here) 1~127
	uint8_t		number_of_channel_support;	  			// 0~200 from 1609.3 (not used here)
	char		aid[MaxAdvertiserIdentifier];			  	// Advertiser Identifier (optional)
	uint16_t	reg_port;			  			// or Service request port
	uint16_t	wsm_forwarder_port;		  			// used in forwarding WSMP packet
	int		wsm_max_len;			  			// default 1400
	
};

struct CchServiceInfo {
//	Used by All device
 
	/*LocalServiceIndex;*/			  // to be determinted
	uint8_t		app_priority;		  // 0~63 0 the lowest 63 the height
	ServiceStatus	service_state;
};

struct ChannelInfo {
//	Used by providers
	
	uint8_t		channel_num;		// 8 bit unsigned int
	uint8_t		adaptable;		// 8 bits long and indicates whether Data Rate and TxPwr_Level are boundary values or fixed values.
	uint8_t		data_rate;		/* (2~127) int from X'02-X'7f, corresponding to data rates in increments of 500kbit/s from 1 Mb/s to 63.5 Mb/s.
						 * Data Rate is interpreted as the minimum rate allowed and any higher rate is also allowed.*/
	uint8_t		txpwr_level;		// (1~8) TxPwr_Level is interpreted as the maximum power allowed, and any lower power is also allowed.
	u_char		edca_parameter_set[20];	// not used here
};

struct AvailableServiceInfo {
// 	Used by Users

	uint32_t	psid;					// ProviderServiceIdentifier 4 bytes unsigned int
	char		psc[MaxProviderServiceContext];		// ProviderServiceContext 0~31 char
	uint8_t		app_priority;				// 0~63 0 the lowest 63 the height
	ChannelInfo	channel_info;
	u_char		souece_mac_addr[6];			// 48 bits
	char            aid[MaxAdvertiserIdentifier];           // Advertiser Identifier (optional)
//	int             basic_rate_set[MaxRateSet];         	// (not used here) 2~127 
//	int             operational_rate_set[MaxRateSet];         // (not used here) 1~127
	//TransmitLocation 					not define
	//TransmitPower						not define
	//ReceivePowerThreshold					not define
	//LinkQuality						not define
	u_char		ipv6_addr[16];				// 128 bits
	uint16_t	service_port;
	u_char		provider_mac_addr[6];
	u_char          default_gateway[16];
	u_char		gateway_mac_addr[6];
	//Certificate;						// not support in nctuns
	ServiceStatus	service_state;

	/* for available service recoard -> Link Quality */
	uint8_t		repeats;
	uint8_t		cch_recvs;
};

struct UserServiceInfo {
//	Used by Users

	//LocalServiceIndex;		not define
	//UserRequestType	user_request_type;
	uint32_t	psid;					// ProviderServiceIdentifier 4 bytes unsigned int
	char		psc[MaxProviderServiceContext];		// ProviderServiceContext 0~31 char
	uint8_t		app_priority;			// 0~63 0 the lowest 63 the height
	u_char		source_mac_addr[6];
	char            aid[MaxAdvertiserIdentifier];           // Advertiser Identifier (optional)
	ChannelInfo	channel_info;
	//LinkQuality                                           not define
//	bool		notify;					// whether WME should send notifications to the requesting entity regarding the status of this request (not supported).
	bool		immediate_access;			// access to the SCH without waiting for the next SCH interval.
	bool		indefinite_access;			// allows communications access to the SCH without pauses for CCH access.
	ServiceStatus	service_state;
};

struct ProviderServiceInfo {
//	Used by Providera

	//LocalServiceIndex;					not define
	uint8_t 	wsa_type;				// WaveServiceAdvertisement/Secured WSA(not supported)
	uint32_t	psid;					// ProviderServiceIdentifier 4 bytes unsigned int
	char		psc[MaxProviderServiceContext];		// ProviderServiceContext 0~31 char
	uint8_t		app_priority;				// 0~63 0 the lowest 63 the height
	u_char		recipient_mac[6];			// If present, the Recipient MAC Address from the WME-ProviderService.request is used; otherwise set to the broadcast MAC address.
	uint8_t		channle_selection;			// the channel number
	bool		persistence;				// Used in advertisement scheduling, whether the advertisement is repeated.
	int		repeats;				// If Recipient MAC Address is present, indicating a unicast delivery, Repeats should equal 0. (0~7)
	bool		ip_service;				// whether the advertised services is IP-based
	u_char		ipv6_addr[16];				// 128 bits
	uint16_t	service_port;
	u_char		provider_mac_addr[6];			
//	bool		notify;					// whether WME should send notifications to the requesting entity regarding the status of this request (not supported).
	ServiceStatus	service_state;	
	
};

struct WsmServiceInfo {
//	Used by devices supporting WSMP

	//LocalServiceIndex;					not define
	uint32_t	psid;					// ProviderServiceIdentifier 4 bytes unsigned int
	ServiceStatus	service_state;	
};

struct WRA {						//WaveRoutingAdvertisement
//	Used by providers of IP services

	uint16_t	router_life_time;		//second
	u_char		ip_prefix[16];			//IPv6 subnet prefix of the link
	uint8_t		prefix_len;
	u_char		default_gateway[16];
	u_char		gateway_macaddr[6];
	u_char		primay_dns[16];
	u_char		secondary_dns[16];

};

struct MIB {
	struct LocalInfo		local_info;
	struct CchServiceInfo		cch_service_info[MaxServceInfoEntry];
	struct AvailableServiceInfo	available_service_info[MaxServceInfoEntry];
	struct UserServiceInfo		user_service_info[MaxServceInfoEntry];
	struct ProviderServiceInfo	provider_service_info[MaxServceInfoEntry];
	struct ChannelInfo		channel_info[MaxServceInfoEntry];
	struct WsmServiceInfo		wsm_service_info[MaxServceInfoEntry];
	struct WRA			wra;
	
};


/*
 *	WSA Struct Element
 */
struct WSA_HDR {				//WSA header
	uint8_t wave_ver;			//WAVE versions
	uint8_t hdr_len;			//Header length
	uint8_t	hdr_cont;			//Header contents
	uint8_t	PR;				//Pertences(B0) and Repeats(B1~B7)
	uint8_t transmit_power;			//optional 0~255 dB
	uint8_t	tarnsmit_location;		//optional
	uint8_t	aid_len;			//optional 1~32
	char	aid[MaxAdvertiserIdentifier];	//optional
};

struct PST_Entry {					//provider service tanle entry
	uint8_t		provider_len;			//provider length
	uint8_t		provider_cont;			//provider contents
	uint32_t	psid;	
	uint8_t		psc_len;			//provider service context length(0~31)
	char		psc[MaxProviderServiceContext];	//provider service context
	uint8_t         app_priority;   	   	// 0~63 0 the lowest 63 the height
	uint8_t		channel_num;			//channel number
	u_char		ipv6_addr[16];			//IPv6 Address (optional)
	uint16_t	service_port;
	uint8_t		provider_device_addr;		//(optional) (not defind)
	u_char		provider_mac_addr[6];		//(optional)
	PST_Entry 	*next;
};

struct WSA_ChannelInfo {
	uint8_t		channel_len;		//channel length
	uint8_t		channel_cont;		//channel contents
	uint8_t		channel_num;		//channel number
	uint8_t		adaptable;
	uint8_t		data_rate;
	uint8_t		txpwr_level;
	u_char		edca_param_set[20];	//optional
	WSA_ChannelInfo	*next;
};

struct PST {					//provider service table
	uint8_t			provider_count;
	struct PST_Entry 	*pst_first;
	uint8_t 		channel_count;
	struct WSA_ChannelInfo 	*ci_first;
};

struct WSA_WRA {					//optional
	uint8_t			wra_cont;		//WRA contents
	uint16_t		router_lifetime;	//router life time
	u_char			ip_prefix[16];
	uint8_t			prefix_len;
	u_char			default_gateway[16];
	u_char			gateway_mac_addr[6];	//Gateway MAC address
	u_char			primary_dns[16];
	u_char			second_dns[16];		//optional
};

struct WSA {

	uint8_t wsa_type;
	struct WSA_HDR wsa_hdr;
	struct PST pst;
	uint8_t wra_len;
	struct WSA_WRA wra;
};


/*
 *	WME primitive present struct 
 */
enum WME_Primitive_Type {
	provider_service_req,
	user_service_req,
	wsm_service_req,
	cch_service_req
};

struct ProviderServiceReq {
	//LocalServiceIndex;                                    not define
	//Action		action;					//add del change
	uint32_t	psid;
	char		psc[MaxProviderServiceContext];		//0~31 charactor
	uint8_t         app_priority;  		    		// 0~63 0 the lowest 63 the height
	uint8_t         channle_selection;                       	// the channel number
        bool            persistence;                             // Used in advertisement scheduling, whether the advertisement is repeated.
        int             repeats;                                // If Recipient MAC Address is present, indicating a unicast delivery, Repeats should equal 0. (0~7)
	//ReceivePowerTheshold
	bool		ip_service;
	u_char		ipv6_addr[16];				//optional
	uint16_t	service_port;				//optional
	u_char		provider_mac_addr[6]; 			//optional
//	uint8_t		receive_power_threshold;		//optional 0~255 dB
	u_char		reciptent_mac_addr[6];			//optional Used to individually address the advertisement.
//	bool		notify;
	
};

struct	UserServiceReq {
	//LocalServiceIndex;                                    not define
	//Action          action;                                 //add del
	//UserRequestType	user_req_type;
	uint32_t	psid;
	char            psc[MaxProviderServiceContext];         //optional 0~31 charactor
	uint8_t		channel_num;				//optional
	u_char		source_mac_addr[6];			//optional
	char    	aid[MaxAdvertiserIdentifier];   	//optional
	//link_quality						//not define
	bool		immediate_access;
	bool		indefinite_access;
//	bool		notify;
};

struct WsmServiceReq {
	//LocalServiceIndex;                                    not define
	//Action                action;                         //add del
	uint32_t		psid;
};

struct CchServiceReq {
	//LocalServiceIndex;                                    not define
	//Action          action;                                 //add del
	uint8_t		app_priority;
};

struct WME_Primitive_Q {
	uint64_t			time;			//the primitive process time
	WME_Primitive_Type		wme_primitive_type;	//what primitive did the higher layer use?
	Action			        action;			//add del change
	struct ProviderServiceReq	provider_service_req;
	struct UserServiceReq 		user_service_req;
	struct WsmServiceReq		wsm_service_req;
	struct CchServiceReq		cch_service_req;
	struct WME_Primitive_Q		*next;
};

struct ServiceIndex {
	uint8_t			index;
	struct ServiceIndex	*next;
};

struct User_List {						//user join list
	struct ServiceIndex    *joined_service;
	uint8_t			joined_count;			//number of user service join the provider
	uint8_t 		max_priority;
	uint8_t			max_priority_index;
	uint8_t			max_priority_channel;
	
};

struct SCH_State {
	
	int	used[NumOfSch];
	int	count;
};

enum	DeviceStatus {
	none,
	provider,
	user,
	//p_u		//doing provider and user at the some time 未完成 重要性低
};

class WME : public NslObject{

	public :
		int CCH;// = 178;							//control channel number
		int SCH[NumOfSch];// = {174, 175, 176, 180, 181, 182};			//service channel numbers
		struct SCH_State		sch_state;

		struct MIB			mib;
		//uint8_t				antenna_num;			//only support one antenna 
		struct User_List		user_list;
		struct WME_Primitive_Q		*wme_primitive_q;
		DeviceStatus			device_status;
		u_char				*my_mac_addr, *recv_mac_addr;

	
		timerObj			wsa_t, primitive_t;	//timer for wsa send and the last wme primitive
		int				nid;			//my node id
		int				stack_port;		//protocol stack port connect to the interface 
		char				*primitive_file;
		struct WSA			wsa;
		char				*wsa_data;
		int				wsa_len;
		int				global_repeats;
		uint64_t			wsa_send_t[8];
		
		WME(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
		~WME();
		int 				init();
		int				set_wsa_timer(Event_ *e);
		int				set_primitive_timer(Event_ *e);
		
		void				wme_provider_service_req(struct WME_Primitive_Q *tmp);
		void				wme_user_service_req(struct WME_Primitive_Q *tmp);
		void				wme_wsm_service_req(struct WME_Primitive_Q *tmp);
		void				wme_cch_service_req(struct WME_Primitive_Q *tmp);

		void				start_provider_service(int index);
		void				send_wsa();
		void				recv_wsa(Packet *pkt);
		void				generate_wsa();
		void				modify_wsa(int index);				//未完成
		void				del_wsa(uint32_t psid);				//未完成
		int 				fill_data(void *dst, void *src, int size, int index, int max);
		
		int 				get_line(int fd, char *mesg, int n);
		int 				get_token(char *str, char **token, char *param);
		struct WME_Primitive_Q *	read_primitive_setting();
		int 				recv(ePacket_ *);
		int 				send(ePacket_ *);

		void 				send_wsm(struct WSM_Header wsm_header, char *wsm_data);
		void				recv_wsm(char *data);
		void 				MLME_CHANNELINACTIVITY_indication(int channel_num);

		void				to_cch();
};
