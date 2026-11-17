# SPDX-License-Identifier: MIT
objs := \
	$(ROOT)/bt/core/buf.o \
	$(ROOT)/bt/hci/hci.o \
	$(ROOT)/bt/l2cap/l2cap.o \
	$(ROOT)/bt/att/att.o \
	$(ROOT)/bt/gatt/gatt.o \
	$(ROOT)/bt/gap/gap.o \
	$(ROOT)/bt/smp/smp.o \
	$(ROOT)/bt/sdp/sdp.o \
	$(ROOT)/bt/rfcomm/rfcomm.o \
	$(ROOT)/bt/stack/stack.o \
	$(ROOT)/bt/profiles/profiles.o \
	$(ROOT)/bt/profiles/spp.o \
	$(ROOT)/bt/profiles/hfp.o \
	$(ROOT)/bt/profiles/a2dp.o \
	$(ROOT)/bt/profiles/avrcp.o \
	$(ROOT)/bt/profiles/tws.o \
	$(ROOT)/bt/profiles/hid.o \
	$(ROOT)/bt/profiles/pan.o \
	$(ROOT)/bt/profiles/dun.o \
	$(ROOT)/bt/profiles/obex.o \
	$(ROOT)/bt/common/packet.o \
	$(ROOT)/bt/common/hopping.o \
	$(ROOT)/bt/common/hci_bridge.o \
	$(ROOT)/bt/rf/radio.o \
	$(ROOT)/bt/rf/rf_coexist.o \
	$(ROOT)/bt/edr/baseband.o \
	$(ROOT)/bt/edr/arq.o \
	$(ROOT)/bt/edr/sco.o \
	$(ROOT)/bt/edr/edr_packet.o \
	$(ROOT)/bt/edr/edr_frame.o \
	$(ROOT)/bt/edr/edr_hw.o \
	$(ROOT)/bt/edr/edr_irq.o \
	$(ROOT)/bt/edr/edr_sched.o \
	$(ROOT)/bt/edr/lmp.o \
	$(ROOT)/bt/edr/ssp.o \
	$(ROOT)/bt/edr/edr_link.o \
	$(ROOT)/bt/le/le.o \
	$(ROOT)/bt/le/le_ll.o \
	$(ROOT)/bt/le/le_csa.o \
	$(ROOT)/bt/le/le_iso.o \
	$(ROOT)/bt/le/le_adv_ext.o \
	$(ROOT)/bt/le/le_hw.o \
	$(ROOT)/bt/le/le_irq.o \
	$(ROOT)/bt/le/le_ring.o \
	$(ROOT)/bt/le/le_timer.o \
	$(ROOT)/bt/le/le_link.o \

GEN_LIB := y
AR_OUT  := $(AR_DIR)/bt.a

archive: $(AR_OUT)

$(AR_OUT): object
	$(V) $(AR) $(AR_ARGS) $@ $(obj_files)

include $(MAKE_RULE)
