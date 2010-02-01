
all: test_80211_association_request_framebody test_80211_association_response_framebody test_80211_authentication_framebody test_80211_deauthentication_framebody test_80211_beacon_framebody test_80211_disassociation_framebody test_80211_probe_request_framebody test_80211_probe_response_framebody test_80211_reassociation_request_framebody test_80211_reassociation_response_framebody

test_80211_association_request_framebody: mac-802_11_management_framebody.o test_80211_association_request_framebody.o
	g++ -Wall -g -o test_80211_association_request_framebody mac-802_11_management_framebody.o test_80211_association_request_framebody.o 

test_80211_association_response_framebody: mac-802_11_management_framebody.o test_80211_association_response_framebody.o
	g++ -Wall -g -o test_80211_association_response_framebody mac-802_11_management_framebody.o test_80211_association_response_framebody.o 

test_80211_authentication_framebody: mac-802_11_management_framebody.o test_80211_authentication_framebody.o 
	g++ -Wall -g -o test_80211_authentication_framebody mac-802_11_management_framebody.o test_80211_authentication_framebody.o 

test_80211_deauthentication_framebody: mac-802_11_management_framebody.o test_80211_deauthentication_framebody.o 
	g++ -Wall -g -o test_80211_deauthentication_framebody mac-802_11_management_framebody.o test_80211_deauthentication_framebody.o 

test_80211_beacon_framebody: mac-802_11_management_framebody.o test_80211_beacon_framebody.o 
	g++ -Wall -g -o test_80211_beacon_framebody mac-802_11_management_framebody.o test_80211_beacon_framebody.o 

test_80211_disassociation_framebody: mac-802_11_management_framebody.o test_80211_disassociation_framebody.o 
	g++ -Wall -g -o test_80211_disassociation_framebody  mac-802_11_management_framebody.o test_80211_disassociation_framebody.o 

test_80211_probe_request_framebody: mac-802_11_management_framebody.o test_80211_probe_request_framebody.o 
	g++ -Wall -g -o test_80211_probe_request_framebody mac-802_11_management_framebody.o test_80211_probe_request_framebody.o 

test_80211_probe_response_framebody: mac-802_11_management_framebody.o test_80211_probe_response_framebody.o 
	g++ -Wall -g -o test_80211_probe_response_framebody mac-802_11_management_framebody.o test_80211_probe_response_framebody.o 

test_80211_reassociation_request_framebody: mac-802_11_management_framebody.o test_80211_reassociation_request_framebody.o 
	g++ -Wall -g -o test_80211_reassociation_request_framebody mac-802_11_management_framebody.o test_80211_reassociation_request_framebody.o 

test_80211_reassociation_response_framebody: mac-802_11_management_framebody.o test_80211_reassociation_response_framebody.o 
	g++ -Wall -g -o test_80211_reassociation_response_framebody mac-802_11_management_framebody.o test_80211_reassociation_response_framebody.o 

mac-802_11_management_framebody.o: ../mac-802_11_management_framebody.h ../mac-802_11_management_framebody.cc
	g++ -Wall -g -c -o mac-802_11_management_framebody.o ../mac-802_11_management_framebody.cc

test_80211_association_request_framebody.o: ../mac-802_11_management_framebody.h test_80211_association_request_framebody.c
	g++ -Wall -g -c -o test_80211_association_request_framebody.o test_80211_association_request_framebody.c

test_80211_association_response_framebody.o: ../mac-802_11_management_framebody.h test_80211_association_response_framebody.c
	g++ -Wall -g -c -o test_80211_association_response_framebody.o test_80211_association_response_framebody.c

test_80211_authentication_framebody.o: ../mac-802_11_management_framebody.h test_80211_authentication_framebody.c
	g++ -Wall -g -c -o test_80211_authentication_framebody.o test_80211_authentication_framebody.c

test_80211_deauthentication_framebody.o: ../mac-802_11_management_framebody.h test_80211_deauthentication_framebody.c
	g++ -Wall -g -c -o test_80211_deauthentication_framebody.o test_80211_deauthentication_framebody.c

test_80211_beacon_framebody.o: ../mac-802_11_management_framebody.h test_80211_beacon_framebody.c
	g++ -Wall -g -c -o test_80211_beacon_framebody.o test_80211_beacon_framebody.c

test_80211_disassociation_framebody.o: ../mac-802_11_management_framebody.h test_80211_disassociation_framebody.c
	g++ -Wall -g -c -o test_80211_disassociation_framebody.o test_80211_disassociation_framebody.c

test_80211_probe_request_framebody.o: ../mac-802_11_management_framebody.h test_80211_probe_request_framebody.c
	g++ -Wall -g -c -o test_80211_probe_request_framebody.o test_80211_probe_request_framebody.c

test_80211_probe_response_framebody.o: ../mac-802_11_management_framebody.h test_80211_probe_response_framebody.c
	g++ -Wall -g -c -o test_80211_probe_response_framebody.o test_80211_probe_response_framebody.c

test_80211_reassociation_request_framebody.o: ../mac-802_11_management_framebody.h test_80211_reassociation_request_framebody.c
	g++ -Wall -g -c -o test_80211_reassociation_request_framebody.o test_80211_reassociation_request_framebody.c

test_80211_reassociation_response_framebody.o: ../mac-802_11_management_framebody.h test_80211_reassociation_response_framebody.c
	g++ -Wall -g -c -o test_80211_reassociation_response_framebody.o test_80211_reassociation_response_framebody.c

clean:
	rm -f *.o test_80211_association_request_framebody test_80211_association_response_framebody test_80211_authentication_framebody test_80211_deauthentication_framebody test_80211_beacon_framebody test_80211_disassociation_framebody test_80211_probe_request_framebody test_80211_probe_response_framebody test_80211_reassociation_request_framebody test_80211_reassociation_response_framebody

