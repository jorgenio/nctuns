#
# Makefile for the NCTUns engine module
#

obj-y	= \
	base_station_info.o \
	frame_mngr.o \
	mac802_16_meshbs.o \
	mac802_16_meshss.o \
	mesh_connection.o \
	mesh_link.o \
	mesh_route.o \
	msh_dsch.o \
	msh_ncfg.o \
	msh_nent.o \
	neighbor.o \
	neighbor_mngr.o \
	net_entry_mngr.o \
	network_des.o \
	node_deg_des.o \
        util_map.o

CXXFLAGS_mac802_16_meshbs.o:=-Wno-deprecated
CXXFLAGS_mac802_16_meshss.o:=-Wno-deprecated
CXXFLAGS_mesh_connection.o:=-Wno-deprecated
CXXFLAGS_mesh_link.o:=-Wno-deprecated
CXXFLAGS_msh_dsch.o:=-Wno-deprecated
CXXFLAGS_msh_ncfg.o:=-Wno-deprecated
CXXFLAGS_neighbor.o:=-Wno-deprecated
CXXFLAGS_neighbor_mngr.o:=-Wno-deprecated
CXXFLAGS_net_entry_mngr.o:=-Wno-deprecated
CXXFLAGS_util_map.o:=-Wno-deprecated

obj-y	+= \
	msh_ncfg/ \
	sch/
